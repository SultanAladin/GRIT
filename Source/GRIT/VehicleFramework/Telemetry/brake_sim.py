"""
Exact physics simulation of VehicleSolver brake behavior.
Models: pressure dynamics, Pacejka MF6.1 (kN units), friction state machine,
wheel omega integration, reversal guard — matching the C++ line by line.
"""

import math
import csv
import os

# =============================================================================
# VEHICLE CONSTANTS (GT-R NISMO defaults from C++ headers)
# =============================================================================
VEHICLE_MASS_KG = 1740.0
GRAVITY = 9.80
NUM_WHEELS = 4
MASS_PER_WHEEL = VEHICLE_MASS_KG / NUM_WHEELS
WEIGHT_PER_WHEEL = MASS_PER_WHEEL * GRAVITY

TIRE_RADIUS = 0.45
WHEEL_INERTIA = 1.2
INV_WHEEL_INERTIA = 1.0 / WHEEL_INERTIA

# Brake specs
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

# Friction state machine
MU_STATIC_LONG = 1.0
VELOCITY_DEADBAND = 0.05
OMEGA_LOCK_THRESHOLD = 0.1
MIN_BRAKE_LOCK = 10.0
C_RR = 0.015
FINAL_DRIVE_EFF = 0.95

# Pacejka MF6.1
PBx1 = 18.0;  PBx2 = 20.0;  PBx3 = 0.50
PCx1 = 1.95
PDx1 = 1.6;   PDx2 = -0.03; PDx3 = 0.015
PEx1 = -0.10; PEx2 = -0.15; PEx3 = 0.005; PEx4 = 0.030
FZ0_KN = 5.0

KINDA_SMALL_NUMBER = 1e-4
UE_SMALL_NUMBER = 1e-8


def calc_brake_torque(pressure_pa):
    clamping_force = pressure_pa * PISTON_AREA
    return 2.0 * FRICTION_COEFF_COLD * clamping_force * EFF_RADIUS


def pacejka_fx(kappa, fz_n):
    if fz_n < UE_SMALL_NUMBER:
        return 0.0
    kappa = max(-0.92, min(1.5, kappa))
    fz_kn = fz_n * 0.001
    dfz = (fz_kn - FZ0_KN) / FZ0_KN
    Dx = (PDx1 + PDx2 * dfz) * fz_kn
    Cx = PCx1
    BCD = (PBx1 + PBx2 * dfz) * fz_kn
    Bx = BCD / max(Cx * Dx, UE_SMALL_NUMBER)
    eff_slip = kappa  # no horizontal shift (defaults 0)
    Ex = (PEx1 + PEx2 * dfz + PEx3 * dfz * dfz) * (1.0 - PEx4 * (1.0 if eff_slip >= 0 else -1.0))
    Ex = max(-1.0, min(1.0, Ex))
    Bk = Bx * eff_slip
    inner = Bk - Ex * (Bk - math.atan(Bk))
    force_kn = Dx * math.sin(Cx * math.atan(inner))
    return force_kn * 1000.0


def sign(x):
    if x > 0: return 1.0
    if x < 0: return -1.0
    return 0.0


def simulate(scenario_name, initial_speed_ms, slope_deg, brake_type, brake_start_t=0.5, duration_s=5.0):
    dt = 1.0 / 240.0
    vx = initial_speed_ms
    omega = initial_speed_ms / TIRE_RADIUS
    brake_pressure = 0.0
    omega_front = omega
    omega_rear = omega

    if brake_type == 'foot':
        num_braking = 4;  max_p = MAX_BRAKE_PRESSURE
    elif brake_type == 'handbrake':
        num_braking = 2;  max_p = MAX_HANDBRAKE_PRESSURE
    else:
        num_braking = 0;  max_p = 0.0

    slope_rad = math.radians(slope_deg)
    fz = MASS_PER_WHEEL * GRAVITY * math.cos(slope_rad)
    # CORRECT SIGN: slope force in wheel-forward direction.
    # Positive slope = uphill ahead. Gravity component along surface = negative (pulls backward).
    f_slope_long = -MASS_PER_WHEEL * GRAVITY * math.sin(slope_rad) * math.cos(slope_rad)
    # Simpler: the tangential gravity component = m*g*sin(theta) pointing downhill.
    # In our convention, uphill = +X (forward). Downhill = -X. So f_slope_long is negative.
    f_slope_long = -MASS_PER_WHEEL * GRAVITY * math.sin(slope_rad)

    f_breakaway_long = fz * MU_STATIC_LONG
    f_rolling_threshold = C_RR * fz

    rows = []
    t = 0.0
    step = 0
    kappa_out = 0.0

    while t < duration_s:
        brake_input = 1.0 if t >= brake_start_t else 0.0

        # Pressure dynamics
        target_p = brake_input * max_p
        pdelta = target_p - brake_pressure
        if pdelta > 0:
            brake_pressure += min(pdelta, PRESSURE_RISE_RATE * dt)
        elif pdelta < 0:
            brake_pressure += max(pdelta, -PRESSURE_FALL_RATE * dt)

        brake_torque = calc_brake_torque(brake_pressure)
        safe_r = max(TIRE_RADIUS, 0.01)
        f_brake_capacity = brake_torque / safe_r
        t_drive_raw = 0.0
        f_drive = 0.0

        omega_w = omega_rear

        # MotionSign: Vx > Omega > Drive > default
        if abs(vx) > 0.1:
            motion_sign = sign(vx)
        elif abs(omega_w) > KINDA_SMALL_NUMBER:
            motion_sign = sign(omega_w)
        elif abs(t_drive_raw) > KINDA_SMALL_NUMBER:
            motion_sign = sign(t_drive_raw)
        else:
            motion_sign = 1.0

        f_brake_signed = f_brake_capacity * motion_sign
        f_net_long = f_drive - f_brake_signed - f_slope_long
        fx_breakaway = abs(f_net_long) > f_breakaway_long

        vx_deadband = abs(vx) < VELOCITY_DEADBAND
        omega_deadband = abs(omega_w) < 2.0
        fx_counteracted = abs(f_net_long) < f_breakaway_long
        fx_static_entry = vx_deadband and fx_counteracted

        brake_active = brake_torque > MIN_BRAKE_LOCK
        f_total_hold = f_brake_capacity + f_breakaway_long
        brake_can_hold = f_total_hold > abs(f_slope_long)
        flat_ground = brake_active and (abs(f_slope_long) < f_rolling_threshold)
        brake_sufficient = brake_active and (brake_can_hold or flat_ground)

        is_flat = abs(f_slope_long) < f_rolling_threshold
        fx_static_lock = not fx_breakaway and fx_static_entry
        static_lock = fx_static_lock and is_flat
        brake_static_lock = brake_active and vx_deadband and omega_deadband and brake_sufficient

        # Force computation
        if static_lock or brake_static_lock:
            eff_mass = fz / GRAVITY
            f_stop = -eff_mass * vx / max(dt, 0.001)
            fx_brake_wheel = f_stop + (-f_slope_long)
            if abs(fx_brake_wheel) > f_breakaway_long:
                scale = f_breakaway_long / abs(fx_brake_wheel)
                fx_brake_wheel *= scale
            omega_rear = 0.0
            omega_front = 0.0
            regime = "STATIC"
            kappa_out = 0.0
        else:
            # KINETIC - Pacejka + Newton solver
            kappa = (omega_w * TIRE_RADIUS - vx) / max(abs(vx), abs(omega_w * TIRE_RADIUS), 1.0)
            # SolveContactSlip BrakeSign: omega > Vx > drive > default
            if abs(omega_w) > KINDA_SMALL_NUMBER:
                bsign = sign(omega_w)
            elif abs(vx) > KINDA_SMALL_NUMBER:
                bsign = sign(vx)
            elif abs(t_drive_raw) > KINDA_SMALL_NUMBER:
                bsign = sign(t_drive_raw)
            else:
                bsign = 1.0
            t_net_solver = t_drive_raw - brake_torque * bsign

            for _ in range(12):
                ks = max(-0.92, min(1.5, kappa))
                fxs = pacejka_fx(ks, fz)
                op = omega_w + ((t_net_solver - fxs * TIRE_RADIUS) / WHEEL_INERTIA) * dt
                wp = op * TIRE_RADIUS
                dn = max(abs(vx), abs(wp), 1.0)
                kt = (wp - vx) / dn
                res = kappa - kt
                if abs(res) < 5e-5: break
                kp = min(1.5, max(-0.92, kappa + 1e-4))
                fxp = pacejka_fx(kp, fz)
                opp = omega_w + ((t_net_solver - fxp * TIRE_RADIUS) / WHEEL_INERTIA) * dt
                wpp = opp * TIRE_RADIUS
                dp = max(abs(vx), abs(wpp), 1.0)
                ktp = (wpp - vx) / dp
                dR = 1.0 - (ktp - kt) / 1e-4
                if abs(dR) > 1e-9:
                    kappa += max(-0.15, min(0.15, -res / dR))
                else:
                    kappa -= res * 0.3

            kappa = max(-0.92, min(1.5, kappa))
            fx_brake_wheel = pacejka_fx(kappa, fz)
            kappa_out = kappa
            regime = "KINETIC"

            # Stage 7
            t_tire = -fx_brake_wheel * TIRE_RADIUS
            t_accel = t_drive_raw + t_tire
            if abs(omega_w) > KINDA_SMALL_NUMBER:
                osr = sign(omega_w)
            elif abs(t_accel) > KINDA_SMALL_NUMBER:
                osr = sign(t_accel)
            else:
                osr = 0.0
            t_roll = -C_RR * fz * TIRE_RADIUS * osr
            t_brake_s7 = -brake_torque * osr
            t_resist_mag = abs(t_roll) + abs(t_brake_s7)
            oa = abs(omega_w)
            t_stop_w = (WHEEL_INERTIA * oa) / dt if dt > 0 else 0.0
            t_resist_c = min(t_resist_mag, t_stop_w) if oa > KINDA_SMALL_NUMBER else t_resist_mag
            t_resist_s = -osr * t_resist_c if abs(osr) > KINDA_SMALL_NUMBER else 0.0
            t_net_s7 = t_accel + t_resist_s
            on = omega_w + (t_net_s7 * INV_WHEEL_INERTIA) * dt

            if sign(omega_w) != sign(on) and sign(omega_w) != 0.0 and t_accel * sign(omega_w) <= 0.0:
                on = 0.0
            if abs(omega_w) < KINDA_SMALL_NUMBER and brake_torque > MIN_BRAKE_LOCK and abs(t_accel) < brake_torque:
                on = 0.0
            if abs(on) < OMEGA_LOCK_THRESHOLD and brake_torque > 10.0:
                on = 0.0

            omega_rear = on
            if brake_type == 'foot':
                omega_front = on

        # Vehicle force
        if brake_type == 'handbrake':
            kf = (omega_front * TIRE_RADIUS - vx) / max(abs(vx), abs(omega_front * TIRE_RADIUS), 1.0)
            fxf = pacejka_fx(kf, fz)
            total_fx = fx_brake_wheel * 2 + fxf * 2
            # Update front omega (no brake)
            tt = -fxf * TIRE_RADIUS
            tr = -C_RR * fz * TIRE_RADIUS * sign(omega_front) if abs(omega_front) > KINDA_SMALL_NUMBER else 0.0
            omega_front = omega_front + ((tt + tr) * INV_WHEEL_INERTIA) * dt
        else:
            total_fx = fx_brake_wheel * 4

        f_slope_vehicle = VEHICLE_MASS_KG * GRAVITY * math.sin(slope_rad)
        ax = (total_fx - f_slope_vehicle) / VEHICLE_MASS_KG
        vx = vx + ax * dt

        if step % 24 == 0:
            rows.append({
                'Time_s': round(t, 4),
                'Vx_ms': round(vx, 6),
                'Speed_kmh': round(vx * 3.6, 3),
                'Omega_r': round(omega_rear, 6),
                'BrkPres_MPa': round(brake_pressure / 1e6, 3),
                'BrkTq_Nm': round(brake_torque, 1),
                'Fx_w_N': round(fx_brake_wheel, 1),
                'F_brkCap_N': round(f_brake_capacity, 1),
                'F_net_N': round(f_net_long, 1),
                'MotSign': motion_sign,
                'Regime': regime,
                'Kappa': round(kappa_out, 5),
                'BrkStLock': brake_static_lock,
                'FxBreak': fx_breakaway,
            })

        t += dt
        step += 1

    out_dir = os.path.dirname(os.path.abspath(__file__))
    fname = os.path.join(out_dir, f"BrakeSim_{scenario_name}.csv")
    with open(fname, 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=rows[0].keys())
        w.writeheader()
        w.writerows(rows)
    print(f"[{scenario_name}] {len(rows)} samples -> {os.path.basename(fname)}")
    print(f"  Setup: {initial_speed_ms:.1f} m/s, slope={slope_deg}deg, {brake_type}, brake@t={brake_start_t}s")

    stopped_t = reversed_t = None
    min_vx = 999.0
    for r in rows:
        if stopped_t is None and abs(r['Vx_ms']) < 0.01 and r['Time_s'] > brake_start_t:
            stopped_t = r['Time_s']
        if r['Vx_ms'] < -0.05 and r['Time_s'] > brake_start_t and reversed_t is None:
            reversed_t = r['Time_s']
        min_vx = min(min_vx, r['Vx_ms'])

    if stopped_t: print(f"  STOPPED at t={stopped_t:.3f}s")
    else: print(f"  NOT STOPPED (final Vx={rows[-1]['Vx_ms']:.4f} m/s)")
    if reversed_t: print(f"  *** REVERSED at t={reversed_t:.3f}s! Min Vx={min_vx:.4f} ***")
    else: print(f"  No reversal (min Vx={min_vx:.6f})")

    # Key transition rows
    prev = None
    for r in rows:
        show = r['Regime'] != prev
        if stopped_t and abs(r['Time_s'] - stopped_t) < 0.15: show = True
        if reversed_t and abs(r['Time_s'] - reversed_t) < 0.15: show = True
        if show:
            print(f"    t={r['Time_s']:.3f} Vx={r['Vx_ms']:+.6f} Om={r['Omega_r']:+.4f} "
                  f"{r['Regime']:>7} BrkTq={r['BrkTq_Nm']:7.0f} Fx={r['Fx_w_N']:+8.1f} "
                  f"F_net={r['F_net_N']:+8.0f} K={r['Kappa']:+.4f} Ms={r['MotSign']:+.0f} "
                  f"BrkSL={r['BrkStLock']} FxBrk={r['FxBreak']}")
        prev = r['Regime']

    return reversed_t is not None


if __name__ == '__main__':
    print("=" * 90)
    print("BRAKE PHYSICS SIMULATION v3")
    print("=" * 90)
    fails = 0

    print("\n--- A: 60 km/h -> Foot Brake (flat) ---")
    fails += simulate("A_FootBrake_Flat", 60/3.6, 0, 'foot', 0.5, 4.0)

    print("\n--- B: 60 km/h -> Handbrake (flat) ---")
    fails += simulate("B_Handbrake_Flat", 60/3.6, 0, 'handbrake', 0.5, 6.0)

    print("\n--- C: Stopped 5-deg slope -> Foot Brake ALREADY held ---")
    fails += simulate("C_FootBrake_Slope_Held", 0, 5, 'foot', 0.0, 3.0)

    print("\n--- D: Stopped 5-deg slope -> Handbrake ALREADY held ---")
    fails += simulate("D_Handbrake_Slope_Held", 0, 5, 'handbrake', 0.0, 3.0)

    print("\n--- E: 30 km/h downhill 10-deg -> Foot Brake ---")
    fails += simulate("E_Downhill10_Foot", 30/3.6, 10, 'foot', 0.5, 6.0)

    print("\n--- F: 2 km/h -> Foot Brake (edge case) ---")
    fails += simulate("F_Creep_Foot", 2/3.6, 0, 'foot', 0.5, 3.0)

    print("\n--- G: 60 km/h DOWNHILL 5-deg -> Foot Brake (stop then hold) ---")
    fails += simulate("G_60kmh_Downhill5_Foot", 60/3.6, 5, 'foot', 0.5, 8.0)

    print("\n--- H: Stopped 5-deg slope -> Brake pressed at t=0.5 (slope slides first) ---")
    fails += simulate("H_Slope5_BrakeLate", 0, 5, 'foot', 0.5, 4.0)

    print("\n" + "=" * 90)
    print(f"{'ALL PASSED' if fails == 0 else f'{fails} SCENARIOS FAILED'}")
    print("=" * 90)
