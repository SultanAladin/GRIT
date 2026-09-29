"""
GT3-style automatic shift policy simulator.

Reproduces the GRIT VehicleSolver shift policy with the same constants
(DCT preset 1, 6 forward gears, FinalDrive=3.55, ShiftTime=0.02s) and
plays out a full-throttle launch on a low-grip surface to recreate the
1<->2 oscillation seen in PowertrainLog.csv.

Outputs a transition log so we can compare candidate policies head-to-head
before touching C++.
"""
from __future__ import annotations

import math
from dataclasses import dataclass, field
from typing import List, Tuple

# --- Constants (mirror DrivetrainSpecs preset 1) ---
RAD_S_TO_RPM = 60.0 / (2.0 * math.pi)
RPM_TO_RAD_S = 1.0 / RAD_S_TO_RPM

GEAR_NAMES = ["R2", "R1", "N", "1", "2", "3", "4", "5", "6"]
GEAR_RATIOS = [-3.8, -2.0, 0.0, 4.2, 3.1, 2.5, 1.9, 1.5, 1.2]
FINAL_DRIVE = 3.55
SHIFT_TIME = 0.02
FIRST_FWD = 3
LAST_GEAR = len(GEAR_RATIOS) - 1

IDLE_RPM = 800.0
REDLINE_RPM = 7000.0

# Stylized torque curve: peak ~480 Nm @ 5500, falls off near redline.
def sample_torque(rpm: float) -> float:
    rpm = max(0.0, rpm)
    if rpm < IDLE_RPM:
        return 50.0
    # Triangular hump
    if rpm <= 5500.0:
        return 50.0 + (480.0 - 50.0) * (rpm - IDLE_RPM) / (5500.0 - IDLE_RPM)
    if rpm <= REDLINE_RPM:
        return 480.0 - (480.0 - 350.0) * (rpm - 5500.0) / (REDLINE_RPM - 5500.0)
    return 350.0  # past redline -- limiter clamps anyway


# Shift map: ShiftMap[gear] = (UpshiftRPM, DownshiftRPM)
SHIFT_MAP = [
    (4000.0, 1500.0),    # R2
    (99999.0, 1500.0),   # R1
    (0.0, 0.0),          # N
    (6500.0, 2000.0),    # 1st
    (6800.0, 2200.0),    # 2nd
    (7000.0, 2500.0),    # 3rd
    (7000.0, 2800.0),    # 4th
    (7000.0, 3000.0),    # 5th
    (99999.0, 3200.0),   # 6th
]


@dataclass
class VehicleState:
    t: float = 0.0
    speed_ms: float = 0.0          # ground speed
    wheel_omega: float = 0.0       # rad/s
    engine_rpm: float = IDLE_RPM
    gear: int = FIRST_FWD          # start in 1st
    target_gear: int = FIRST_FWD
    is_shifting: bool = False
    shift_phase: str = "None"      # None | ClutchRelease | GearSwap | RevMatch
    shift_timer: float = 0.0
    clutch_pos: float = 1.0
    hyst_timer: float = 0.0
    log: List[str] = field(default_factory=list)


# --- Vehicle parameters ---
WHEEL_RADIUS = 0.45        # m
VEHICLE_MASS = 1500.0      # kg
DRIVE_INERTIA = 0.6        # kg.m^2 -- rolling driveline + wheels (lumped)
ENGINE_INERTIA = 0.2       # kg.m^2
ROLLING_RES = 200.0        # N
DRAG_COEFF = 0.45 * 1.225 * 2.0   # 0.5*rho*Cd*A approx -> N per (m/s)^2 / 2

# Tire grip: low to recreate wheelspin. Friction coefficient * normal load.
TIRE_FX_MAX = 0.18 * VEHICLE_MASS * 9.81 * 0.5  # very low grip -> reproduces wheelspin loop


def policy_current(state: VehicleState, throttle: float, brake: float) -> int | None:
    """Returns desired gear *change* delta (+1, -1, or None) per current C++ logic."""
    if state.is_shifting or state.hyst_timer > 0.0:
        return None
    g = state.gear
    if g < FIRST_FWD:
        return None

    should_up = False
    should_down = False

    # --- A. Torque-based upshift ---
    if g < LAST_GEAR and throttle > 0.5:
        cur_ratio = GEAR_RATIOS[g]
        nxt_ratio = GEAR_RATIOS[g + 1]
        rpm_cur = state.wheel_omega * cur_ratio * FINAL_DRIVE * RAD_S_TO_RPM
        rpm_nxt = state.wheel_omega * nxt_ratio * FINAL_DRIVE * RAD_S_TO_RPM
        min_safe = max(IDLE_RPM * 1.5, REDLINE_RPM * 0.35)
        if rpm_nxt > min_safe and rpm_cur < REDLINE_RPM - 100.0:
            tw_cur = sample_torque(rpm_cur) * cur_ratio * FINAL_DRIVE
            tw_nxt = sample_torque(rpm_nxt) * nxt_ratio * FINAL_DRIVE
            if tw_nxt > tw_cur * 1.03:
                should_up = True
        elif rpm_cur >= REDLINE_RPM - 50.0:
            should_up = True   # redline forced

    # --- B. Downshift ---
    if g > FIRST_FWD:
        base_down = SHIFT_MAP[g][1]
        target_down = base_down + 3500.0 * brake
        lower_ratio = GEAR_RATIOS[g - 1]
        rpm_lower = state.wheel_omega * lower_ratio * FINAL_DRIVE * RAD_S_TO_RPM
        max_safe = REDLINE_RPM - 500.0
        kickdown = (throttle > 0.9 and state.engine_rpm < REDLINE_RPM * 0.5)
        # CURRENT BUG: kickdown bypasses rpm_lower < max_safe safety
        if (state.engine_rpm < target_down and rpm_lower < max_safe) or kickdown:
            should_down = True

    if should_up:
        return +1
    if should_down:
        return -1
    return None


def policy_fixed(state: VehicleState, throttle: float, brake: float) -> int | None:
    """GT3-style policy that survives wheelspin.

    Core insight: when traction is broken, engine RPM hits redline while the
    car is barely moving. Upshifting drops engine to ground-speed-implied RPM
    in the new gear. If that's below the downshift threshold, kickdown fires
    immediately -> 1<->2 oscillation forever.

    Fix:
      1. Upshift trigger: engine_rpm >= up_threshold AND
         ground_omega-derived predicted next-gear RPM >= max(IdleRPM*1.8, downshift+500).
         If predicted RPM would be below downshift threshold, *refuse* to upshift
         (we'd just bounce back). Sit at redline + ECU rev limiter does its job.
      2. Use ground speed (not wheel omega) for prediction, so wheelspin doesn't
         lie to us.
      3. Downshift kickdown subject to UNCONDITIONAL lower-gear safety check.
      4. Post-shift hysteresis 0.6s (>= shift duration + RevMatch + settling).
      5. Don't downshift while still under power if ground-speed predicted RPM
         in the lower gear would exceed redline-500.
    """
    if state.is_shifting or state.hyst_timer > 0.0:
        return None
    g = state.gear
    if g < FIRST_FWD:
        return None

    # Ground-speed-implied wheel omega -- ignores wheelspin.
    ground_omega = state.speed_ms / WHEEL_RADIUS

    should_up = False
    should_down = False

    # --- A. Upshift ---
    if g < LAST_GEAR and throttle > 0.5:
        up_rpm = SHIFT_MAP[g][0]
        nxt_down_rpm = SHIFT_MAP[g + 1][1]
        nxt_ratio = abs(GEAR_RATIOS[g + 1])
        # Predicted RPM after upshift, using GROUND speed (post-RevMatch reality)
        rpm_nxt_predicted = ground_omega * nxt_ratio * FINAL_DRIVE * RAD_S_TO_RPM
        # Required margin: must clear next gear's downshift trigger by at least 500 RPM
        min_required = max(IDLE_RPM * 1.8, nxt_down_rpm + 500.0)
        if state.engine_rpm >= up_rpm and rpm_nxt_predicted >= min_required:
            should_up = True

    # --- B. Downshift ---
    if g > FIRST_FWD:
        base_down = SHIFT_MAP[g][1]
        target_down = base_down + 3500.0 * brake
        lower_ratio = abs(GEAR_RATIOS[g - 1])
        # Ground-speed predicted RPM in lower gear
        rpm_lower_predicted = ground_omega * lower_ratio * FINAL_DRIVE * RAD_S_TO_RPM
        max_safe = REDLINE_RPM - 500.0
        kickdown = (throttle > 0.9 and state.engine_rpm < REDLINE_RPM * 0.5)

        # SAFETY: applies unconditionally
        if rpm_lower_predicted < max_safe:
            if state.engine_rpm < target_down or kickdown:
                should_down = True

    if should_up:
        return +1
    if should_down:
        return -1
    return None


def step(state: VehicleState, throttle: float, brake: float, dt: float, policy) -> None:
    # --- Shift decision ---
    delta = policy(state, throttle, brake)
    if delta is not None:
        new_target = state.gear + delta
        if 0 <= new_target < len(GEAR_RATIOS) and new_target != state.gear:
            state.target_gear = new_target

    # --- Shift state machine ---
    if state.target_gear != state.gear and state.shift_phase == "None":
        state.shift_phase = "ClutchRelease"
        state.shift_timer = 0.030
        state.is_shifting = True
        state.clutch_pos = 0.0
        state.log.append(f"t={state.t:.3f} SHIFT START {GEAR_NAMES[state.gear]}->{GEAR_NAMES[state.target_gear]} rpm={state.engine_rpm:.0f} v={state.speed_ms*3.6:.1f}km/h")

    if state.shift_phase == "ClutchRelease":
        state.shift_timer -= dt
        state.clutch_pos = 0.0
        if state.shift_timer <= 0.0:
            state.gear = state.target_gear
            state.shift_phase = "GearSwap"
            state.shift_timer = max(0.005, SHIFT_TIME)
    elif state.shift_phase == "GearSwap":
        state.shift_timer -= dt
        state.clutch_pos = 0.0
        if state.shift_timer <= 0.0:
            state.shift_phase = "RevMatch"
            state.shift_timer = 0.20
    elif state.shift_phase == "RevMatch":
        state.shift_timer -= dt
        combined = abs(GEAR_RATIOS[state.gear]) * FINAL_DRIVE
        target_engine = abs(state.wheel_omega) * combined * RAD_S_TO_RPM
        target_engine = max(IDLE_RPM, min(REDLINE_RPM, target_engine))
        # FInterpTo strength=25 -> tau ~ 1/25 = 40ms
        alpha = 1.0 - math.exp(-25.0 * dt)
        state.engine_rpm += (target_engine - state.engine_rpm) * alpha
        state.clutch_pos = min(1.0, state.clutch_pos + dt * 5.0)
        slip = abs(state.engine_rpm - target_engine)
        if (slip < 200.0 and state.clutch_pos >= 0.95) or state.shift_timer <= 0.0:
            state.shift_phase = "None"
            state.is_shifting = False
            state.clutch_pos = 1.0
            state.hyst_timer = 0.6     # FIX: 600ms post-shift lockout
            state.log.append(f"t={state.t:.3f} SHIFT END   gear={GEAR_NAMES[state.gear]} rpm={state.engine_rpm:.0f}")

    # --- Engine torque -> driveline ---
    combined_ratio = abs(GEAR_RATIOS[state.gear]) * FINAL_DRIVE
    if state.is_shifting and state.shift_phase != "RevMatch":
        # Throttle cut + clutch open: engine free-decays
        eng_tq = -10.0   # friction
        wheel_drive_tq = 0.0
    else:
        eng_tq = throttle * sample_torque(state.engine_rpm) * state.clutch_pos
        wheel_drive_tq = eng_tq * combined_ratio   # at the wheel

    # Simple wheel: drive force = drive_torque / radius, capped by tire grip.
    fx_demanded = wheel_drive_tq / WHEEL_RADIUS
    fx_actual = max(-TIRE_FX_MAX, min(TIRE_FX_MAX, fx_demanded))
    # Wheelspin slip: if demanded > available, excess accelerates the wheels.
    slip_torque = (fx_demanded - fx_actual) * WHEEL_RADIUS

    # --- Vehicle longitudinal ---
    drag = DRAG_COEFF * state.speed_ms * state.speed_ms
    rolling = ROLLING_RES if state.speed_ms > 0.1 else 0.0
    brake_force = brake * 8000.0
    net_force = fx_actual - drag - rolling - brake_force
    state.speed_ms += (net_force / VEHICLE_MASS) * dt
    state.speed_ms = max(0.0, state.speed_ms)
    ground_omega = state.speed_ms / WHEEL_RADIUS

    # --- Wheel omega ---
    # Driven wheels in GRIT: when shifting (decoupled), they slow rapidly via
    # drag toward ground_omega. When clutch-locked, wheel = engine/ratio with
    # spin excess if tire is over the grip limit.
    if state.is_shifting:
        # Decoupled: collapse fast toward ground speed (the bug-trigger)
        state.wheel_omega = ground_omega + (state.wheel_omega - ground_omega) * math.exp(-15.0 * dt)
    else:
        target_wheel_omega = state.engine_rpm * RPM_TO_RAD_S / combined_ratio
        if abs(fx_demanded) > TIRE_FX_MAX:
            wheel_accel = slip_torque / (DRIVE_INERTIA * WHEEL_RADIUS) * 0.05
            state.wheel_omega = max(ground_omega, state.wheel_omega + wheel_accel * dt)
            state.wheel_omega = max(state.wheel_omega, target_wheel_omega * 0.95)
        else:
            state.wheel_omega = ground_omega + max(0.0, target_wheel_omega - ground_omega) * 0.95

    # --- Engine RPM update (skip during RevMatch; that's handled above) ---
    if state.shift_phase != "RevMatch":
        if state.is_shifting:
            # Free decay
            state.engine_rpm -= 800.0 * dt    # ~ engine friction drag
            state.engine_rpm = max(IDLE_RPM, state.engine_rpm)
        else:
            # Engine pulled toward wheel*ratio via clutch (locked)
            target = state.wheel_omega * combined_ratio * RAD_S_TO_RPM
            target = max(IDLE_RPM, min(REDLINE_RPM, target))
            # If throttle is on and torque can sustain higher RPM, integrate up.
            net_eng_tq = eng_tq - 10.0
            if state.clutch_pos > 0.99:
                state.engine_rpm = target  # locked
            else:
                state.engine_rpm += (net_eng_tq / ENGINE_INERTIA) * RAD_S_TO_RPM * dt
                state.engine_rpm = max(IDLE_RPM, min(REDLINE_RPM, state.engine_rpm))

    # --- Timers ---
    if state.hyst_timer > 0.0:
        state.hyst_timer = max(0.0, state.hyst_timer - dt)
    state.t += dt


def run_sim(policy, label: str, duration: float = 30.0, dt: float = 1.0/240.0):
    state = VehicleState()
    sample_t = 0.0
    samples: List[Tuple[float, float, float, int, str]] = []
    while state.t < duration:
        # Full-throttle launch from rest, no brake.
        step(state, throttle=1.0, brake=0.0, dt=dt, policy=policy)
        if state.t - sample_t >= 0.05:
            samples.append((state.t, state.engine_rpm, state.speed_ms * 3.6, state.gear, state.shift_phase))
            sample_t = state.t

    print(f"\n=== {label} ===")
    print("Transitions:")
    for line in state.log[:40]:
        print("  " + line)
    print(f"\nFinal: t={state.t:.2f}s gear={GEAR_NAMES[state.gear]} rpm={state.engine_rpm:.0f} v={state.speed_ms*3.6:.1f}km/h")
    print(f"Total shifts: {len(state.log) // 2}")
    return state


if __name__ == "__main__":
    print("Simulating 30-second full-throttle launch on low-grip surface...")
    run_sim(policy_current, "CURRENT POLICY (with kickdown bug)")
    run_sim(policy_fixed,   "FIXED POLICY (kickdown safety + RPM-based upshift)")
