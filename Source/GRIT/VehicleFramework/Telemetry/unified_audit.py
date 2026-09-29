"""
Unified time-aligned audit: pull data for the SAME timestamp from every CSV,
plug it into the diff/clutch/Pacejka equations the solver runs, and look for discrepancies.

Anchor times: low-speed (gear stable, building boost), mid-speed (~100 km/h), peak speed.
"""
import csv, math
from collections import defaultdict

# -----------------------------------------------------------------------------
# 1. LOAD ALL CSVs
# -----------------------------------------------------------------------------
def load(p):
    with open(p, encoding='utf-8') as f:
        return list(csv.DictReader(f))

PT  = load('PowertrainLog.csv')
AERO= load('AerodynamicsLog.csv')
FSM = load('FrictionStateMachineLog.csv')

print(f'PowertrainLog : {len(PT)} rows')
print(f'AerodynamicsLog: {len(AERO)} rows')
print(f'FrictionStateMachineLog: {len(FSM)} rows  (per-wheel rows, so 4x sample count)')

# Group FSM rows by timestamp -> list of 4 wheel rows
fsm_by_t = defaultdict(list)
for r in FSM:
    fsm_by_t[float(r['Time_s'])].append(r)
print(f'FSM unique timestamps: {len(fsm_by_t)}')
print()

# -----------------------------------------------------------------------------
# 2. PICK ANCHOR TIMESTAMPS  --  pick from PowertrainLog where throttle > 0.5
# -----------------------------------------------------------------------------
def find_pt(target_speed):
    best = None; best_diff = 1e9
    for r in PT:
        if float(r['Throttle']) < 0.5: continue
        d = abs(float(r['Speed_kmh']) - target_speed)
        if d < best_diff:
            best_diff = d; best = r
    return best

def closest_aero(t):
    return min(AERO, key=lambda r: abs(float(r['Time_s']) - t))

def closest_fsm(t):
    # find fsm timestamp closest to t (returns the 4 wheel rows at that time)
    nearest_t = min(fsm_by_t.keys(), key=lambda x: abs(x - t))
    return nearest_t, fsm_by_t[nearest_t]

ANCHORS = [
    ('low',  find_pt(40)),
    ('mid',  find_pt(100)),
    ('high', find_pt(150)),
]

# -----------------------------------------------------------------------------
# 3. REPRODUCE THE SOLVER MATH FOR EACH ANCHOR
# -----------------------------------------------------------------------------
# Constants from spec files
WHEEL_RADIUS_m   = 0.504           # large tire (user said intentional)
DRIVETRAIN_EFF   = 0.95            # FinalDriveEfficiency in VehicleSolver.cpp:5566
FINAL_DRIVE      = 3.55            # TransmissionSpec/DiffSpec
GEARS = {3: 4.2, 4: 3.1, 5: 2.5, 6: 1.9, 7: 1.5, 8: 1.2}  # idx 3..8 = 1st..6th
FRONT_REAR_BIAS  = 0.35            # CenterDifferential preset 1: AWD/TV, 35:65 split
MASS_kg          = 1740            # Nismo mass
g                = 9.81

def gear_label(idx):
    n = idx - 2
    return f'{n}st/nd/th' if n>0 else f'reverse_or_neutral({idx})'

print('='*120)
print('UNIFIED AUDIT TABLE  (Powertrain + Aero + Friction-state at same timestamp)')
print('='*120)

for label, pt in ANCHORS:
    if not pt: continue
    t = float(pt['Time_s'])
    aero = closest_aero(t)
    nearest_fsm_t, fsm4 = closest_fsm(t)

    # --- Read powertrain row ---
    v_kmh = float(pt['Speed_kmh']); v_ms = v_kmh/3.6
    rpm   = float(pt['EngineRPM'])
    eng_tq= float(pt['EngineTorque_Nm'])
    cl_tq = float(pt['ClutchTorque_Nm'])
    cl_eng= float(pt['ClutchEngagement']); cl_lock = float(pt['ClutchLockup'])
    slip  = float(pt['SlipRPM'])
    gear  = int(pt['GearCurrent'])
    cratio= float(pt['TxCombinedRatio'])
    accelG= float(pt['Acceleration_G'])
    thr   = float(pt['Throttle'])
    W_drv = [float(pt[f'W{i}_DriveTq']) for i in range(4)]
    W_om  = [float(pt[f'W{i}_Omega']) for i in range(4)]
    W_fx  = [float(pt[f'W{i}_Fx']) for i in range(4)]
    W_load= [float(pt[f'W{i}_Load']) for i in range(4)]
    W_code= [int(pt[f'W{i}_Code']) for i in range(4)]
    W_slip= [float(pt[f'W{i}_SlipRatio']) for i in range(4)]

    # --- Aero ---
    drag = float(aero['TotalDrag_N']); df = float(aero['TotalDownforce_N'])
    front_df = float(aero['FrontDownforce_N']); rear_df = float(aero['RearDownforce_N'])

    # --- FSM (per wheel) ---
    fsm_per_wheel = {}
    for r in fsm4:
        idx = int(r['WheelIdx'])
        fsm_per_wheel[idx] = r

    # ------------- DIFFERENTIAL MATH (replay the solver) -------------
    # Step 1: trans output torque
    trans_out_calc = cl_tq * cratio                               # [Nm]
    # Step 2: AWD branch → 35/65 split
    base_front = trans_out_calc * FRONT_REAR_BIAS
    base_rear  = trans_out_calc * (1.0 - FRONT_REAR_BIAS)
    # Step 3: per-axle 50/50 ignoring LSD lock (steering=0, deadbands)
    front_each_predicted = base_front * 0.5
    rear_each_predicted  = base_rear  * 0.5
    # Step 4: drive force at wheel = T * eff / R
    front_F_each = front_each_predicted * DRIVETRAIN_EFF / WHEEL_RADIUS_m
    rear_F_each  = rear_each_predicted  * DRIVETRAIN_EFF / WHEEL_RADIUS_m
    F_drive_predicted = 2*front_F_each + 2*rear_F_each
    F_drive_actual    = sum(W_drv) * DRIVETRAIN_EFF / WHEEL_RADIUS_m
    Fx_actual_total   = sum(W_fx)

    # Newton's 2nd law check
    F_net_observed = MASS_kg * accelG * g
    F_roll = 0.012 * (MASS_kg*g + df)  # estimate Crr=0.012

    # --- Build the table row ---
    print()
    print(f'\\n[ANCHOR: {label.upper()}]  t={t:.3f}s   gear_idx={gear}({gear_label(gear)})  v={v_kmh:.1f} km/h')
    print('-'*120)
    print(f'  POWERTRAIN  rpm={rpm:6.0f}  eng_tq={eng_tq:6.0f} Nm  cl_tq={cl_tq:6.0f} Nm  cl_eng={cl_eng:.2f}  lock={cl_lock:.2f}  slip_rpm={slip:6.0f}  thr={thr:.2f}')
    print(f'              CombinedRatio(reported)={cratio:6.3f}   Spec_ratio_for_gear{gear}={GEARS.get(gear,0)*FINAL_DRIVE:.3f}   accel={accelG:.3f}g')
    print()
    print(f'  WHEEL CODES   W0={W_code[0]:>3} (FL=-1)   W1={W_code[1]:>3} (FR=+1)   W2={W_code[2]:>3} (RL=-2)   W3={W_code[3]:>3} (RR=+2)')
    print(f'  GROUPING TEST   FL_match={W_code[0]==-1}  FR_match={W_code[1]==1}  RL_match={W_code[2]==-2}  RR_match={W_code[3]==2}')
    print()
    print(f'  DIFF MATH (predicted)')
    print(f'    TransOut       = clutch_tq * ratio = {cl_tq:.0f} * {cratio:.3f} = {trans_out_calc:.0f} Nm')
    print(f'    AWD split 35/65: front_axle={base_front:.0f}  rear_axle={base_rear:.0f}')
    print(f'    Per wheel pred : front_each={front_each_predicted:.0f}  rear_each={rear_each_predicted:.0f}')
    print(f'  ACTUAL (telemetry W*_DriveTq)')
    print(f'    W0={W_drv[0]:7.0f}  W1={W_drv[1]:7.0f}  W2={W_drv[2]:7.0f}  W3={W_drv[3]:7.0f}')
    print(f'    SUM={sum(W_drv):.0f} Nm   (predicted SUM={trans_out_calc:.0f})  ratio_actual_to_pred={sum(W_drv)/max(trans_out_calc,1):.2%}')
    print()
    print(f'  WHEEL OMEGA   W0={W_om[0]:6.1f}  W1={W_om[1]:6.1f}  W2={W_om[2]:6.1f}  W3={W_om[3]:6.1f} rad/s   (expected ~{v_ms/WHEEL_RADIUS_m:.1f})')
    print(f'  WHEEL FX      W0={W_fx[0]:7.0f} N  W1={W_fx[1]:7.0f}  W2={W_fx[2]:7.0f}  W3={W_fx[3]:7.0f}   (sum={sum(W_fx):.0f})')
    print(f'  WHEEL LOAD    W0={W_load[0]:6.0f} N  W1={W_load[1]:6.0f}  W2={W_load[2]:6.0f}  W3={W_load[3]:6.0f}')
    print(f'  WHEEL SLIP    W0={W_slip[0]:7.4f}  W1={W_slip[1]:7.4f}  W2={W_slip[2]:7.4f}  W3={W_slip[3]:7.4f}')
    print()
    print(f'  AERO          drag={drag:.0f} N  downforce={df:.0f} N  (front {front_df:.0f}, rear {rear_df:.0f})')
    print(f'  NEWTON CHECK  F_net = m*a = {MASS_kg}*{accelG:.3f}*{g} = {F_net_observed:.0f} N')
    print(f'                F_drive (from Fx) = {Fx_actual_total:.0f} N')
    print(f'                F_drag = {drag:.0f} N    F_roll(est) = {F_roll:.0f} N')
    print(f'                F_net_predicted = Fx - drag - roll = {Fx_actual_total - drag - F_roll:.0f} N')
    print(f'                discrepancy m*a vs Fx-drag-roll = {(Fx_actual_total - drag - F_roll) - F_net_observed:.0f} N')
    print()
    print(f'  FSM PER WHEEL  (state machine outputs)')
    for wi in sorted(fsm_per_wheel.keys()):
        r = fsm_per_wheel[wi]
        st = r['State']
        is_rear = r['bIsRearWheel']
        din = float(r['DriveTorque_Nm'])
        fxp = float(r['Fx_PreOverride_N'])
        fxa = float(r['Fx_Applied_N'])
        tlim = float(r['F_TireFrictionLimit_N'])
        lk = r['bWheelLocked']
        print(f'    W{wi}: state={st:>16}  isRear={is_rear}  drive_in={din:7.0f}  Fx_pre={fxp:7.0f}  Fx_app={fxa:7.0f}  TireLim={tlim:7.0f}  Locked={lk}')

# -----------------------------------------------------------------------------
# 4. SCAN: are wheel codes EVER the spec values across the log?
# -----------------------------------------------------------------------------
print()
print('='*120)
print('WHEELCODE SCAN ACROSS ENTIRE LOG')
print('='*120)
combos = defaultdict(int)
for r in PT:
    key = (r['W0_Code'], r['W1_Code'], r['W2_Code'], r['W3_Code'])
    combos[key] += 1
for key, count in combos.items():
    print(f'  W0={key[0]:>3}  W1={key[1]:>3}  W2={key[2]:>3}  W3={key[3]:>3}   ->  {count} rows')
print()
print('Spec says (E_WheelCode):  FL=-1  FR=+1  RL=-2  RR=+2')
print()

# Scan FSM bIsRearWheel — does it agree?
fsm_rear_by_idx = defaultdict(set)
for r in FSM:
    fsm_rear_by_idx[int(r['WheelIdx'])].add(r['bIsRearWheel'])
print('FSM bIsRearWheel per wheel index (across entire log):')
for idx in sorted(fsm_rear_by_idx.keys()):
    print(f'  W{idx}: {fsm_rear_by_idx[idx]}')
