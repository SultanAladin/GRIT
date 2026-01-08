# VehicleSolver.cpp - Engineering Review

**File:** `VehicleSolver.cpp`
**Lines:** 6403
**Review Date:** 2026-01-08
**Reviewer:** AI Engineering Analysis
**Scope:** Accuracy + performance (mandatory units audit; slope behavior; aero/load transfer; clutch/solver stability; intake/turbo model)

---

## EXECUTIVE SUMMARY

This review is strictly about:

- **Accuracy** (dimensional consistency, physical correctness, frame correctness)
- **Performance** (hot paths, algorithmic complexity, avoidable work)

The file contains several strong subsystem implementations (notably the tire Newton solver), but **a few unit/meaning ambiguities materially undermine interpretability and validation**.

### Ratings (accuracy + performance only)

| Category | Rating | Notes |
|----------|--------|-------|
| **Units & Dimensional Consistency** | C | Demonstrable ambiguity/conflict around anti-roll bar stiffness usage and “weight vs normal load” in slope logic |
| **Slope / gravity behavior** | B- | Good intent + stiction handling exists; needs proof via frame-consistent gravity decomposition + validation matrix |
| **Aerodynamics correctness** | C+ | Magnitudes are SI-consistent; force direction/basis assumptions can be wrong on slopes/yaw |
| **Load transfer realism** | B | Two-path structure is good; roll centers currently hard-coded despite configuration support |
| **Numerical stability (tire Newton solver)** | A- | Robust; performance risk is Pacejka evaluation count and iteration budget |
| **Powertrain coupling correctness** | B- | Smoothing is numerically motivated; needs explicit validation criteria vs ground truth |
| **Turbo/intake physical correctness** | C | Turbo spool is plausible; intake/MAP dynamics are not engineering-grade |

**Overall assessment:** Several subsystems are strong, but the solver is not yet “proveably correct” under the requested slope/aero/roll-center validation conditions.

---

# 0) Units & Dimensional Consistency Audit (MANDATORY)

## 0.1 Unit system used in the solver (observed)

- **Length:** Unreal units `[cm]` for transforms; converted to `[m]` via `* 0.01f` in multiple subsystems.
- **Force:** SI `[N]` in solver math; converted to Unreal force units via `* 100.0f` before applying to Chaos (`1 N = 100 kg·cm/s²`).
- **Torque:** SI `[N·m]` in math; converted to Unreal torque units via `* 100.0f` when applied as `N·cm`.

## 0.2 Aerodynamics dimensional chain (force + moment)

**Ground truth:**

```
q = 0.5 * ρ * V^2        [Pa]
F = q * A * C            [N]
M = r × F                [N·m]
```

**Observed:** aerodynamic package structs store:

- Areas in `[m²]`
- Coefficients dimensionless `[-]`
- Force application points relative to COM in `[m]` (computed from socket locations in `[cm]` then `*0.01f`)

**Dimensional status:** consistent for magnitudes.

## 0.3 Roll stiffness terms (springs vs ARB) 

In `ComputeLoadTransferRealtime` (evidence):

- `K_spring_front` is formed by summing wheel spring rates → `[N/m]`.
- `K_phi_springs_front = K_spring_front * t^2 * 0.5` → `[N·m]` (often written `[N·m/rad]`, since `rad` is dimensionless).

**Dimensional status:** correct.

## 0.4 Anti-roll bar stiffness: explicit ambiguity/conflict

Two different interpretations are simultaneously present in code paths:

- **Interpretation A (force-based, consistent with ARB force implementation):**
  - `Bar.Stiffness` is a *linear* stiffness relating left-right wheel displacement difference to a vertical force couple.
  - Units: `[N/m]`.

- **Interpretation B (roll-stiffness-based, assumed in load transfer distribution):**
  - `Bar.Stiffness` is *already* an axle roll stiffness contribution.
  - Units: `[N·m]` (commonly `[N·m/rad]`).

The solver currently:

- Uses `Bar.Stiffness` as `[N/m]` when computing ARB forces.
- Adds `Bar.Stiffness` into `K_phi_ARB_front/rear` as if it were `[N·m]`.

**Dimensional verdict:** the code contains a **hard, demonstrable unit/meaning conflict**.

**Required resolution (documentation-level):** the project must choose one meaning and apply a conversion:

- If the tuning value is `[N/m]`, then an equivalent axle roll stiffness is approximately:
  - `K_phi_ARB ≈ k_arb * (t^2 / 2)`.
- If the tuning value is `[N·m]`, then the equivalent force law needs a mapping from roll to wheel displacement:
  - `F_left = +(K_phi_ARB / t) * φ`, `F_right = -(K_phi_ARB / t) * φ`, with `φ ≈ Δz / t`.

## 0.5 “Weight” vs “normal load” in slope logic

The solver frequently uses:

- `Fz` as the *contact normal load* `[N]`.

Any code that constructs “gravity” magnitudes from `Fz` is dimensionally fine but **physically ambiguous**, because:

- `Fz` is not generally equal to `m_wheel * g` (especially on slopes and under dynamic load transfer).

## 0.6 Turbo/intake pressure units: bar vs kPa vs Pa

Telemetry records `ManifoldPressure_kPa`.

Turbo specs use `BoostPressureCurve` labeled `[Bar]` and build a dimensionless pressure ratio via `1.0f + BoostPressure_bar`.

**Dimensional status:** pressure ratio is dimensionless, but the mapping implicitly assumes “bar above 1 bar ambient”. This is **meaning-ambiguous** unless explicitly defined.

### Accuracy/Performance/Risk

- **Accuracy impact:** High
- **Performance impact:** None
- **Risk level:** High (unit ambiguity blocks calibration and invalidates validation results)

---

# 1) Gravity & Slope Decomposition (required test conditions)

## 1.1 Ground truth (frame-consistent)

Let:

- `g_world = (0,0,-g)` with `g = 9.80 m/s²`
- `n` = contact/ground unit normal
- `ê_long`, `ê_lat` = vehicle body forward/right unit vectors

Then:

```
g_parallel = g_world - (g_world · n) n
g_long = g_parallel · ê_long
g_lat  = g_parallel · ê_lat
F_long,grav = m_total * g_long
F_lat,grav  = m_total * g_lat
```

These are the components that must be resisted (if static is possible) or will cause drift (if kinetic).

## 1.2 Required slope test matrix (steering = 0)

Test angles:

- `θ ∈ {2°, 5°, 10°, 15°, 20°, 30°}`

Test orientations (vehicle yaw relative to fall line):

- **S1:** side-slope (pure camber), `δ = 90°`
- **S2:** yawed slightly uphill, `δ = 90° - 15°`
- **S3:** yawed slightly downhill, `δ = 90° + 15°`
- **S4:** aligned straight uphill/downhill, `δ = 0°` or `180°`

Controls (repeat each case):

- `Throttle = 0`, `Brake = 0`
- `Throttle = small constant`, `Brake = 0`
- `Throttle = 0`, `Brake = applied`

## 1.3 Expected behavior + when lateral drift is correct

- **S1:** `F_long,grav ≈ 0`, `|F_lat,grav| = m g sinθ`.
  - Drift is correct if `|F_lat,grav| > μ_s * ΣFz`.
- **S4:** `F_lat,grav ≈ 0`.
  - Any persistent lateral acceleration/drift (with no steering and no aero cross-force) is a red flag.

## 1.4 Telemetry and pass/fail criteria

The solver already emits a per-wheel friction-state CSV via `WriteFrictionStateCsv()`.

### Available signals (grounded in current code)

The CSV header includes (per wheel):

- `VehicleSpeed_ms`
- `Vx_ms`, `Vy_ms`
- `ContactSpeed_ms`
- `WheelOmega_rads`
- `BrakeTorque_Nm`, `ClutchTorque_Nm`, `DriveTorque_Nm`
- `F_Slope_Long_N`, `F_Slope_Lat_N`
- `F_TireFrictionLimit_N` (current breakaway threshold used by the state machine)
- `F_MechHoldLimit_N` (brake-based holding capacity)
- `Fx_Applied_N`, `Fy_Applied_N`
- `WheelLoad_N`
- `bTireCanHoldLong`, `bMechCanHoldLong`

**Important:** `F_Slope_*` is computed from:

- `F_gravity_world = (0,0,-WheelLoad_N)` and then projected onto the ground plane.

Therefore, it is a “slope demand proxy” derived from per-wheel normal load, **not** a direct `m g` projection.

Pass/fail checks (per case, quasi-static low speed):

- **P1 (gravity decomposition correctness; absolute):** compare against `m_total g sinθ` projections in vehicle frame (Section 1.1).
  - **Status with current telemetry:** **UNPROVEN** (requires either logging ground normal `n`/slope angle `θ` and vehicle yaw-to-fallline `δ`, or logging an independent gravity projection built from `m_total` rather than `WheelLoad_N`).

- **P2 (no phantom drift):** in S4, `Vy_ms` must converge to ~0 (tolerance set by deadband) with `Steering=0`.
  - **Status with current telemetry:** provable from `Vy_ms`.

- **P3 (correct static/kinetic boundary; operational):** when the solver claims static hold is possible (via `bTireCanHoldLong` / `bMechCanHoldLong`) and `Vy_ms` is inside deadband, `Fy_Applied_N` must oppose `Vy_ms` and remain bounded by `F_TireFrictionLimit_N`.
  - **Status with current telemetry:** provable from existing fields.

- **P4 (no “slope-lateral injection” in static lock):** when the solver is in its lateral-static regime (indirectly inferable from `Vy_ms` deadband + `Fy_Applied_N` structure), `Fy_Applied_N` must not track `F_Slope_Lat_N` when `Vy_ms ≈ 0`.
  - **Status with current telemetry:** provable.

If any required signal is missing, comparisons that depend on it are **UNPROVEN**.

### Accuracy/Performance/Risk

- **Accuracy impact:** High
- **Performance impact:** Low
- **Risk level:** High (slope bugs masquerade as “tire model” issues)

---

# 2) Roll Transfer & Roll Centers (geometry-derived; required)

## 2.1 Ground truth: why roll centers matter

In the two-path load transfer decomposition used by `ComputeLoadTransferRealtime`:

- **Geometric load transfer** depends on the suspension’s roll center height(s):
  - `ΔFz_geom_axle = (m_axle * a_y * h_RC_axle) / t_axle`
- **Elastic load transfer** depends on the CG-to-roll-center lever arm:
  - `ΔFz_elastic_total = (m_sprung * a_y * (h_CG - h_RC_avg)) / t_avg`

So `h_RC` is not a “tuning number” in this formulation; it is a *geometric property* that directly determines how much lateral load transfer bypasses springs/ARBs.

## 2.2 What the current solver does (grounded in code)

In `ComputeLoadTransferRealtime`, roll centers are currently hard-coded:

- `h_RC_front_m = 0.05`
- `h_RC_rear_m  = 0.10`

Yet the codebase already contains a suspension architecture enum and RC parameters in `FChassisConfiguration`:

- `ESuspensionType` (Telescopic / DoubleWishbone / MacPhersonStrut / MultiLink / SolidAxle / Manual)
- `RollCenterHeightFront_m`, `RollCenterHeightRear_m`

**Interpretation:** the project already has the data model needed to support a geometry-derived (or at least config-derived) roll center, but the active load transfer path does not currently consume it.

## 2.3 Why hard-coded RC heights are invalid long-term

Hard-coding `h_RC` causes systematic errors in:

- **Elastic vs geometric split** (springs/ARB tuning will “compensate” for incorrect `h_RC`)
- **Under/oversteer balance** as suspension geometry changes
- **Ride height effects** (real roll center migrates with bump/rebound)

This also makes ARB stiffness calibration ambiguous because ARB influence is mediated by `(h_CG - h_RC)`.

## 2.4 Conceptual roll center computation from existing geometry (not implemented)

This section defines a *conceptual* procedure to compute `h_RC_front/rear` from suspension hardpoints.

### 2.4.1 Inputs required (per axle, per side)

For any geometry-derived RC, you need hardpoints in the vehicle/suspension plane, typically (names are conceptual):

- **Double wishbone:**
  - Upper arm inner pivots (front/rear) and upper ball joint
  - Lower arm inner pivots (front/rear) and lower ball joint

- **MacPherson strut:**
  - Strut top mount
  - Lower ball joint
  - Lower arm inner pivots

- **Solid axle with Panhard/Watt’s:**
  - Panhard/Watt’s link endpoints to define lateral locating mechanism

### 2.4.2 Front-view instant center method (classic)

For independent suspensions, compute the **instant center (IC)** per side in the *front view*:

- Project relevant arms/links into the front view plane.
- For double wishbone:
  - Define line `L_upper` through (upper inner pivot midpoint → upper ball joint).
  - Define line `L_lower` through (lower inner pivot midpoint → lower ball joint).
  - The intersection `IC = L_upper ∩ L_lower` is the side’s instant center.

Then:

- Construct the **swing arm line** from the tire contact patch center through `IC`.
- Repeat for left and right sides.
- The **roll center** is the intersection of the left and right swing arm lines with the vehicle center plane.

This yields `h_RC` as the Z coordinate (in `[m]`).

### 2.4.3 Mapping to `ESuspensionType`

The existing enum in `FChassisConfiguration` should be treated as the *authoritative selector*:

- `Manual`: use `RollCenterHeight*_m` directly.
- `Telescopic`: approximate RC at (effective) spring top mount height times `TelescopicCorrectionFactor`.
- `DoubleWishbone` / `MacPhersonStrut`: compute IC and RC by the method above.
- `MultiLink`: compute a *virtual* upper/lower arm (requires extra hardpoints or a precomputed kinematic model).
- `SolidAxle`: RC depends on lateral locating device (Panhard/Watt’s); absent link data, axle center is a crude fallback.

## 2.5 Explicit validation conditions + pass/fail criteria

### V1: Lateral load transfer partitioning (flat ground)

Test:

- Flat ground
- Constant speed, constant radius cornering
- Measure `a_y` (body lateral acceleration) and wheel loads

Ground truth prediction:

- Compute `ΔFz_geom_front/rear` and `ΔFz_elastic_front/rear` from the equations in 2.1.

Solver observables:

- `Wheel_LoadTransfer_N[i]` (from telemetry)
- `Wheel_Load_N[i]` vs static loads

Pass/fail:

- The axle-summed left-right deltas must match the predicted `ΔFz_lat_axle` within 10% at steady state.

### V2: Roll center sensitivity sanity

Test:

- Repeat V1 with `h_RC_front_m` increased/decreased by a known amount (configuration-only test).

Expected qualitative behavior:

- Increasing `h_RC_front` must increase the fraction of geometric load transfer on the front axle and reduce elastic share.

Pass/fail:

- `ΔFz_geom_front` must be monotonic in `h_RC_front` for fixed `a_y`.

### Accuracy/Performance/Risk

- **Accuracy impact:** High (RC is a first-order parameter in load transfer split)
- **Performance impact:** Medium (geometry-derived RC requires per-frame or per-state computation; caching/approximation likely required)
- **Risk level:** Medium (implementation complexity; but documentation/validation is low-risk)

---

# 3) Air Intake / Turbo / Manifold Model (design-grade critique + replacement proposal)

## 3.1 Ground truth: what must be modeled

For a turbocharged spark-ignition engine, “boost” felt by the engine is governed by **intake manifold state**, not directly by turbo shaft RPM.

Minimum physical state:

- Manifold absolute pressure: `P_man [Pa]`
- Manifold temperature: `T_man [K]` (or assumed constant as a first-order simplification)
- Manifold volume: `V_man [m³]`

Key constraints:

- **Mass conservation:** `dm_man/dt = m_dot_in - m_dot_out`
- **Ideal gas closure:** `m_man = (P_man * V_man) / (R * T_man)`
- **Compressor power balance** and **turbine power balance** drive shaft dynamics.

Authoritative references for this class of lumped models:

- Heywood, *Internal Combustion Engine Fundamentals* (gas dynamics + engine flow)
- Watson & Janota, *Turbocharging the Internal Combustion Engine*
- Standard 1D compressible orifice flow relations (choked/unchoked)

## 3.2 What the current solver does (grounded in code)

The current turbo implementation in `SolvePowertrain` contains:

- A turbine/compressor power balance with plausible isentropic work equations.
- A shaft acceleration step using `TurboInertia`.

However, the intake air path is not represented as a mass balance system.

Evidence in code:

- “Flow” is computed from an exhaust flow curve scaled by throttle:
  - `MaxFlowRate = Engine.SampleFlowRate(EngineRPM)`
  - `CurrentFlowRate = MaxFlowRate * EffectiveThrottle`
- “Target boost” is computed as a function of turbo RPM through a preset curve:
  - `TargetBoostPressure_bar = Turbocharger.EvalBoostPressure(TurboShaftRPM)`
  - `CompressorPressureRatio = 1.0 + TargetBoostPressure_bar`
- “Boost dynamics” are a first-order filter of that target:
  - `CurrentBoostPressure = FInterpTo(CurrentBoostPressure, PotentialBoost, ...)`

**Critical observation:** there is no explicit `P_man` state integration, and there is no `m_dot_out` term tied to cylinder filling.

## 3.3 Design-grade issues (accuracy)

### I1: Boost is not coupled to engine air demand

In reality, for fixed turbo speed and wastegate, `P_man` depends strongly on:

- Throttle plate restriction
- Engine volumetric efficiency `VE(RPM,MAP)`
- Manifold volume and temperature

The current model uses throttle mainly as a multiplier on “flow” and exhaust temp, but does not compute the resulting manifold pressure from a mass balance.

### I2: Pressure units and meaning are ambiguous

`EvalBoostPressure()` returns `[bar]`, and the solver uses:

- `PressureRatio = 1 + Boost_bar`

This assumes “bar above ambient” with ambient fixed at 1 bar.

If `Boost_bar` is interpreted as absolute bar, this is incorrect. If it is gauge bar, it is still missing `P_ambient` variability (altitude).

### I3: Wastegate/BOV are not physically coupled into turbine/compressor flow

- Wastegate position is computed from boost overshoot but does not feed back into turbine expansion ratio or turbine mass flow.
- BOV affects a filtered boost value, but it does not vent mass from a modeled plenum.

### I4: Compressor map is not used as a compressor map

`TurbochargerSpecifications` includes a `CompressorMap`, but the current turbo stage does not solve for operating point `(m_dot, π_c)` on that map.

## 3.4 Replacement proposal: lumped manifold (MAP) + throttle + VE model

This is a conceptual model intended to replace “boost as filtered curve output” with a **conservation-law system**.

### 3.4.1 State variables

- `P_man [Pa]`
- optionally `T_man [K]` (or assume constant ~ ambient initially)

### 3.4.2 Manifold mass balance

Use ideal gas and mass conservation:

```
m_man = (P_man * V_man) / (R * T_man)
dm_man/dt = m_dot_in - m_dot_out
dP_man/dt = (R * T_man / V_man) * (m_dot_in - m_dot_out)
```

### 3.4.3 Engine air demand (outflow)

For a 4‑stroke engine:

```
m_dot_out = VE(RPM, P_man) * (P_man * V_disp / (R * T_man)) * (RPM / 120)
```

Where:

- `V_disp [m³]` engine displacement
- `RPM/120` is intake events per second for a 4-stroke (`RPM/60` rev/s, `/2` intake per 2 rev)
- `VE` is a calibrated map or curve (at minimum a 2D table `VE(RPM,MAP)`)

### 3.4.4 Throttle / compressor inflow

Model inflow through an effective restriction using compressible orifice flow.

Inputs:

- Upstream pressure `P_up` (compressor outlet or ambient)
- Downstream `P_man`
- Effective area `A_throttle = A_max * f(throttle)`

Then use choked/unchoked mass flow relations (or a simplified quadratic if compressible flow is too expensive).

### 3.4.5 Turbo coupling (minimal viable)

Maintain the existing shaft energy balance concept, but couple it to manifold physics:

- `m_dot_in` comes from compressor map / throttle relation, not from exhaust flow curves.
- Compressor pressure ratio becomes an outcome of `(ω_turbo, m_dot_in)` on a map.
- Wastegate reduces effective turbine mass flow and/or expansion ratio.

## 3.5 Explicit validation conditions + pass/fail criteria

This section defines measurable acceptance criteria comparing:

- **Ground truth expectation** (first principles)
- **Current implementation output**
- **Proposed model output**

### V1: Manifold pressure steady state (step throttle)

Test:

- Fixed RPM (hold via dyno mode / clutch lock)
- Step throttle 0 → 100% and 100% → 0

Ground truth expectation:

- `P_man` rises with a time constant proportional to `V_man` and net `(m_dot_in - m_dot_out)`.

Pass/fail (proposed model):

- `P_man` must asymptotically converge (no oscillation) and remain ≥ ambient.
- Time to reach 63% of final value must be monotonic in `V_man`.

Status with current model:

- **UNPROVEN** because there is no explicit `P_man` state, only `CurrentBoostPressure`.

### V2: Wastegate authority

Test:

- Command a max-boost target and then reduce it (or trigger overboost)

Pass/fail:

- Increasing wastegate position must reduce turbine power and prevent `P_man` overshoot.

Status with current model:

- **UNPROVEN** because wastegate does not couple into turbine power calculation.

### V3: Pumping loss / engine load coupling via MAP

Test:

- Fixed vehicle speed, vary throttle and observe engine braking/pumping behavior

Ground truth expectation:

- At low throttle, manifold pressure falls, pumping torque increases.

Pass/fail:

- Pumping torque term (or an equivalent) must be monotonic with decreasing `P_man` at fixed RPM.

Status with current model:

- Current pumping blend uses `1 - throttle` rather than `P_man`, so MAP-driven pumping is **not modeled**.

### Accuracy/Performance/Risk

- **Accuracy impact:** High (MAP is the correct coupling variable between engine air, turbo, and pumping)
- **Performance impact:** Medium (adds a few scalar states and flow computations; needs careful map evaluation/caching)
- **Risk level:** Medium (more parameters; but validation criteria are straightforward)

---

# 4) Aerodynamics (frame correctness + slopes)

## 4.1 Ground truth

At minimum, aero needs consistent answers to:

- **Drag:** always opposes air-relative velocity.
- **Lift/downforce:** acts in a well-defined direction (usually body `-Z` for a vehicle-fixed aero package).
- **Side force:** should be near zero at zero sideslip/crosswind.

On a slope/bank, **world up** is not equal to **body up**, so applying downforce along world `-Z` injects errors in normal-force and tangential components.

## 4.2 What the current solver does (grounded in code)

In `ComputeAerodynamicForces()`:

- Body aero decomposes velocity by reading **world components**:
  - `Vx = Vel_World.X`, `Vy = Vel_World.Y`, `Vz = Vel_World.Z`
  - This implicitly assumes world axes are body axes.

- Drag vector is applied as:
  - `DragForceWorld = -VelocityDir_World * TotalDrag_N`
  - This is directionally correct (drag opposes airflow).

- Downforce vector is applied as:
  - `LiftForceWorld = (0,0,-TotalDownforce_N)`
  - This assumes “down” is world `-Z`.

- Side force direction is computed using world up:
  - `RightVector = cross((0,0,1), VelocityDir_World)`

## 4.3 Failure modes (when you will see wrong behavior)

- **Bank/slope normal-force error:** downforce is applied world-vertical instead of body-vertical, changing the portion of downforce that actually increases `Fz` on a bank.
- **Bank/slope lateral injection:** because the road normal is tilted, a world-vertical force has a tangential component in the contact plane; this can present as “mysterious” lateral drift/load transfer.
- **Incorrect yaw/crosswind handling:** using `Vel_World.X/Y/Z` to represent longitudinal/lateral/vertical velocity is wrong once the vehicle rotates away from world axes.

## 4.4 Proposed conceptual fix (no code here)

Compute aero in the **vehicle body frame**, then transform to world:

- Obtain `V_world` and `HullXfm`.
- Compute `V_body = HullXfm.InverseTransformVectorNoScale(V_world)`.
- Compute body-axis forces:
  - `F_drag_body` along `-sign(V_body.X)` etc (or simply `-V_body.GetSafeNormal() * D` for drag).
  - `F_down_body = (0,0,-Downforce)` (body-down).
  - `F_side_body` from sideslip/crosswind (body Y).
- Transform: `F_world = HullXfm.TransformVectorNoScale(F_body)`.

Ground-effect terms that depend on ride height should reference ride height along the contact normal (or a filtered chassis up/ground normal) rather than world `Z`.

## 4.5 Explicit validation conditions + pass/fail criteria

### A1: No sideslip, flat ground

Test:

- Flat ground, no wind
- Vehicle aligned with direction of motion (no yaw slip)

Pass/fail:

- Side force must be near zero. Suggested threshold: `|F_side| < 0.01 * F_drag + 25 N`.
- **Status:** likely **UNPROVEN** unless side force is logged.

### A2: Banked road, no sideslip

Test:

- Bank angle `θ`, constant speed
- No steering input and no yaw slip

Pass/fail:

- Downforce should remain mostly normal to the body (vehicle-fixed aero), and the portion contributing to `Fz` should scale with `cos(θ)`.
- Any *new* lateral drift introduced purely by downforce is a fail.
- **Status:** partly provable via wheel loads vs speed if downforce distribution is logged.

### Accuracy/Performance/Risk

- **Accuracy impact:** High
- **Performance impact:** Low (mostly vector transforms)
- **Risk level:** Medium (touches many “feel” aspects; must validate)

---

# 5) Anti‑Roll Bar (ARB) stiffness interpretation (explicit verification)

## 5.1 What is currently true in the codebase

- `FAntiRollbar.Stiffness` is documented as `[N/m]`.
- `ComputeAntiRollbarForces()` uses it as `[N/m]` to generate a force couple.
- `ComputeLoadTransferRealtime()` currently adds `Bar.Stiffness` into `K_phi_ARB_*` as if it were `[N·m]`.

This must be resolved as a single consistent interpretation.

## 5.2 How to verify the intended meaning with a measurement

The solver already stores `LastAntiRollTorque` (in `N·cm`). Use it as the truth signal.

### If `Stiffness` is `[N/m]`

Apply a known left-right suspension displacement difference `Δz` (in meters).

- Predicted vertical force magnitude: `F = k_arb * Δz`.
- Predicted roll torque magnitude about the body longitudinal axis:
  - `τ ≈ F * t` where `t` is track width.

Example:

- `k_arb = 60000 N/m`, `Δz = 0.01 m`, `t = 1.6 m` → `τ ≈ 960 N·m`.

Check:

- `|LastAntiRollTorque| / 100` should be near `τ` when only ARB produces the couple.

### If `Stiffness` is `[N·m]` (roll stiffness)

Define roll angle proxy `φ ≈ Δz / t`.

- `τ = K_phi_ARB * φ`.
- Equivalent linear stiffness would be `k_arb ≈ K_phi_ARB / t^2`.

## 5.3 Pass/fail criteria

- **ARB force-torque consistency:** measured `τ` must match predicted `τ` within 10% across multiple `Δz` values.
- **Load transfer consistency:** the ARB contribution used for elastic load transfer split must be the *same* physical quantity (either convert `k_arb→K_phi` or vice versa).

### Accuracy/Performance/Risk

- **Accuracy impact:** High
- **Performance impact:** None
- **Risk level:** High (misinterpreting ARB units poisons handling balance tuning)

---

# 6) Clutch / Powertrain Coupling (engine feedback + slope load)

## 6.1 Ground truth

- Clutch transmits torque up to a capacity `T_max` and slips when exceeded.
- Slope/drag/rolling resistance should reflect to the engine as a load torque through drivetrain ratio and efficiency.

## 6.2 What the current solver does (grounded in code)

- Clutch engagement is smoothed (`FInterpTo`) and torque is modeled as a bounded slip-proportional element.
- Engine load reflection computes resistive power from:
  - aero drag + rolling resistance + grade resistance
  - then reflects it as `T_load = P_resist / (ω_engine * η)` (with smoothing).

## 6.3 Validation conditions + pass/fail criteria

### PWR1: Constant speed on flat ground

Test:

- Hold steady speed in gear (no acceleration)

Pass/fail:

- `EngineLoadTorque * ω_engine * η` should match `F_resist * V` within 5–10%.

### PWR2: Coast down on a known grade

Test:

- Throttle=0, brake=0, steering=0
- Multiple slope angles

Pass/fail:

- Measured longitudinal decel should match `g sinθ - (F_drag + F_rr)/m` within 10% at moderate speeds.

### PWR3: Clutch lock/unlock transition sanity

Pass/fail:

- Lockup should not create sign-flip impulses in driveline torque.
- SlipRPM should decay monotonically under increasing engagement for fixed wheel speed.

### Accuracy/Performance/Risk

- **Accuracy impact:** Medium
- **Performance impact:** Low
- **Risk level:** Medium

---

# 7) Tire Slip Newton Solver Damping / Stability / Performance

## 7.1 What “damping” is doing here (grounded in code)

The Newton iteration applies a trust-region style clamp:

- `Kappa += clamp(ΔK, -0.15, 0.15)`
- `Alpha += clamp(ΔA, -0.05, 0.05)`

This is not physical damping; it is a numerical stability device that:

- prevents overshoot near peak-force Jacobian singularities
- limits sensitivity to finite-difference noise

## 7.2 Performance reality

Per wheel, per iteration, the solver evaluates combined Pacejka forces multiple times.

The code itself estimates:

- ~12 iterations × ~2 Pacejka calls ≈ 24 MF evaluations per wheel.

## 7.3 Validation criteria

- **NS1 (convergence):** residuals `Res_K`, `Res_A` should converge below tolerance within `MaxIterations` under nominal driving.
- **NS2 (robustness):** no oscillation in `Kappa` sign under steady throttle; no exploding slip under low speeds.

### Accuracy/Performance/Risk

- **Accuracy impact:** High (transient correctness)
- **Performance impact:** Medium (Pacejka call count is dominant)
- **Risk level:** Low (numerically conservative design)

---

# 8) System-Level Validation Matrix (ground truth vs current vs proposed)

The purpose of this matrix is to prevent “feels wrong” debugging. Each subsystem must have a measurable pass/fail.

| Subsystem | Ground truth | Observable / log | Pass/fail | Current status |
|----------|--------------|------------------|-----------|----------------|
| Slope decomposition | `F_parallel = m g sinθ` projected into vehicle axes | `F_Slope_Long_N`, `F_Slope_Lat_N` | match within 10% | **UNPROVEN** (current `F_Slope_*` derives from `WheelLoad_N`) |
| Static drift on aligned incline | `Fy` should not accelerate vehicle when `Vy≈0` | `Vy_ms`, `Fy_Applied_N`, `F_Slope_Lat_N` | `Fy_Applied_N` does not track `F_Slope_Lat_N` when `Vy≈0` | provable |
| ARB units | `τ = k Δz t` or `τ = Kφ φ` consistently | `LastAntiRollTorque` | within 10% across `Δz` | currently ambiguous |
| Aero on slopes | downforce direction is body-fixed, not world-fixed | wheel loads vs speed | monotonic `Fz` vs `V^2` without lateral injection | partly provable |
| Engine load reflection | `P = F_resist V`, `T = P/(ω η)` | telemetry torque/power + speed | within 5–10% at steady speed | provable |
| Turbo/MAP dynamics | manifold mass balance | MAP telemetry | step response time constant behaves with `V_man` | **UNPROVEN** (no explicit MAP state) |

### Accuracy/Performance/Risk

- **Accuracy impact:** High
- **Performance impact:** Low
- **Risk level:** Low (documentation/test definition)

---

# Appendix A) Legacy pass-by-pass notes (superseded)

This appendix is retained for historical detail, but it predates Sections 0–8 and contains assumptions that may be incorrect under the units audit (especially ARB units). Treat it as **non-authoritative**.

## PASS 1: LOAD TRANSFER SYSTEM

### Location: Lines 2282-2630

### 1.1 ComputeLoadTransferLagrange()

**Purpose:** Compute dynamic wheel loads from acceleration and aerodynamic forces.

**Physics Model:**
```
W_i = W_static_i + ΔW_longitudinal + ΔW_lateral + W_aero
```

**PROOF OF CORRECTNESS:**

The implementation uses the fundamental load transfer equation:
```cpp
// Line 2310-2330
const float LongTransfer = (Mass * Ax_ms2 * CoM_Height) / Wheelbase;  // [N]
const float LatTransfer = (Mass * Ay_ms2 * CoM_Height) / TrackWidth; // [N]
```

**Verification (Milliken & Milliken, Ch. 17):**
```
ΔW_long = (m × a_x × h) / L
```
Where:
- m = vehicle mass [kg]
- a_x = longitudinal acceleration [m/s²]
- h = CoM height [m]
- L = wheelbase [m]

**Evidence:** Line 2315 matches the canonical formula exactly.

**ISSUES IDENTIFIED:**

1. **[MINOR] Line 2345:** CoM height hardcoded fallback
   ```cpp
   const float CoM_Height = FMath::Max(CenterOfMass.Z * 0.01f, 0.3f); // [m]
   ```
   **Risk:** 0.3m fallback may be incorrect for lowered vehicles.
   **Recommendation:** Use specification-based default.

2. **[INFO] Line 2380-2400:** Aerodynamic load distribution assumes 50/50 left/right split.
   ```cpp
   Sample.Wheel_AeroDownforce_N[0] = AeroForces.FrontDownforce_N * 0.5f; // FL
   Sample.Wheel_AeroDownforce_N[1] = AeroForces.FrontDownforce_N * 0.5f; // FR
   ```
   **Note:** Correct for symmetric vehicles, may need extension for asymmetric aero.

### 1.2 ComputeSuspensionSprungMasses()
**Location:** Lines 2503-2585

**Purpose:** Distribute total sprung mass to individual wheels based on CoM position.

**Physics Model:** Lagrange multiplier optimization for static equilibrium.

**PROOF OF CORRECTNESS:**

For N wheels, the problem is:
```
Minimize: Σ(m_i - m_avg)²
Subject to: Σm_i = M_total
            Σ(m_i × x_i) = M_total × x_CoM
            Σ(m_i × y_i) = M_total × y_CoM
```

**Implementation Verification (Line 2562-2572):**
```cpp
const float Det = (XDotX * YDotY * Count) + (2.0f * XDotY * SumX * SumY)
                - (YDotY * SumX * SumX) - (XDotX * SumY * SumY)
                - (XDotY * XDotY * Count);
```

This is the determinant of the 3×3 constraint matrix - correct application of Cramer's rule.

**Special Cases Handled:**
- Count = 1: Single spring → all mass (Line 2516)
- Count = 2: Linear interpolation (Line 2522)
- Count ≥ 3: Full Lagrange multiplier solution

**Grade: A** - Mathematically rigorous implementation.

---

## PASS 2: ANTI-ROLL BAR FORCES

### Location: Lines 2633-2695

**Purpose:** Compute roll-resisting torque from ARB torsional spring.

**Physics Model:**
```
τ_ARB = K_arb × Δz + C_arb × Δż
```
Where:
- K_arb = torsional stiffness [N·m/rad]
- Δz = left-right displacement difference [m]
- C_arb = damping coefficient [N·m·s/rad]

**PROOF OF CORRECTNESS (Line 2674-2686):**
```cpp
const float DeltaDispM = (AxleData.SpringDisplacements[LeftIdx] -
                          AxleData.SpringDisplacements[RightIdx]) * 0.01f; // [m]
const float DeltaVelMs = (AxleData.SpringVelocities[LeftIdx] -
                          AxleData.SpringVelocities[RightIdx]) * 0.01f;    // [m/s]
const float TotalForceN = Bar.Stiffness * DeltaDispM + Bar.Damping * DeltaVelMs; // [N]
```

**Verification:** Matches standard ARB model from Genta & Morello (2009), Ch. 5.

**Torque Application (Line 2683-2686):**
```cpp
TotalTorqueCm += FVector::CrossProduct(
    AxleData.WorldSpringOrigins[RightIdx] - AxleData.WorldSpringOrigins[LeftIdx],
    WorldSuspensionNormal * (TotalForceN * 100.0f)
);
```

**Physics Proof:**
- Cross product of track vector × force gives roll torque about longitudinal axis
- Negative sign in Line 2693 applies restoring torque

**Grade: A** - Correct implementation with proper damping.

---

## PASS 3: STEERING SYSTEM

### Location: Lines 2697-3003

### 3.1 Variable Steering Ratio
**Lines 2770-2802**

**Research Basis (documented in code):**
- SAE J2564 (2011) Power Steering Systems
- BMW Active Steering: 10:1 @ parking → 16:1 @ 180 km/h
- Porsche Dynamic Steering: 12:1 → 17.5:1

**Implementation:**
```cpp
const float SmoothT = NormalizedSpeed * NormalizedSpeed * (3.0f - 2.0f * NormalizedSpeed);
SpeedFactor = FMath::Lerp(1.0f, SteeringSystem.VariableRatioMinFactor, SmoothT);
```

**PROOF:** Smoothstep function 3t² - 2t³ provides C1 continuity (continuous first derivative), matching real hydraulic/electric steering feel.

### 3.2 Stability Control (ESC)
**Lines 2837-2909**

**Physics Model:**
```
β = atan(v_lateral / v_longitudinal)  // Slip angle
Authority = f(β / β_critical)          // Progressive reduction
```

**Implementation Verification (Lines 2884-2908):**
```cpp
const float Beta_rad = FMath::Atan2(Rec.ν_lateralMs, FMath::Max(Vx_sc, VXLOW_SC));
const float SafetyFactor = FMath::GetMappedRangeValueClamped(
    FVector2D(InterventionThreshold, Beta_critical),
    FVector2D(1.0f, SteeringSystem.StabilityMinAuthority),
    FMath::Abs(Beta_deg)
);
```

**Research Reference:** Bosch ESP 9.0 activates at β > 3-5° (documented Line 2737).

**ISSUE IDENTIFIED:**

**[MEDIUM] Low-Speed Singularity Guard (Lines 2876-2883):**
```cpp
constexpr float VXLOW_SC = 0.5f;  // [m/s]
constexpr float Vth_SC = 1.0f;    // [m/s]
```

The smoothstep blend prevents atan2 singularity but creates a "dead zone" at very low speeds where ESC cannot accurately measure slip angle.

**Recommendation:** Document this as intended behavior for parking maneuvers.

### 3.3 Ackermann Geometry
**Lines 2919-2997**

**Physics Model:**
```
R = L / tan(δ)                    // Turn radius
δ_inner = atan(L / (R - w/2))     // Inner wheel angle
δ_outer = atan(L / (R + w/2))     // Outer wheel angle
```

**Implementation (Lines 2943-2952):**
```cpp
const float R = WheelBase / FMath::Tan(steerRadAbs);
const bool bIsInnerWheel = (steerSign > 0.0f && bRightSide) || (steerSign < 0.0f && !bRightSide);
const float WheelR = bIsInnerWheel ? FMath::Abs(R - HalfTrack) : R + HalfTrack;
WheelAngleDeg = steerSign * FMath::RadiansToDegrees(FMath::Atan(WheelBase / FMath::Max(WheelR, SMALL_NUMBER)));
```

**PROOF:** Matches standard Ackermann geometry from Rajamani (2012), Ch. 2.

**Configurations Supported:**
- FrontAckerman (default)
- RearAckerman
- FourWheelAckerman
- FourWheelParallel
- CrabSteer

**Grade: A** - Production-quality steering implementation.

---

## PASS 4: TIRE SLIP SOLVER

### Location: Lines 3005-3291

### 4.1 Implicit Newton Solver
**Lines 3088-3200**

**CRITICAL INNOVATION:**

Traditional kinematic slip:
```
κ = (Ω·R - Vx) / max(|Ω·R|, |Vx|)  // Explicit, one-frame lag
```

This implementation uses implicit Newton-Raphson:
```
Find κ such that: κ = (Ω_next·R - Vx) / Denom
Where: Ω_next = Ω + (T_net - F_x(κ)·R) / I · Δt
```

**PROOF OF SUPERIORITY (documented Lines 3093-3113):**

| Property | Kinematic | Newton (This) |
|----------|-----------|---------------|
| Transient lag | 1 frame | 0 frames |
| Peak force stability | Oscillates | Stable |
| Discontinuous torque | Glitches | Handles |
| Low-speed behavior | Singular | Robust |

**Convergence Parameters (Lines 3123-3131):**
```cpp
constexpr float Eps = 1e-4f;              // Finite difference step
constexpr int32 MaxIterations = 12;       // Iteration limit
constexpr float ConvergenceTol = 5e-5f;   // Tolerance
constexpr float KAPPA_MAX_BRAKE = -1.0f;  // Locked wheel limit
constexpr float KAPPA_MAX_DRIVE = 1.5f;   // Spin limit
```

**ISSUE IDENTIFIED:**

**[LOW] Line 3185-3194:** Regularized gradient descent at singularity
```cpp
if (FMath::Abs(Det) < 1e-9f)
{
    const float RegularizedSlope = (dResK_dK >= 0.0f ? 1.0f : -1.0f) *
                                   FMath::Max(FMath::Abs(dResK_dK), 1e-4f);
    DeltaK = -Res_K / RegularizedSlope;
}
```

**Risk:** At exact peak force (Jacobian = 0), this may cause slow convergence.
**Mitigation:** Clamped step size (Line 3197) prevents divergence.

### 4.2 Transient Relaxation
**Lines 3213-3245**

**Physics Model (Pacejka 2012, Section 4.3):**
```
τ = σ / V                           // Time constant [s]
F_transient += α × (F_steady - F_transient)
Where: α = Δt / τ
```

**Implementation (Lines 3228-3242):**
```cpp
const float Sigma_Long = TireModel.RelaxationLengthLong *
                         FMath::Pow(LoadRatio, TireModel.RelaxationLoadExponentLong);
const float Tau_Long = Sigma_Long / FMath::Max(V_contact, 0.5f);
const float Alpha_Long = FMath::Clamp(DeltaTime / Tau_Long, 0.0f, 1.0f);
AxleData.Fx_Transient[WheelIndex] += Alpha_Long * (Fx_steady - AxleData.Fx_Transient[WheelIndex]);
```

**PROOF:** First-order lag with load-dependent relaxation length matches MF-Swift (Schmeitz et al., 2004).

**Grade: A** - State-of-the-art tire dynamics.

---

## PASS 5: PACEJKA MF6.1 IMPLEMENTATION

### Location: Lines 3293-3651

### 5.1 Longitudinal Force (Fx)
**Lines 3297-3358**

**Magic Formula:**
```
Fx = Dx × sin(Cx × atan(Bx×κ' - Ex×(Bx×κ' - atan(Bx×κ')))) + SVx
Where: κ' = κ + SHx
```

**Implementation Verification (Lines 3351-3354):**
```cpp
const float Bx_x = Bx * EffectiveSlip;
const float InnerTerm = Bx_x - Ex * (Bx_x - FMath::Atan(Bx_x));
const float SineComponent = FMath::Sin(Cx * FMath::Atan(InnerTerm));
const float PacejkaForce = Dx * SineComponent + SVx;
```

**EXACT MATCH** to Pacejka (2012) Eq. 4.E1.

**Load Sensitivity Correction (Lines 2332-2334):**
```cpp
const float LoadFactorLong = FMath::Max(1.0f - Cache.MuLoadSensitivityLong * FMath::Abs(dfz),
                                        Cache.MuLoadMinFactor);
const float Dx = Dx_nominal * LoadFactorLong;
```

**Physics Basis:** Normalized load sensitivity per Besselink et al. (2010).

### 5.2 Self-Aligning Torque (Mz)
**Lines 3445-3565**

**CRITICAL FIX DOCUMENTED (Lines 3497-3508):**
```cpp
// PROBLEM: FMath::Tan(LongitudinalSlip/2) → ∞ as slip → π
// SOLUTION: Clamp tan() input to elastic deformation domain
constexpr float MaxSlipInput = 1.5f;
const float ClampedSlip = FMath::Clamp(HalfSlip, -MaxSlipInput, MaxSlipInput);
const float SlipAngleCorrection = FMath::Atan(StiffnessRatio * FMath::Tan(ClampedSlip));
```

**High-Slip Fade (Lines 3538-3548):**
```cpp
const float TotalSlip = FMath::Sqrt(FMath::Square(SlipAngleRad) + FMath::Square(LongitudinalSlip));
const float BurnoutFade = FMath::GetMappedRangeValueClamped(
    FVector2D(1.0f, 2.5f),   // Slip range
    FVector2D(1.0f, 0.0f),   // Fade factor
    TotalSlip);
const float TrailEffective_m = Trail_m * BurnoutFade;
```

**Physics Justification (documented):** At extreme slip, contact patch forces become uniform → no offset → zero aligning torque.

**Grade: A** - Proper handling of combined slip edge cases.

### 5.3 Combined Slip Weighting
**Lines 3568-3651**

**MF6.1 Ellipse Model (Pacejka 2012, Eq. 4.E45-4.E49):**
```cpp
// Line 3627-3632
G_xAlpha = FMath::Cos(C.rCx1 * FMath::Atan(C.rBx1 * alpha_s));
const float rBy = C.rBy1 * FMath::Cos(FMath::Atan(C.rBy2 * (SlipAngleRad - C.rBy4)));
G_yKappa = FMath::Cos(C.rCy1 * FMath::Atan(rBy * kappa_normalized));
```

**PROOF:** Weighting functions reduce pure slip forces as combined slip increases - standard friction ellipse behavior.

---

## PASS 6: POWERTRAIN SOLVER

### Location: Lines 3656-5241

### 6.1 Turbocharger
**Lines 3728-3904**

**Thermodynamic Model:**

**Turbine Power (Isentropic Expansion):**
```cpp
// Line 3781-3782
float IsentropicWork_turbine = Cp_exhaust_actual * CurrentExhaustTemp
    * (1.0f - FMath::Pow(1.0f / TurbineExpansionRatio, (Gamma_exhaust - 1.0f) / Gamma_exhaust));
```

**PROOF (Heywood 1988, Ch. 6):**
```
W_isentropic = Cp × T1 × [1 - (P2/P1)^((γ-1)/γ)]
```
The implementation matches the canonical isentropic expansion formula.

**Variable Efficiency Model (Lines 3785-3787):**
```cpp
float BladeSpeedRatio = (TurboOmega * 0.025f) / FMath::Sqrt(FMath::Max(CurrentExhaustTemp, 300.0f));
float TurbineEfficiency_actual = DrivetrainSpecs_PT.Turbocharger.TurbineEfficiency
    * FMath::Clamp(0.85f + 0.15f * BladeSpeedRatio, 0.70f, 1.0f);
```

**Physics Basis:** Blade speed ratio affects turbine efficiency (Dixon 2013).

**Internal Heat Transfer (Lines 3827-3831):**
```cpp
float DeltaT_housing = TurbineHousingTemp - CompressorHousingTemp;
constexpr float HeatTransferCoeff = 45.0f;  // [W/K]
float InternalHeatTransfer = HeatTransferCoeff * DeltaT_housing;
```

**Reference:** Serrano et al. (2007), Burke et al. (2015) - documented in header.

### 6.2 Engine Integration
**Lines 3956-4006**

**RK4 Integration (Decoupled Mode):**
```cpp
const float Alpha_k1 = EvalNetTorque(RPM_k1) * InvInertia;
const float Omega_k2 = InitialOmega + (Alpha_k1 * DeltaTime * 0.5f);
const float Alpha_k2 = EvalNetTorque(RPM_k2) * InvInertia;
// ... k3, k4
const float AlphaAverage = (Alpha_k1 + 2.0f*Alpha_k2 + 2.0f*Alpha_k3 + Alpha_k4) / 6.0f;
```

**PROOF:** Standard 4th-order Runge-Kutta with correct weights (1, 2, 2, 1)/6.

**Lumped Mode (Clutch Locked):**
```cpp
// Line 3979-3981
const float NetTorque = ThrottleTorque + BrakingTorque - LoadTorque;
const float CommonAlpha = NetTorque / TotalSystemInertia;
NewEngineOmega = EngineOmega + (CommonAlpha * DeltaTime);
```

**Physics Basis:** When clutch is locked, entire drivetrain rotates as rigid body.

### 6.3 Clutch Dynamics
**Lines 4067-4203**

**Progressive Engagement (Lines 4130-4136):**
```cpp
float InterpSpeed = (TargetEngagement < ClutchEngagement) ? 15.0f : 5.0f;
ClutchEngagement = FMath::FInterpTo(ClutchEngagement, TargetEngagement, DeltaTime, InterpSpeed);
```

**Design Rationale (documented):**
- Open fast (15 Hz): Protect engine from stall
- Close slow (5 Hz): Smooth engagement, no torque spikes

**Stiffness-Based Slip Model (Lines 4154-4164):**
```cpp
constexpr float BreakawaySlipRPM = 300.0f;
constexpr float BreakawaySlipRadS = BreakawaySlipRPM * (2.0f * PI / 60.0f);
const float ClutchStiffness = CurrentCapacity / FMath::Max(BreakawaySlipRadS, 1.0f);
ClutchTorque_Nm = FMath::Clamp(SlipOmega * ClutchStiffness, -CurrentCapacity, CurrentCapacity);
```

**Tuning Note:** BreakawaySlipRPM controls feel - lower = snappier (race), higher = smoother (street).

### 6.4 Differential Solver
**Lines 4398-4518**

**Universal Solver Lambda:**
```cpp
auto SolveDifferential = [&](float InputTorque, float OmegaA, float OmegaB,
                             const FDifferentialSpecifications& Specs, float SteeringInput)
```

**Output Calculation (Lines 4512-4515):**
```cpp
float TorqueA = (InputTorque * 0.5f) - NetTransfer;
float TorqueB = (InputTorque * 0.5f) + NetTransfer;
```

**Physics Basis:** Standard differential equation T_left + T_right = T_input, T_left - T_right = 2×T_transfer.

**ISSUE IDENTIFIED:**

**[LOW] Deadband Implementation (Lines 4428-4433):**
```cpp
constexpr float LSDDeadband = 0.5f; // [rad/s]
if (FMath::Abs(DiffOmega) > LSDDeadband)
{
    const float EffectiveDiffOmega = DiffOmega - (FMath::Sign(DiffOmega) * LSDDeadband);
    LockingTorque = (EffectiveDiffOmega * Specs.LSDLockingCoefficient) + ...
}
```

**Observation:** Deadband is discontinuous at threshold crossing.
**Recommendation:** Consider smoothstep transition for smoother response.

**Grade: B+** - Solid implementation, minor smoothness improvements possible.

---

## PASS 7: STICTION STATE MACHINE

### Location: Lines 4653-5053

### 7.1 State Definitions
```cpp
enum class ETireState
{
    Kinetic,          // Pacejka active
    Static_Lateral,   // Lateral hold, longitudinal kinetic
    Static_Full       // Complete static friction hold
};
```

### 7.2 Transition Logic
**Lines 4839-4861**

```cpp
if (v_contact_speed < STOP_VELOCITY_THRESHOLD)  // 0.2 m/s
{
    if (FMath::Abs(F_Slope_Lat_N) < F_TireFrictionLimit_N)  // Lateral grip OK
    {
        const float F_FrictionRemaining_N = FMath::Sqrt(
            FMath::Max(0.0f, FMath::Square(F_TireFrictionLimit_N) - FMath::Square(F_Slope_Lat_N)));

        bTireCanHoldLong = FMath::Abs(F_Slope_Long_N) < F_FrictionRemaining_N;
        bMechCanHoldLong = FMath::Abs(F_Slope_Long_N) < F_MechanicalHoldLimit_N;

        if (bTireCanHoldLong && bMechCanHoldLong)
            CurrentState = ETireState::Static_Full;
        else
            CurrentState = ETireState::Static_Lateral;
    }
}
```

**PROOF OF FRICTION CIRCLE:**
```
F_available = √(F_max² - F_used_lat²)
```
Correct application of friction circle for remaining longitudinal capacity.

### 7.3 Force Application
**Lines 4875-4916**

**Grip Budget System:**
```cpp
const float F_Available_N = Fz * MU_STATIC;
const float F_Hold_Mag = FMath::Sqrt(Fx_Hold * Fx_Hold + Fy_Hold * Fy_Hold);
const float F_Budget_For_Damp = FMath::Max(0.0f, F_Available_N - F_Hold_Mag);

// Clamp damping to remaining budget
if (F_Damp_Mag > F_Budget_For_Damp && F_Damp_Mag > KINDA_SMALL_NUMBER)
{
    const float Scale = F_Budget_For_Damp / F_Damp_Mag;
    Fx_Damp *= Scale;
    Fy_Damp *= Scale;
}
```

**Design Rationale:** Prioritize slope holding over damping - vehicle stays put even if it takes longer to stop residual motion.

### 7.4 Brake Acceleration Guard
**Lines 4942-4958**

**CRITICAL FIX:**
```cpp
if (BrakeState.CurrentTorque > 50.0f)
{
    if (Vx_Local > 0.1f && Fx_Final_N > 0.0f)      // Moving forward, force forward
        Fx_Final_N = 0.0f;                          // WRONG - clamp to zero
    else if (Vx_Local < -0.1f && Fx_Final_N < 0.0f) // Moving backward, force backward
        Fx_Final_N = 0.0f;                          // WRONG - clamp to zero
}
```

**Purpose:** Catches solver edge case where Pacejka output pushes vehicle in wrong direction under heavy braking.

**Grade: A-** - Robust stiction with proper edge case handling.

---

## PASS 8: AERODYNAMICS

### Location: Lines 4615-4649 (application), AerodynamicSpecifications.h (model)

### 8.1 Force Application
```cpp
// Line 4644-4645
TotalForceCm += AeroForces.DragForceWorld * 100.0f;  // Drag only to chassis
// NOTE: Downforce NOT applied here - already in wheel loads
```

**CRITICAL DESIGN:**
- Downforce → Wheel loads (ComputeLoadTransferRealtime) → Increases grip
- Drag → Chassis force → Decelerates vehicle
- Prevents double-counting

### 8.2 Ground Effect Model
**From AerodynamicSpecifications.h:**
```cpp
if (h_m > h_opt)
    Modifier = (h_opt / h_m)^1.2;         // Above optimal
else if (h_m < h_crit)
    Modifier = h_m / h_crit;               // Porpoising zone
else
    Modifier = Lerp(0.6, 1.0, t);          // Transition
```

**Physical Basis:** Ground effect venturi efficiency decreases with ride height (inverse power law) and collapses below critical height (flow separation).

**Grade: A** - Proper force accounting and ground effect model.

---

## PASS 9: ENGINE LOAD REFLECTION

### Location: Lines 5104-5156

**Purpose:** Reflect road resistance back to engine to create realistic load feeling.

**Implementation (Lines 5128-5151):**
```cpp
const float TotalResistForce_N = AeroDragForce_N + TotalRollingResistanceForce_N + F_grade_resist_N;
const float ResistivePower_W = TotalResistForce_N * SpeedForwardAbs;

if (ClutchState.ClutchEngagement > 0.5f && RatioAbs > 1e-3f && EngineOmega_now > 1.0f)
{
    const float TargetLoad = ResistivePower_W / (EngineOmega_now * Eff);
    EngineLoadFromResist_Nm = FMath::FInterpTo(EngineState.EngineLoadTorque, NewLoadTarget, DeltaTime, 5.0f);
}
```

**CRITICAL FIX (Lines 5121-5125):**
```cpp
// Grade resistance must OPPOSE motion regardless of direction
const float F_grade_resist_N = FMath::Max(0.0f, -Fg_forward_N * FMath::Sign(V_forward_ms));
```

**Physics Proof:**
- Uphill forward: Fg_forward_N < 0, V > 0 → F_grade_resist_N > 0 (opposes motion) ✓
- Downhill forward: Fg_forward_N > 0, V > 0 → F_grade_resist_N = 0 (gravity aids) ✓

**Grade: A** - Correct sign handling for all slope/direction combinations.

---

## PASS 10: NUMERICAL STABILITY

### 10.1 Division Guards
**Identified Safe Divisions:**
- Line 2562: `if (FMath::IsNearlyZero(Det, SUSPENSION_SMALL_NUMBER)) return false;`
- Line 2612: `if (FMath::Abs(Aug[i][i]) < KINDA_SMALL_NUMBER) return Solution;`
- Line 3149: `const float Denom = FMath::Max3(FMath::Abs(Vx_Base), FMath::Abs(WheelSpeed_New), MinSpeed);`
- Line 3233: `const float Tau_Long = Sigma_Long / FMath::Max(V_contact, 0.5f);`

### 10.2 Clamp Ranges
| Parameter | Min | Max | Location |
|-----------|-----|-----|----------|
| κ (slip ratio) | -1.0 | 1.5 | 3129-3130 |
| α (slip angle) | -1.48 rad | 1.48 rad | 3131 |
| Engine RPM | Idle | Redline | 4030 |
| Clutch engagement | 0.0 | 1.0 | 4136 |
| Oil temperature | Ambient | Damage+20 | 5226-5228 |

### 10.3 POTENTIAL ISSUE

**[MEDIUM] Line 3847:** Turbo omega division
```cpp
float DrivingTorque = (NetPower / FMath::Max(TurboOmega, 1.0f));
```

**Risk:** At very low turbo RPM (< 1 rad/s), torque can spike.
**Current Mitigation:** `FMath::Max(TurboOmega, 1.0f)` prevents division by zero.
**Recommendation:** Consider smoothstep transition as TurboOmega → 0.

**Grade: A-** - Comprehensive guards with one minor improvement opportunity.

---

## PASS 11: CODE QUALITY

### 11.1 Documentation
- **Excellent:** Physics equations documented with LaTeX-style notation
- **Excellent:** Research references inline (Pacejka, Heywood, etc.)
- **Excellent:** "Reason:" comments explain conditional logic
- **Good:** Unit annotations `[N]`, `[m/s]`, `[rad]` throughout

### 11.2 Structure
- **Concern:** Single 5683-line file
- **Recommendation:** Consider splitting into:
  - `VehicleSolver_Tire.cpp` (Pacejka, slip solver)
  - `VehicleSolver_Powertrain.cpp` (Engine, trans, diff)
  - `VehicleSolver_Chassis.cpp` (Suspension, ARB, load transfer)
  - `VehicleSolver_Telemetry.cpp` (CSV logging, debug)

### 11.3 Naming Conventions
- **Consistent:** `*_PT` suffix for physics thread data
- **Consistent:** `*_N`, `*_Nm`, `*_ms` for units in variable names
- **Good:** CamelCase for functions, snake_case discouraged

### 11.4 Magic Numbers
Most constants are named, but some remain:
- Line 4757: `0.05f * Omega` (airborne drag) - Could be `AirborneWheelDragCoeff`
- Line 4997: `MIN_HOLDING_TORQUE` used for rolling resistance - Rename or document dual use

**Grade: B+** - Well-documented, would benefit from file splitting.

---

## SUMMARY OF ISSUES

### Critical (0)
None identified.

### High Priority (0)
None identified.

### Medium Priority (2)
1. **Line 3847:** Turbo torque spike at very low RPM - add smoothstep
2. **Line 4428-4433:** Differential deadband discontinuity - add transition

### Low Priority (4)
1. **Line 2345:** Hardcoded CoM height fallback
2. **Line 3185-3194:** Peak force regularization may slow convergence
3. **Line 2876-2883:** ESC low-speed dead zone undocumented
4. **Line 4997:** MIN_HOLDING_TORQUE naming ambiguity

### Informational (2)
1. **Line 2380-2400:** 50/50 aero split assumption
2. **Lines 5683:** Single large file structure

---

## RECOMMENDATIONS

### Immediate
1. Add smoothstep to turbo torque calculation at low RPM
2. Document ESC behavior at parking speeds

### Near-Term
1. Split VehicleSolver.cpp into subsystem files
2. Add differential deadband smoothing
3. Rename MIN_HOLDING_TORQUE to clarify dual purpose

### Future
1. Consider adaptive Newton iteration count based on slip magnitude
2. Add runtime validation for physics thread data consistency
3. Implement tire thermal model integration (placeholders exist)

---

## CONCLUSION

VehicleSolver.cpp represents a **professional-grade vehicle dynamics implementation** suitable for AAA racing simulation. The physics models are research-backed, numerically stable, and well-documented. The implicit Newton slip solver is a notable innovation over traditional kinematic approaches.

**Production Readiness:** YES
**Technical Debt:** LOW
**Recommended Action:** Address medium-priority items before public release.

---

*Review conducted using static analysis and physics verification against canonical references.*
