"""
Free-body diagram for the GTR at 99 km/h (4th gear, WOT, no brake).
Pulls the EXACT numbers from the telemetry CSVs and reconciles them.

Question: is aero the bottleneck? Or something else?
"""

import math

# =============================================================================
# OBSERVED VALUES (single frame: t = 10.5555s)
# =============================================================================
# From PowertrainLog at t=10.555:
v_kmh = 99.067
v_ms = v_kmh / 3.6                          # 27.52 m/s
engine_rpm = 5159.9
gear = 4                                    # idx 4 = 2nd forward (index map: 3=1st)
clutch_tq = 1275.0                          # Nm at clutch output
clutch_eng = 1.0
slip_rpm = 1290.9                           # ENGINE - DRIVELINE slip
throttle = 1.0
accel_g = 0.4124                            # measured longitudinal acceleration

# Wheel-side (W0,W1=front; W2,W3=rear) at same frame:
W_DriveTq = [4498.5, 4498.5, -96.9, -96.9]  # Nm requested at each wheel
W_Fx     = [5289.1, 5289.6, 4323.7, 4323.7]  # actual longitudinal force at contact patch
# (telemetry header order: Fx is cols 67..70, but per my pull above I read 67-69 and 56=W0 — let me verify)
# Actually re-reading: cols 30-33 = W*_DriveTq (4498.5/4498.5/-96.9/-96.9 confirmed)
# cols 67-70 = W*_Fx; in the row above those are -98.5/6.3/-7.7/+something — TINY values
# but cols 56-59 = W*_StaticLoad ~5300 N each
# And there seem to be larger ~4400 numbers too — those are W*_Load (col 52-55) maybe
# Let me reread my data row carefully:
# t,speed,rpm,gear,clutchTq,clutchEng,slipRPM,W0DT,W1DT,W2DT,W3DT, W0Fx, W1Fx, W2Fx, W3Fx, ...
# from awk extraction: "$66 $67 $68 $69 $70 $71 $72 $73 $56 $57 $58 $59"
# col66=W0_LoadTransfer, col67-70 = W*_Fx, col70-73=W*_Fy, col56-59=W*_StaticLoad? Need to recount
# Actually the printed line: ... 4498.5,4498.5,-96.9,-96.9, 6.3,6.3,-7.7,-7.7, 5289.1,5289.6,4323.7,4323.7
# = DriveTq[0..3], something[4], something[5], W*_StaticLoad[0..3]?
# Header pos: col30-33=W_DriveTq, col38-41=W_Omega, col66-69=W_Fx, col70-73=W_Fy
# Output cols: $30,$31,$32,$33,$66,$67,$68,$69,$70,$71,$72,$73,$56,$57,$58,$59
# So 4498.5,4498.5,-96.9,-96.9 = DriveTq (verified)
#    6.3,6.3,-7.7,-7.7         = W_Fx?? These are tiny!!! But they're column 66-69
#    5289.1,5289.6,4323.7,4323.7 = W_Fy?? These are cols 70-73
# HMMMM those cols don't match — let me just rely on AeroLog and Powertrain summary.

# Actual W_Fx that I want = column 67-70 in PowertrainLog, but my awk may have extracted wrong cols.
# Use Acceleration_G as ground truth: a = 0.4124 g = 4.04 m/s^2
mass_kg = 1740.0
F_net_observed = mass_kg * accel_g * 9.81   # N — net force from observed acceleration

# From AeroLog at t=10.555:
aero_drag = 1374.34                         # N
aero_downforce = 3129.56
aero_body_drag = 298.70
aero_body_df = -128.02
aero_front_df = 1692.01
aero_rear_df = 1339.35

# Engine torque curve eval at 5159 RPM
TORQUE_CURVE = [
    (700.0, 220.0), (1500.0, 350.0), (2500.0, 500.0), (3000.0, 580.0),
    (3600.0, 652.0), (4000.0, 652.0), (4500.0, 652.0), (5000.0, 652.0),
    (5800.0, 652.0), (6000.0, 640.0), (6500.0, 620.0), (6800.0, 600.0),
    (7000.0, 570.0), (7200.0, 520.0)
]
def torque_at(rpm):
    if rpm <= TORQUE_CURVE[0][0]: return TORQUE_CURVE[0][1]
    if rpm >= TORQUE_CURVE[-1][0]: return TORQUE_CURVE[-1][1]
    for i in range(len(TORQUE_CURVE)-1):
        r0,t0=TORQUE_CURVE[i]; r1,t1=TORQUE_CURVE[i+1]
        if r0<=rpm<=r1:
            return t0 + (rpm-r0)/(r1-r0)*(t1-t0)
    return 0.0

eng_tq_curve = torque_at(engine_rpm)
turbo_boost = 1.27
eng_tq_actual = eng_tq_curve * turbo_boost   # Nm

# Drivetrain
gear_ratio_2nd = 3.1                         # GearRatios[4] = 2nd forward
final_drive = 3.55
combined_ratio = gear_ratio_2nd * final_drive  # 11.005
drivetrain_eff = 0.95
wheel_radius = 0.504                         # fitted from earlier

# =============================================================================
# FREE BODY DIAGRAM
# =============================================================================
print("="*70)
print(f"FREE-BODY DIAGRAM @ t=10.555s, v={v_kmh:.1f} km/h, gear=2nd, WOT")
print("="*70)

print("\n[INPUTS — all from telemetry]")
print(f"  speed                : {v_kmh:.2f} km/h = {v_ms:.2f} m/s")
print(f"  engine RPM           : {engine_rpm:.0f}")
print(f"  measured acceleration: {accel_g:.4f} g = {accel_g*9.81:.2f} m/s^2")
print(f"  throttle             : {throttle}")
print(f"  clutch slip          : {slip_rpm:.0f} RPM (engine ahead of driveline)")

print("\n[ENGINE OUTPUT]")
print(f"  curve torque @ 5160 RPM    : {eng_tq_curve:.0f} Nm")
print(f"  × turbo boost ({turbo_boost})         : {eng_tq_actual:.0f} Nm")
eng_power_kw = eng_tq_actual * engine_rpm * 2*math.pi / 60.0 / 1000.0
print(f"  engine power               : {eng_power_kw:.0f} kW = {eng_power_kw*1.341:.0f} hp")

print("\n[CLUTCH SECTION]")
print(f"  clutch torque transmitted  : {clutch_tq:.0f} Nm  (capped at 1500 Nm capacity)")
print(f"  ratio of curve→clutch      : {clutch_tq/eng_tq_actual*100:.1f}%  ← only this much survives!")
print(f"  power AT CLUTCH OUTPUT     : {clutch_tq * (engine_rpm-slip_rpm) * 2*math.pi/60 / 1000:.0f} kW")
print(f"  power LOST AS CLUTCH HEAT  : {(eng_tq_actual*engine_rpm - clutch_tq*(engine_rpm-slip_rpm)) * 2*math.pi/60/1000:.0f} kW")

print("\n[TRANSMISSION OUTPUT]")
trans_out_tq = clutch_tq * combined_ratio
drive_force_max_avail = trans_out_tq * drivetrain_eff / wheel_radius
print(f"  combined ratio (2nd × FD)  : {combined_ratio:.2f}")
print(f"  trans output torque        : {trans_out_tq:.0f} Nm   (clutch_tq × ratio)")
print(f"  available drive force      : {drive_force_max_avail:.0f} N at the wheel (×0.95 eff / R)")

print("\n[WHEEL TORQUE BUDGET]")
total_drive_tq = sum(W_DriveTq)
print(f"  W0 (FL) DriveTq            : {W_DriveTq[0]:>8.0f} Nm  ← FRONT")
print(f"  W1 (FR) DriveTq            : {W_DriveTq[1]:>8.0f} Nm  ← FRONT")
print(f"  W2 (RL) DriveTq            : {W_DriveTq[2]:>8.0f} Nm  ← REAR")
print(f"  W3 (RR) DriveTq            : {W_DriveTq[3]:>8.0f} Nm  ← REAR")
print(f"  TOTAL DriveTq              : {total_drive_tq:.0f} Nm")
print(f"  Front share                : {(W_DriveTq[0]+W_DriveTq[1])/total_drive_tq*100:.0f}%")
print(f"  Rear share                 : {(W_DriveTq[2]+W_DriveTq[3])/total_drive_tq*100:+.0f}%  (negative = rear is BRAKING!)")

print("\n[AERODYNAMICS — from AeroLog at same timestamp]")
print(f"  TotalDrag                  : {aero_drag:.0f} N")
print(f"  TotalDownforce             : {aero_downforce:.0f} N")
print(f"  Body drag                  : {aero_body_drag:.0f} N")
print(f"  Front downforce            : {aero_front_df:.0f} N")
print(f"  Rear downforce             : {aero_rear_df:.0f} N")
print(f"  Implied CdA                : {aero_drag/(0.5*1.225*v_ms*v_ms):.3f} m²")
print(f"  Real GTR CdA reference     : ~0.78 m²")

print("\n[NEWTON'S 2ND LAW BALANCE]")
print(f"  m·a (observed)             : {mass_kg:.0f} × {accel_g:.4f}g × 9.81 = {F_net_observed:.0f} N")
F_drive_total = total_drive_tq / wheel_radius
print(f"  drive force F_drive = ΣTq/R: {F_drive_total:.0f} N")
print(f"  drag F_drag                : {aero_drag:.0f} N")
# rolling resistance
crr = 0.015
F_roll = crr * (mass_kg*9.81 + aero_downforce)
print(f"  rolling F_roll = Crr·(mg+DF): {F_roll:.0f} N")
F_net_predicted = F_drive_total - aero_drag - F_roll
print(f"  → F_drive - F_drag - F_roll= {F_drive_total:.0f} - {aero_drag:.0f} - {F_roll:.0f} = {F_net_predicted:.0f} N")
print(f"  → predicted accel          : {F_net_predicted/mass_kg/9.81:.4f} g")
print(f"  → observed accel           : {accel_g:.4f} g")

print("\n[VERDICT — what's the bottleneck?]")
print(f"  Engine power available     : {eng_power_kw:.0f} kW")
print(f"  Power needed for v=27.5m/s : drag·v = {aero_drag*v_ms/1000:.0f} kW (just to overcome drag)")
print(f"  Power going to chassis     : F_drive·v = {F_drive_total*v_ms/1000:.0f} kW")
print(f"  Power lost in clutch slip  : {(eng_tq_actual*engine_rpm - clutch_tq*(engine_rpm-slip_rpm)) * 2*math.pi/60/1000:.0f} kW")

print("\n[TOP-SPEED PROJECTION at this CdA]")
# At rev limiter in 5th gear, what's drag-equilibrium top speed?
v_top_solve = lambda P: (2*P*1000 / (1.225*aero_drag/(0.5*v_ms*v_ms))) ** (1/3)
# F_drive at rev limiter:
eng_rpm_limit = 7150
ent_tq_limit = torque_at(eng_rpm_limit) * turbo_boost
ratio_5th = 1.5 * 3.55
F_drive_5th_locked = ent_tq_limit * ratio_5th * drivetrain_eff / wheel_radius
F_drive_5th_slipping = 1500 * ratio_5th * drivetrain_eff / wheel_radius
F_drive_5th_use = min(F_drive_5th_locked, F_drive_5th_slipping)
cda_implied = aero_drag/(0.5*1.225*v_ms*v_ms)
# v_eq: 0.5*ρ*v²*CdA + Crr*(mg + 0.5*ρ*v²*ClA) = F_drive
cla_implied = aero_downforce/(0.5*1.225*v_ms*v_ms)
A_coef = 0.5*1.225*(cda_implied + crr*cla_implied)
B_const = crr*mass_kg*9.81
# F_drive = A*v² + B  → v = sqrt((F-B)/A)
v_eq = math.sqrt(max(0,(F_drive_5th_use - B_const)/A_coef))
print(f"  At rev limiter in 5th: F_drive = {F_drive_5th_use:.0f} N (clutch{'-limited' if F_drive_5th_slipping<F_drive_5th_locked else ' locked, engine-limited'})")
print(f"  Drag-equilibrium top speed: {v_eq*3.6:.0f} km/h")
print(f"  vs telemetry actual peak  : 141 km/h")
print()
print("  If F_drive > F_drag at all speeds → top speed is bound by clutch/engine, not drag")
print("  If F_drive saturates at clutch cap → top speed grows only with √(P/CdA)")
