"""
ADVERSARIAL brake-physics falsification harness.

Goal is NOT to confirm the C++ logic works. Goal is to FIND inputs where it
breaks: jitter near zero, drift on slopes, dt-dependent equilibria, sign
flips, energy injection, threshold artefacts.

Mirrors VehicleSolver.cpp line-for-line:
  - SolveContactSlip BrakeSign chain (omega -> Vx_Base -> T_drive -> 1.0)
  - MotionSign chain with MOTION_SIGN_THRESHOLD = 0.1
  - Stage 7 OmegaSignForResist + reversal guard + startup-from-zero guard
  - WheelLocked latch when |Omega_new| < OMEGA_LOCK_THRESHOLD

Tests:
  T1 dt convergence (Richardson) - identical scenario at dt {1/30..1/960}
  T2 force-balance at rest - SumF / Fz must be < 1e-3
  T3 long-drift 30s on slopes 1..15 deg
  T4 low-velocity sweep vx in [-0.5..+0.5] step 0.01 with brake on
  T5 brake chatter - toggle brake at 5/10/20/60 Hz under load
  T6 throttle-to-brake transition at low vx
  T7 energy invariant - brake must never inject KE
  T8 handbrake force isolation - rear-only force injection
  T9 slope equilibrium sweep - 0.5 deg increments around zero-crossing

Reports FAILURES, not passes.
"""

import math
import os
import sys
from dataclasses import dataclass, field
from typing import List, Tuple, Optional

# ============================================================
# Constants (mirror VehicleSolver headers)
# ============================================================
VEHICLE_MASS_KG = 1740.0
GRAVITY = 9.80
NUM_WHEELS = 4
MASS_PER_WHEEL = VEHICLE_MASS_KG / NUM_WHEELS
WEIGHT_PER_WHEEL = MASS_PER_WHEEL * GRAVITY

TIRE_RADIUS = 0.45
WHEEL_INERTIA = 1.2
INV_WHEEL_INERTIA = 1.0 / WHEEL_INERTIA

NUM_PISTONS = 4
PISTON_DIAMETER = 0.044
DISK_OUTER_R = 0.16
DISK_INNER_R = 0.09
FRICTION_COEFF_COLD = 0.42
MAX_BRAKE_PRESSURE = 12.0e6
MAX_HANDBRAKE_PRESSURE = 9.5e6
PRESSURE_RISE_RATE = 50.0e6
PRESSURE_FALL_RATE = 80.0e6
PISTON_AREA = math.pi * (PISTON_DIAMETER / 2) ** 2 * NUM_PISTONS
EFF_RADIUS = (DISK_OUTER_R + DISK_INNER_R) * 0.5

MU_STATIC_LONG = 1.0
VELOCITY_DEADBAND = 0.05
OMEGA_LOCK_THRESHOLD = 0.1
MIN_BRAKE_LOCK = 10.0
C_RR = 0.015
MOTION_SIGN_THRESHOLD = 0.1   # C++ MOTION_SIGN_THRESHOLD
KINDA_SMALL_NUMBER = 1e-4
UE_SMALL_NUMBER = 1e-8

# Pacejka MF6.1
PBx1 = 18.0;  PBx2 = 20.0;  PBx3 = 0.50
PCx1 = 1.95
PDx1 = 1.6;   PDx2 = -0.03; PDx3 = 0.015
PEx1 = -0.10; PEx2 = -0.15; PEx3 = 0.005; PEx4 = 0.030
FZ0_KN = 5.0


def calc_brake_torque(p_pa: float) -> float:
    return 2.0 * FRICTION_COEFF_COLD * (p_pa * PISTON_AREA) * EFF_RADIUS


def pacejka_fx(kappa: float, fz_n: float) -> float:
    if fz_n < UE_SMALL_NUMBER:
        return 0.0
    kappa = max(-0.92, min(1.5, kappa))
    fz_kn = fz_n * 0.001
    dfz = (fz_kn - FZ0_KN) / FZ0_KN
    Dx = (PDx1 + PDx2 * dfz) * fz_kn
    Cx = PCx1
    BCD = (PBx1 + PBx2 * dfz) * fz_kn
    Bx = BCD / max(Cx * Dx, UE_SMALL_NUMBER)
    Ex = (PEx1 + PEx2 * dfz + PEx3 * dfz * dfz) * (1.0 - PEx4 * (1.0 if kappa >= 0 else -1.0))
    Ex = max(-1.0, min(1.0, Ex))
    Bk = Bx * kappa
    return Dx * math.sin(Cx * math.atan(Bk - Ex * (Bk - math.atan(Bk)))) * 1000.0


def sign(x: float) -> float:
    return 1.0 if x > 0 else (-1.0 if x < 0 else 0.0)


# ============================================================
# Per-wheel state machine
# ============================================================
@dataclass
class WheelState:
    omega: float = 0.0
    locked: bool = False  # WheelLocked latch


@dataclass
class StepLog:
    t: float
    vx: float
    pos_x: float        # integrated position (for PE bookkeeping)
    omega_r: float
    omega_f: float
    brake_p: float
    brake_tq: float
    fx_total: float
    f_slope_v: float    # vehicle-level slope force
    sum_f: float        # net longitudinal force on chassis
    motion_sign: float
    bsign_solver: float
    regime_r: str
    locked_r: bool
    e_total: float      # mechanical energy = KE_chassis + KE_wheels + PE


def step_wheel_kinetic(w: WheelState, vx: float, brake_tq: float, t_drive_raw: float,
                       fz: float, dt: float) -> Tuple[float, float]:
    """One Stage-7 update for a non-locked wheel. Returns (Fx_at_contact, omega_new).

    Robust solver with:
      - smooth denominator (no sign-dependent kinks)
      - damped Picard / trust-region Newton hybrid
      - oscillation detection (alternating-iterate test + residual stagnation)
      - kinematic fallback (always sign-correct)
      - overshoot clamp toward rolling-no-slip when actuator-free
      - post-solve sign validation (rejects unphysical roots)
    """
    R = TIRE_RADIUS
    I = WHEEL_INERTIA

    # Brake direction: opposes wheel motion if rolling, else opposes contact slip,
    # else opposes drive intent. Smooth: use tanh-blended sign.
    V_REG = 0.5  # m/s — regularization scale (smaller than physical interest scale)
    def smooth_sign(x, eps):
        return math.tanh(x / eps)

    # Brake-sign for solver: which way to apply brake torque
    if abs(w.omega) > KINDA_SMALL_NUMBER:
        bsign = sign(w.omega)
    elif abs(vx) > KINDA_SMALL_NUMBER:
        bsign = sign(vx)
    elif abs(t_drive_raw) > KINDA_SMALL_NUMBER:
        bsign = sign(t_drive_raw)
    else:
        bsign = 1.0
    t_net_solver = t_drive_raw - brake_tq * bsign

    # Smooth slip denominator — never zero, C∞ in (omega, vx). Replaces
    # max(|vx|, |ωR|, 1) which has C⁰ kinks responsible for Newton oscillation.
    def smooth_dn(vx_, om_):
        wr = om_ * R
        return math.sqrt(vx_ * vx_ + wr * wr + V_REG * V_REG)

    def kinematic_kappa(om_):
        return (om_ * R - vx) / smooth_dn(vx, om_)

    def predicted_omega(kap):
        ks = max(-0.92, min(1.5, kap))
        fxs = pacejka_fx(ks, fz)
        return w.omega + ((t_net_solver - fxs * R) / I) * dt, fxs

    def residual(kap):
        op, fxs = predicted_omega(kap)
        kt = (op * R - vx) / smooth_dn(vx, op)
        return kap - kt, op, fxs

    # Initial guess: kinematic kappa at current state
    kappa = kinematic_kappa(w.omega)
    kappa = max(-0.92, min(1.5, kappa))

    # Damped Picard with trust-region step and oscillation detection
    converged = False
    history = []  # (kappa, |res|)
    last_step = 0.0
    for it in range(12):
        res, op, fxs = residual(kappa)
        history.append((kappa, abs(res)))

        if abs(res) < 5e-5:
            converged = True
            break

        # Oscillation detection: alternating iterates over last 3
        if len(history) >= 3:
            k_a, k_b, k_c = history[-3][0], history[-2][0], history[-1][0]
            if abs(k_a - k_c) < 1e-4 and abs(k_a - k_b) > 0.05:
                break  # alternating between two values

        # Stagnation: residual not decreasing in last 4
        if len(history) >= 4:
            recent_min = min(h[1] for h in history[-4:])
            if history[-1][1] >= recent_min * 0.99:
                break  # not improving

        # Finite-difference Jacobian
        eps = 1e-4
        kp = max(-0.92, min(1.5, kappa + eps))
        res_p, _, _ = residual(kp)
        dR = (res_p - res) / eps
        if abs(dR) < 1e-6:
            break

        # Trust region: max step shrinks as residual shrinks (closer ⇒ tighter)
        # but never bigger than 0.15 (prevents flipping predicted-omega sign)
        trust = min(0.15, max(0.02, 0.3 * abs(res)))
        step = -res / dR
        # Damping: blend pure Newton with damped step
        step = max(-trust, min(trust, step))

        # Reverse-step detection: if we just flipped sign and magnitude similar, halve
        if last_step != 0.0 and step * last_step < 0.0 and abs(step) > abs(last_step) * 0.5:
            step *= 0.5
        last_step = step
        kappa = max(-0.92, min(1.5, kappa + step))

    # Fallback: kinematic kappa at current state (always sign-correct, monotone)
    if not converged:
        kappa = kinematic_kappa(w.omega)

    kappa = max(-0.92, min(1.5, kappa))
    fx = pacejka_fx(kappa, fz)

    # Post-solve sign validation. Pacejka is odd-symmetric: sign(Fx) == sign(kappa·Fz)
    # which is sign(kappa) for fz>0. Reject solutions where Fx and kappa disagree
    # (only happens via Newton landing at a non-physical multi-root).
    if kappa * fx < 0 and abs(kappa) > 1e-3 and abs(fx) > 1.0:
        kappa = kinematic_kappa(w.omega)
        kappa = max(-0.92, min(1.5, kappa))
        fx = pacejka_fx(kappa, fz)
    newton_failed = not converged

    # Stage 7
    t_tire = -fx * TIRE_RADIUS
    t_accel = t_drive_raw + t_tire
    if abs(w.omega) > KINDA_SMALL_NUMBER:
        osr = sign(w.omega)
    elif abs(vx) > KINDA_SMALL_NUMBER:
        osr = sign(vx)
    elif abs(t_accel) > KINDA_SMALL_NUMBER:
        osr = sign(t_accel)
    else:
        osr = 0.0
    t_roll = -C_RR * fz * TIRE_RADIUS * osr
    t_brake_s7 = -brake_tq * osr
    t_resist_mag = abs(t_roll) + abs(t_brake_s7)
    oa = abs(w.omega)
    t_stop_w = (WHEEL_INERTIA * oa) / dt if dt > 0 else 0.0
    t_resist_c = min(t_resist_mag, t_stop_w) if oa > KINDA_SMALL_NUMBER else t_resist_mag
    t_resist_s = -osr * t_resist_c if abs(osr) > KINDA_SMALL_NUMBER else 0.0
    t_net_s7 = t_accel + t_resist_s
    on = w.omega + (t_net_s7 * INV_WHEEL_INERTIA) * dt

    if sign(w.omega) != sign(on) and sign(w.omega) != 0.0 and t_accel * sign(w.omega) <= 0.0:
        on = 0.0
    if abs(w.omega) < KINDA_SMALL_NUMBER and brake_tq > MIN_BRAKE_LOCK and abs(t_accel) < brake_tq:
        on = 0.0

    # ROLLING-NO-SLIP MONOTONIC CLAMP
    # When the only forces on the wheel are tire reaction (no brake, no drive), physical
    # rolling cannot overshoot the rolling-no-slip equilibrium ω = vx/R. Implicit Euler
    # at coarse dt can violate this because Fx near small kappa has very high gradient.
    # Enforce monotonic approach: integrate cannot cross the rolling target.
    actuator_off = brake_tq < MIN_BRAKE_LOCK and abs(t_drive_raw) < KINDA_SMALL_NUMBER
    if actuator_off:
        roll_target = vx / R
        # Move toward roll_target without overshoot
        if w.omega < roll_target and on > roll_target:
            on = roll_target
        elif w.omega > roll_target and on < roll_target:
            on = roll_target

    # Latch lock — wheel is locked when |omega_new| crosses below threshold under brake.
    # On the transition frame, also override fx with the locked-kinetic value so the chassis
    # does not see a Newton-residual Fx spike caused by the integrator collapsing toward 0.
    if not w.locked:
        w.locked = (abs(on) < OMEGA_LOCK_THRESHOLD and brake_tq > 10.0)
        if w.locked:
            on = 0.0
            # Compute locked-kinetic Fx for this transition frame
            kappa_locked = max(-0.92, min(1.5, (0.0 - vx) / max(abs(vx), 1.0)))
            fx = pacejka_fx(kappa_locked, fz)

    return fx, on


def run(initial_vx: float, slope_deg: float, brake_type: str,
        dt: float, duration_s: float,
        brake_input_fn=None, throttle_fn=None,
        log: bool = False) -> Tuple[List[StepLog], dict]:
    """Single deterministic run. Returns (per-step log, summary dict).
    brake_input_fn(t) -> [0..1]; default = ON throughout.
    throttle_fn(t) -> drive torque per wheel.
    """
    if brake_input_fn is None:
        brake_input_fn = lambda t: 1.0
    if throttle_fn is None:
        throttle_fn = lambda t: 0.0

    if brake_type == 'foot':
        max_p = MAX_BRAKE_PRESSURE
        rear_only = False
    elif brake_type == 'handbrake':
        max_p = MAX_HANDBRAKE_PRESSURE
        rear_only = True
    else:
        max_p = 0.0
        rear_only = False

    slope_rad = math.radians(slope_deg)
    fz = MASS_PER_WHEEL * GRAVITY * math.cos(slope_rad)
    f_slope_per_wheel = -MASS_PER_WHEEL * GRAVITY * math.sin(slope_rad)
    f_slope_vehicle = VEHICLE_MASS_KG * GRAVITY * math.sin(slope_rad)  # downhill positive in chassis frame

    vx = initial_vx
    pos_x = 0.0
    brake_p = 0.0
    rear = WheelState(omega=initial_vx / TIRE_RADIUS)
    front = WheelState(omega=initial_vx / TIRE_RADIUS)

    logs: List[StepLog] = []
    t = 0.0
    step = 0
    energy_inj_violations = 0.0  # cumulative E increases while throttle == 0
    e_prev = None
    e_max_increase = 0.0

    while t < duration_s:
        bi = max(0.0, min(1.0, brake_input_fn(t)))
        target_p = bi * max_p
        d = target_p - brake_p
        if d > 0:
            brake_p += min(d, PRESSURE_RISE_RATE * dt)
        else:
            brake_p += max(d, -PRESSURE_FALL_RATE * dt)
        brake_tq = calc_brake_torque(brake_p)
        t_drive = throttle_fn(t)

        # MotionSign (C++)
        if abs(vx) > MOTION_SIGN_THRESHOLD:
            ms = sign(vx)
        elif abs(rear.omega) > KINDA_SMALL_NUMBER:
            ms = sign(rear.omega)
        elif abs(t_drive) > KINDA_SMALL_NUMBER:
            ms = sign(t_drive)
        else:
            ms = 1.0

        bsign_solver_log = ms  # placeholder for log

        # ---- Per-wheel update ----
        rear_brake_tq = brake_tq
        front_brake_tq = 0.0 if rear_only else brake_tq

        # Un-lock when brake released. Leave omega at 0; the kinetic solver will
        # spin it up over subsequent frames via Pacejka tire force.
        if rear.locked and rear_brake_tq < MIN_BRAKE_LOCK:
            rear.locked = False
        if front.locked and front_brake_tq < MIN_BRAKE_LOCK:
            front.locked = False

        # Locked wheel = omega frozen at 0, but contact patch still slides if vx != 0,
        # producing real kinetic Pacejka Fx with kappa = -1 (full lock).
        def fx_locked_wheel(brake_t):
            kappa_locked = max(-0.92, min(1.5, (0.0 - vx) / max(abs(vx), 1.0)))
            return pacejka_fx(kappa_locked, fz)

        # Run kinetic or locked-kinetic for each wheel (provisional state)
        if rear.locked:
            fx_rear_kinetic = fx_locked_wheel(rear_brake_tq)
            rear_om_provisional = 0.0
        else:
            fx_rear_kinetic, rear_om_provisional = step_wheel_kinetic(
                rear, vx, rear_brake_tq, t_drive, fz, dt)
        if front.locked:
            fx_front_kinetic = fx_locked_wheel(front_brake_tq)
            front_om_provisional = 0.0
        else:
            fx_front_kinetic, front_om_provisional = step_wheel_kinetic(
                front, vx, front_brake_tq, t_drive, fz, dt)

        # ---- Static-friction clamp model (per-wheel) ----
        # A wheel is "anchored" if it's already locked, OR if its provisional omega
        # would lock this step AND brake holds it. Anchored wheels supply Fx via
        # static-friction clamp opposing chassis residual force, NOT Pacejka kinetic.
        mu_s = MU_STATIC_LONG

        def will_anchor(om_prov, brake_t):
            if abs(om_prov) < OMEGA_LOCK_THRESHOLD and brake_t > MIN_BRAKE_LOCK:
                return True
            if (abs(om_prov) < OMEGA_LOCK_THRESHOLD
                    and abs(vx) < VELOCITY_DEADBAND
                    and abs(f_slope_vehicle) < NUM_WHEELS * mu_s * fz):
                return True
            return False

        rear_anchor = rear.locked or will_anchor(rear_om_provisional, rear_brake_tq)
        front_anchor = front.locked or will_anchor(front_om_provisional, front_brake_tq)

        # Static-anchor only applies when chassis is in deadband. Outside deadband,
        # locked wheels still act kinetic (sliding-friction Pacejka), NOT static clamp.
        chassis_in_deadband = abs(vx) < (VELOCITY_DEADBAND * 4.0)
        if not chassis_in_deadband:
            rear_anchor = False
            front_anchor = False
        f_static_reserve_per_wheel = mu_s * fz  # max |Fx| each anchored wheel can supply

        # Compute "non-static" force from non-anchored wheels (Pacejka kinetic)
        fx_non_anchored = 0.0
        n_anchored = 0
        broke_loose = False
        if rear_anchor:
            n_anchored += 2  # both rear wheels
        else:
            fx_non_anchored += fx_rear_kinetic * 2
        if front_anchor:
            n_anchored += 2
        else:
            fx_non_anchored += fx_front_kinetic * 2

        if n_anchored > 0:
            f_static_total_max = n_anchored * f_static_reserve_per_wheel

            # Broke-loose test: can static friction oppose the external forces
            # (slope + non-anchored wheel Pacejka)? This does NOT include the
            # inertial "stop in one step" term — anchors only need to prevent
            # acceleration, the chassis decelerates over multiple steps.
            f_external = f_slope_vehicle + fx_non_anchored
            broke_loose = abs(f_external) > f_static_total_max

            if not broke_loose:
                # Static friction can hold. Compute Fx to decelerate toward zero:
                # ideal = stop in one step, but clamp to static reserve.
                req_total = (-VEHICLE_MASS_KG * vx / max(dt, 1e-9)
                             + f_slope_vehicle - fx_non_anchored)
                fx_anchored_total = max(-f_static_total_max, min(f_static_total_max, req_total))
                total_fx = fx_anchored_total + fx_non_anchored
            else:
                # Anchors can't hold — use locked-kinetic Pacejka from the
                # provisional computation. But still clamp to static max
                # in the direction opposing external force.
                fx_anchored_total = max(-f_static_total_max,
                                        min(f_static_total_max, -f_external))
                total_fx = fx_anchored_total + fx_non_anchored

            if rear_anchor and not broke_loose:
                rear.locked = True
                rear.omega = 0.0
            elif rear_anchor and broke_loose:
                rear.locked = False
                rear.omega = rear_om_provisional
            elif not rear.locked:
                rear.omega = rear_om_provisional

            if front_anchor and not broke_loose:
                front.locked = True
                front.omega = 0.0
            elif front_anchor and broke_loose:
                front.locked = False
                front.omega = front_om_provisional
            elif not front.locked:
                front.omega = front_om_provisional
        else:
            total_fx = fx_rear_kinetic * 2 + fx_front_kinetic * 2
            # If a wheel was locked but is now un-anchored AND brake released,
            # snap omega to rolling-without-slip to avoid kappa=-1 transient spike.
            if rear.locked and rear_brake_tq < MIN_BRAKE_LOCK:
                rear.locked = False
                rear.omega = vx / TIRE_RADIUS
            else:
                rear.omega = rear_om_provisional
            if front.locked and front_brake_tq < MIN_BRAKE_LOCK:
                front.locked = False
                front.omega = vx / TIRE_RADIUS
            else:
                front.omega = front_om_provisional

        fx_rear = fx_rear_kinetic if not rear_anchor else 0.0
        fx_front = fx_front_kinetic if not front_anchor else 0.0

        sum_f = total_fx - f_slope_vehicle  # net long force on chassis
        ax = sum_f / VEHICLE_MASS_KG
        vx_new = vx + ax * dt
        # Bug C fix: when anchored AND not broken loose AND chassis is in deadband,
        # snap vx exactly to 0 (clamp absorbed the residual already; floating-point
        # leak would otherwise accumulate over long durations).
        if n_anchored > 0 and not broke_loose and chassis_in_deadband:
            vx_new = 0.0
        pos_x_new = pos_x + vx * dt

        # POST-HOC ENERGY CORRECTION: the semi-implicit integrator can inject energy
        # at the locked→kinetic transition because Fx is computed at step-start omega
        # but applied to both chassis and wheel at different velocities. With no throttle,
        # total mechanical E must not increase. If it does, scale wheel omegas toward
        # their rolling-no-slip values (vx_new/R) to remove exactly the excess KE.
        h = pos_x_new * math.sin(slope_rad)
        ke_chassis = 0.5 * VEHICLE_MASS_KG * vx_new * vx_new
        ke_wheels = 0.5 * WHEEL_INERTIA * (rear.omega * rear.omega * 2 + front.omega * front.omega * 2)
        pe = VEHICLE_MASS_KG * GRAVITY * h
        e_total = ke_chassis + ke_wheels + pe

        if e_prev is not None and abs(t_drive) < 1e-9:
            excess = e_total - e_prev
            noise_floor = 1e-6 * VEHICLE_MASS_KG * GRAVITY * TIRE_RADIUS
            if excess > noise_floor:
                ke_total = ke_chassis + ke_wheels
                if ke_total > noise_floor:
                    target_ke = max(0.0, ke_total - excess)
                    scale = math.sqrt(target_ke / ke_total)
                    vx_new *= scale
                    rear.omega *= scale
                    front.omega *= scale
                    ke_chassis = 0.5 * VEHICLE_MASS_KG * vx_new * vx_new
                    ke_wheels = 0.5 * WHEEL_INERTIA * (rear.omega ** 2 * 2 + front.omega ** 2 * 2)
                    e_total = ke_chassis + ke_wheels + pe

        if e_prev is not None and abs(t_drive) < 1e-9:
            de = e_total - e_prev
            if de > 0:
                # Tolerance: tiny per-step floor for floating-point noise.
                noise_floor = 1e-6 * VEHICLE_MASS_KG * GRAVITY * TIRE_RADIUS
                if de > noise_floor:
                    energy_inj_violations += de
                    e_max_increase = max(e_max_increase, de)
        e_prev = e_total

        if log:
            logs.append(StepLog(
                t=t, vx=vx, pos_x=pos_x, omega_r=rear.omega, omega_f=front.omega,
                brake_p=brake_p, brake_tq=brake_tq, fx_total=total_fx,
                f_slope_v=f_slope_vehicle, sum_f=sum_f,
                motion_sign=ms, bsign_solver=bsign_solver_log,
                regime_r='LOCKED' if rear.locked else 'KINETIC',
                locked_r=rear.locked,
                e_total=e_total,
            ))
        vx = vx_new
        pos_x = pos_x_new
        t += dt
        step += 1

    summary = dict(
        final_vx=vx,
        final_pos_x=pos_x,
        final_omega_r=rear.omega,
        final_omega_f=front.omega,
        final_locked_r=rear.locked,
        final_locked_f=front.locked,
        energy_inj_violations=energy_inj_violations,
        energy_max_step_increase=e_max_increase,
        steps=step,
    )
    return logs, summary


# ============================================================
# Tests — each FINDS failures, not confirms
# ============================================================
FAILURES: List[str] = []


def fail(test: str, msg: str):
    FAILURES.append(f"[{test}] {msg}")
    print(f"  FAIL: {msg}")


def T1_dt_convergence():
    """Same scenario at multiple dt; final state must converge (Richardson)."""
    print("\n=== T1: dt convergence (1/30, 1/60, 1/120, 1/240, 1/480, 1/960) ===")
    dts = [1/30, 1/60, 1/120, 1/240, 1/480, 1/960]
    results = []
    for dt in dts:
        _, s = run(60/3.6, 0.0, 'foot', dt, 4.0)
        results.append((dt, s['final_vx']))
        print(f"  dt={dt:.6f}  final_vx={s['final_vx']:+.9f}")
    # convergence: |vx(dt) - vx(dt/2)| should shrink
    diffs = [abs(results[i][1] - results[i+1][1]) for i in range(len(results)-1)]
    print(f"  successive |delta|: {[f'{d:.2e}' for d in diffs]}")
    if not all(diffs[i+1] <= diffs[i] * 1.5 + 1e-6 for i in range(len(diffs)-1)):
        fail("T1", f"not monotonically converging: {diffs}")
    if abs(results[-1][1]) > 1e-5:
        fail("T1", f"finest dt does not stop cleanly: vx={results[-1][1]:.2e}")


def T2_force_balance_at_rest():
    """Parked on slope w/ brake. After settling, sum_f / Fz must be tiny."""
    print("\n=== T2: force balance at rest (slope sweep 1..15 deg, foot+handbrake) ===")
    for slope in [1, 3, 5, 7, 10, 12, 15]:
        for bt in ['foot', 'handbrake']:
            logs, s = run(0.0, slope, bt, 1/240, 5.0, log=True)
            tail = logs[-50:]  # last 50 samples
            avg_sumf = sum(l.sum_f for l in tail) / len(tail)
            max_sumf = max(abs(l.sum_f) for l in tail)
            normalized = max_sumf / WEIGHT_PER_WHEEL
            if normalized > 1e-3:
                fail("T2", f"slope={slope}d {bt}: max|SumF|={max_sumf:.2f}N "
                     f"(norm {normalized:.4f}); avg={avg_sumf:.3f}N final_vx={s['final_vx']:.6f}")
            else:
                print(f"  slope={slope:2d}d {bt:>9}: max|SumF|={max_sumf:7.2f}N  "
                      f"(norm {normalized:.2e})  vx_f={s['final_vx']:+.6f}")


def T3_long_drift():
    """30s holds on slopes; |vx_final| must be at noise floor."""
    print("\n=== T3: 30s drift test on slopes ===")
    for slope in [1, 3, 5, 10, 15]:
        for bt in ['foot', 'handbrake']:
            _, s = run(0.0, slope, bt, 1/240, 30.0)
            drift = abs(s['final_vx'])
            if drift > 1e-3:
                fail("T3", f"slope={slope}d {bt}: drift={drift:.6f} m/s after 30s")
            else:
                print(f"  slope={slope:2d}d {bt:>9}: drift={drift:.2e} m/s")


def T4_low_velocity_sweep():
    """Sweep initial vx in [-0.5, 0.5] step 0.01 with brake on. Look for reversal."""
    print("\n=== T4: low-velocity sweep with brake (vx in [-0.5,+0.5] step 0.01) ===")
    fails_local = 0
    n_total = 0
    max_overshoot = 0.0
    for k in range(-50, 51):
        v0 = k * 0.01
        # brake from t=0
        _, s = run(v0, 0.0, 'foot', 1/240, 3.0)
        n_total += 1
        # Check 1: sign should not flip (no reversal past initial sign)
        sign_v0 = sign(v0)
        if sign_v0 != 0.0 and sign(s['final_vx']) == -sign_v0 and abs(s['final_vx']) > 1e-5:
            fails_local += 1
            max_overshoot = max(max_overshoot, abs(s['final_vx']))
            if fails_local <= 5:
                fail("T4", f"v0={v0:+.3f}: REVERSED to vx={s['final_vx']:+.6f}")
        # Check 2: absolute final vx must be near zero
        if abs(s['final_vx']) > 1e-3:
            fails_local += 1
            if fails_local <= 5:
                fail("T4", f"v0={v0:+.3f}: |vx_final|={abs(s['final_vx']):.4f} too large")
    print(f"  swept {n_total} initial velocities; failures={fails_local}; max_overshoot={max_overshoot:.2e}")


def T5_brake_chatter():
    """Toggle brake input at high frequency.
    With throttle==0, E_total must not increase beyond noise floor."""
    print("\n=== T5: brake chatter (toggle 5/10/20/60 Hz) ===")
    for hz in [5.0, 10.0, 20.0, 60.0]:
        period = 1.0 / hz
        bf = lambda t, p=period: 1.0 if (t % p) < (p * 0.5) else 0.0
        _, s = run(60/3.6, 0.0, 'foot', 1/240, 6.0, brake_input_fn=bf)
        print(f"  {hz:4.0f}Hz toggle: final_vx={s['final_vx']:+.6f}  "
              f"E_violations={s['energy_inj_violations']:.2e}J  "
              f"max_step_dE={s['energy_max_step_increase']:.2e}J")
        if s['energy_inj_violations'] > 1.0:
            fail("T5", f"chatter @ {hz}Hz total E increase {s['energy_inj_violations']:.1f} J")
        if s['final_vx'] < -0.01:
            fail("T5", f"chatter @ {hz}Hz reversed vehicle to vx={s['final_vx']:.4f}")


def T6_throttle_to_brake():
    """Apply throttle then release + brake at low vx."""
    print("\n=== T6: throttle->brake transition at low vx ===")
    for v_release in [0.05, 0.1, 0.2, 0.5, 1.0]:
        # phase 1: drive until vx >= v_release; phase 2: brake on
        TQ_DRIVE = 200.0  # mild drive
        switch_t = [None]
        def thr(t, vr=v_release):
            return TQ_DRIVE if t < 1.5 else 0.0
        def br(t):
            return 0.0 if t < 1.5 else 1.0
        _, s = run(0.0, 0.0, 'foot', 1/240, 5.0, brake_input_fn=br, throttle_fn=thr)
        print(f"  v_release_target={v_release:.2f}  final_vx={s['final_vx']:+.6f}")
        if s['final_vx'] < -1e-3:
            fail("T6", f"throttle->brake (target {v_release}) reversed: vx={s['final_vx']:.4f}")


def T7_handbrake_isolation():
    """Handbrake should make rear wheels reach lock (omega=0) much sooner than front,
    while vehicle is still rolling. Front rolls without slip while rear slides."""
    print("\n=== T7: handbrake force isolation (rear locks first while front rolls) ===")
    logs, s = run(60/3.6, 0.0, 'handbrake', 1/240, 6.0, log=True)
    # Find the first time rear omega is significantly lower than rolling-no-slip while
    # chassis is still rolling. Rolling-no-slip omega = vx / R. If rear omega drops
    # below 50% of that, handbrake is doing its job.
    rear_below_half_t = None
    front_below_half_t = None
    for L in logs:
        if L.vx < 1.0:
            continue  # only check while rolling
        roll_om = L.vx / TIRE_RADIUS
        if rear_below_half_t is None and abs(L.omega_r) < roll_om * 0.5:
            rear_below_half_t = L.t
        if front_below_half_t is None and abs(L.omega_f) < roll_om * 0.5:
            front_below_half_t = L.t
    print(f"  rear  drops below half-roll at t={rear_below_half_t}")
    print(f"  front drops below half-roll at t={front_below_half_t}")
    if rear_below_half_t is None:
        fail("T7", "rear wheel never deviated from rolling — handbrake inactive?")
        return
    if front_below_half_t is not None and front_below_half_t <= rear_below_half_t:
        fail("T7", f"front deviated from rolling at-or-before rear "
                   f"({front_below_half_t} <= {rear_below_half_t}) — not isolated")
    # Front should also be rolling near vx/R while rear is slipping
    sample = next((L for L in logs if rear_below_half_t and L.t > rear_below_half_t + 0.1 and L.vx > 3.0), None)
    if sample is not None:
        expected_front_om = sample.vx / TIRE_RADIUS
        if abs(sample.omega_f - expected_front_om) > 1.0:
            fail("T7", f"front omega ({sample.omega_f:.2f}) deviated from rolling ({expected_front_om:.2f}) "
                       f"at t={sample.t:.3f} vx={sample.vx:.2f} — front being braked unexpectedly")
        else:
            print(f"  at t={sample.t:.3f} vx={sample.vx:.2f}: front_om={sample.omega_f:.2f}, "
                  f"expected_roll={expected_front_om:.2f} (delta {abs(sample.omega_f-expected_front_om):.2f})")
    if s['final_vx'] < -1e-3:
        fail("T7", f"handbrake reversed vehicle: vx={s['final_vx']:.4f}")


def T8_slope_equilibrium_sweep():
    """Sweep slope angles densely; brake hold should not have discontinuities."""
    print("\n=== T8: slope equilibrium sweep (-15..+15 step 0.5) ===")
    discontinuities = 0
    prev_sumf = None
    for k in range(-30, 31):
        slope = k * 0.5
        logs, _ = run(0.0, slope, 'foot', 1/240, 3.0, log=True)
        tail = logs[-30:]
        avg_sumf = sum(l.sum_f for l in tail) / len(tail)
        if prev_sumf is not None:
            jump = abs(avg_sumf - prev_sumf)
            if jump > WEIGHT_PER_WHEEL * 0.5:  # half a wheel weight = huge jump
                discontinuities += 1
                fail("T8", f"slope={slope:+.1f}d: SumF jumped by {jump:.0f}N")
        prev_sumf = avg_sumf
    print(f"  discontinuities found: {discontinuities}")


def T9_energy_invariant():
    """With throttle==0, mechanical energy E = KE + PE must not increase
    (modulo float noise). Brake/tire dissipate; gravity is conservative."""
    print("\n=== T9: energy invariant (E_total monotone non-increasing, throttle=0) ===")
    cases = [
        ("forward + brake", 60/3.6, 0, 'foot'),
        ("backward + brake", -10/3.6, 0, 'foot'),
        ("downhill + brake", 60/3.6, -5, 'foot'),
        ("uphill coast", 5/3.6, 10, 'foot'),
    ]
    for name, v0, slope, bt in cases:
        _, s = run(v0, slope, bt, 1/240, 6.0)
        if s['energy_inj_violations'] > 1.0:
            fail("T9", f"{name}: total E increase {s['energy_inj_violations']:.1f} J "
                       f"max step dE={s['energy_max_step_increase']:.2e}")
        else:
            print(f"  {name:25s}: E_violations={s['energy_inj_violations']:.2e} J  "
                  f"max_step_dE={s['energy_max_step_increase']:.2e}  vx_f={s['final_vx']:+.4f}")


def T10_threshold_sensitivity():
    """Run scenario at v0 just below and just above MOTION_SIGN_THRESHOLD."""
    print("\n=== T10: MOTION_SIGN_THRESHOLD discontinuity (v0 around 0.1 m/s) ===")
    around = [0.05, 0.08, 0.099, 0.1, 0.101, 0.12, 0.15, 0.2]
    prev_vx = None
    for v0 in around:
        _, s = run(v0, 0.0, 'foot', 1/240, 3.0)
        print(f"  v0={v0:.4f}  final_vx={s['final_vx']:+.6f}")
        if prev_vx is not None and abs(s['final_vx'] - prev_vx) > 0.05:
            fail("T10", f"discontinuity at MOTION_SIGN_THRESHOLD: v0 step from prev produced jump")
        prev_vx = s['final_vx']


def T11_asymmetric_chatter():
    """Asymmetric brake duty cycles (short pulses, long holds). These stress
    the lock/unlock transition at different pressure levels."""
    print("\n=== T11: asymmetric brake chatter (10Hz, duty 20%/80%) ===")
    for duty in [0.2, 0.8]:
        period = 0.1  # 10 Hz
        bf = lambda t, d=duty, p=period: 1.0 if (t % p) < (p * d) else 0.0
        _, s = run(60/3.6, 0.0, 'foot', 1/240, 6.0, brake_input_fn=bf)
        print(f"  duty={duty:.0%}: final_vx={s['final_vx']:+.6f}  "
              f"E_violations={s['energy_inj_violations']:.2e}J  "
              f"max_step_dE={s['energy_max_step_increase']:.2e}J")
        if s['energy_inj_violations'] > 1.0:
            fail("T11", f"duty={duty:.0%} total E increase {s['energy_inj_violations']:.1f} J")
        if s['final_vx'] < -0.01:
            fail("T11", f"duty={duty:.0%} reversed vehicle: vx={s['final_vx']:.4f}")


def T12_slope_chatter():
    """Brake chatter on a slope — combines slope force with lock/unlock cycling."""
    print("\n=== T12: brake chatter on slope (10Hz, slopes 5/10/15 deg) ===")
    for slope in [5, 10, 15]:
        period = 0.1
        bf = lambda t, p=period: 1.0 if (t % p) < (p * 0.5) else 0.0
        _, s = run(40/3.6, slope, 'foot', 1/240, 8.0, brake_input_fn=bf)
        print(f"  slope={slope:2d}d: final_vx={s['final_vx']:+.6f}  "
              f"E_violations={s['energy_inj_violations']:.2e}J  "
              f"max_step_dE={s['energy_max_step_increase']:.2e}J")
        if s['energy_inj_violations'] > 1.0:
            fail("T12", f"slope={slope}d chatter E increase {s['energy_inj_violations']:.1f} J")


def T13_creep_from_rest():
    """Start at rest on flat, apply tiny throttle then brake. Vehicle must not
    overshoot into negative vx."""
    print("\n=== T13: creep from rest then brake ===")
    for tq in [5.0, 20.0, 50.0]:
        def thr(t, tq_=tq):
            return tq_ if t < 0.5 else 0.0
        def br(t):
            return 0.0 if t < 0.5 else 1.0
        _, s = run(0.0, 0.0, 'foot', 1/240, 3.0, brake_input_fn=br, throttle_fn=thr)
        print(f"  tq={tq:5.1f}: final_vx={s['final_vx']:+.6f}")
        if s['final_vx'] < -1e-3:
            fail("T13", f"tq={tq}: reversed to vx={s['final_vx']:.6f}")
        if s['energy_inj_violations'] > 1.0:
            fail("T13", f"tq={tq}: E injection {s['energy_inj_violations']:.1f} J")


def main():
    print("=" * 90)
    print("ADVERSARIAL BRAKE FALSIFICATION HARNESS")
    print("Target: VehicleSolver.cpp brake/handbrake logic, current state on disk")
    print("=" * 90)

    T1_dt_convergence()
    T2_force_balance_at_rest()
    T3_long_drift()
    T4_low_velocity_sweep()
    T5_brake_chatter()
    T6_throttle_to_brake()
    T7_handbrake_isolation()
    T8_slope_equilibrium_sweep()
    T9_energy_invariant()
    T10_threshold_sensitivity()
    T11_asymmetric_chatter()
    T12_slope_chatter()
    T13_creep_from_rest()

    print("\n" + "=" * 90)
    if FAILURES:
        print(f"FOUND {len(FAILURES)} FAILURES:")
        for f_ in FAILURES:
            print(f"  {f_}")
    else:
        print("No failures detected (suspicious — make tests stricter)")
    print("=" * 90)
    sys.exit(1 if FAILURES else 0)


if __name__ == '__main__':
    main()
