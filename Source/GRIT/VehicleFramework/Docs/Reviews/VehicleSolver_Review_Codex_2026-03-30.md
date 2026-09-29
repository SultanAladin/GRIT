# VehicleSolver Engineering Review

**Files**: `VehicleSolver.cpp`, `VehicleSolver.h`, `Controllers/VehicleController.cpp`, `Components/AntiRollbarSpecifications.h`, `Components/ChassisConfiguration.h`
**Reviewer**: Codex (GPT-5)
**Review Date**: 2026-03-30
**Review ID**: CODEX_VEHICLESOLVER_2026-03-30_RTNET
**Previous Review**: `Docs/CoreReview/VehicleSolver_Review_Kiro_2025-12-07.md`
**Scope**: Full audit of current vehicle physics for mathematical correctness, real-time scalability, and multiplayer responsiveness.

---

## Executive Summary

| Metric | Score | Status |
|--------|-------|--------|
| Mathematical Accuracy | 56% | 🔴 |
| Unit Consistency | 58% | 🔴 |
| Physics Validity | 60% | ⚠️ |
| Code Quality | 79% | ⚠️ |
| Numerical Stability | 74% | ⚠️ |
| Real-Time Scalability | 57% | 🔴 |
| **OVERALL RATING** | **61%** | **⭐⭐☆☆☆** |

**Key Findings**: The solver has several strong foundations: a custom physics-thread pipeline, good use of SoA data, precomputed Pacejka caches, and a serious attempt at transient tire dynamics. However, the current snapshot still contains three ship-blocking correctness issues for an accuracy-focused driving game: lateral load transfer is signed incorrectly, body aero is resolved in world axes instead of body axes, and slope-hold logic derives gravity from normal load rather than true mass. Multiplayer is also not yet optimized for a responsive client-driven experience because non-authority clients run no local vehicle physics and only interpolate replicated transforms.

---

## PASS 1: NETWORK REPLICATION & PLAYER RESPONSIVENESS (VehicleSolver.cpp 96-217, VehicleController.cpp 128-156, 297-318, 522-531)

### Function Rating
**Overall Score**: 45%  
**Star Rating**: ⭐☆☆☆☆  
**Reviewed By**: Codex on 2026-03-30  
**Status**: 🔴 Critical Issues

---

### Code Snippet
```cpp
if (bIsClient)
{
    if (VehicleHull)
    {
        VehicleHull->SetSimulatePhysics(false);
    }
    return;
}

if (GetLocalRole() != ROLE_Authority && VehicleHull)
{
    const FVector NewLoc = FMath::VInterpTo(CurrentLoc, ReplicatedLocation, DeltaTime, InterpSpeed);
    const FRotator NewRot = FMath::RInterpTo(CurrentRot, ReplicatedRotation, DeltaTime, InterpSpeed);
    VehicleHull->SetWorldLocation(NewLoc);
    VehicleHull->SetWorldRotation(NewRot);
    return;
}
```

---

### Mathematical Analysis

#### Equation 1.1: Replication Update Pressure
**Analytical Form**:
```text
U = N_vehicles × N_receivers × f_update
```

**Literature Reference**:
- **Source**: Unreal Engine network replication model; project-local `Docs/Networking/P2P_Physics_Replication_Guide.md`
- **Type**: Engine implementation guidance

**Unit Analysis**:
| Variable | Units | Dimensional Formula |
|----------|-------|---------------------|
| `N_vehicles` | count | 1 |
| `N_receivers` | count | 1 |
| `f_update` | Hz | T^-1 |
| **Result** | updates/s | T^-1 |

**Physics / Networking Validation**:
- ✅ Authority is centralized, which is safe for cheating and divergence.
- ❌ Autonomous proxies do not run local physics, so the owning player gets server-latency-driven control.
- ❌ `bAlwaysRelevant = true` plus `60 Hz` update cadence scales poorly for many cars.

**Numerical Test**:
```text
Given: 50 vehicles, 16 receivers, 60 Hz
Expected update pressure: 50 × 16 × 60 = 48,000 replicated vehicle updates/s
Status: ⚠️ High bandwidth / relevancy pressure for a large session
```

---

### Issues & Concerns

#### 🔴 Critical Issues (P0 - Ship Blocker)
1. **Owning clients have no local vehicle simulation** (`VehicleSolver.cpp:125-129`, `VehicleSolver.cpp:184-201`)
   - **Problem**: Non-authority clients disable physics and only interpolate replicated transforms.
   - **Impact**: Local steering, throttle, and braking feel RTT-limited instead of immediate.
   - **Root Cause**: `ROLE_AutonomousProxy` is treated like a simulated proxy rather than a predicted proxy.
   - **Fix**: Add local prediction plus server reconciliation, or use Unreal's networked physics path instead of pure transform replication.
   - **Verification**: Test a remote client at 60-100 ms RTT and compare steering response before/after prediction.

#### ⚠️ High Priority Issues (P1 - Accuracy / Scalability)
1. **All vehicles are always relevant at 60 Hz** (`VehicleSolver.cpp:28-30`)
   - **Problem**: `bAlwaysRelevant = true`, `SetNetUpdateFrequency(60.0f)`, and `SetMinNetUpdateFrequency(30.0f)` are applied globally.
   - **Impact**: Poor scaling with many cars; bandwidth is spent on distant vehicles that do not need 60 Hz state.
   - **Fix**: Use relevancy, distance-based frequency reduction, or compact replicated state.

2. **Extra replicated state is sent but not consumed by clients** (`VehicleSolver.cpp:64-65`, `VehicleSolver.cpp:197-198`)
   - **Problem**: `ReplicatedVelocity` and `ReplicatedAngularVelocity` replicate, but current client interpolation only uses location and rotation.
   - **Impact**: Wasted bandwidth.
   - **Fix**: Either consume the extra state for Hermite/extrapolated smoothing or stop replicating it.

#### 💡 Medium Priority Issues (P2 - Polish)
1. **Server RPC chains into another server RPC** (`VehicleController.cpp:522-531`)
   - **Problem**: `ServerSendInput_Implementation()` forwards to `ControlledVehicle->ServerUpdateInput(NewInput)`.
   - **Impact**: Extra indirection and harder-to-reason ownership path.
   - **Fix**: Apply input directly on the server-owned vehicle from the controller RPC.

---

### Code Quality Assessment

**Readability**: 8/10
- ✅ Intent is documented clearly.
- ⚠️ Network-role handling is spread across controller and pawn, which makes authority flow easy to misread.

**Maintainability**: 6/10
- ✅ Good separation between controller input and vehicle solver.
- ❌ Current networking architecture will need a redesign for serious multiplayer.

**Performance**: 4/10
- ❌ Current replication model is not suitable for "many cars" at low latency.

---

## PASS 2: LOAD TRANSFER MODEL (VehicleSolver.cpp 1617-1887, AntiRollbarSpecifications.h 20, ChassisConfiguration.h 83/103)

### Function Rating
**Overall Score**: 48%  
**Star Rating**: ⭐☆☆☆☆  
**Reviewed By**: Codex on 2026-03-30  
**Status**: 🔴 Critical Issues

---

### Code Snippet
```cpp
const float dFz_geom_front_total = (m_front_total * a_y * h_RC_front_m) / FMath::Max(t_front_m, 0.1f);
const float dFz_lat_front = dFz_geom_front_total + dFz_elastic_front;
const float signLat = FMath::Sign(a_y);

AxleData.ComputedLoads_LT[0] = AxleData.StaticLoads_LT[0]
                             - signLat * (dFz_lat_front * 0.5f);
AxleData.ComputedLoads_LT[1] = AxleData.StaticLoads_LT[1]
                             + signLat * (dFz_lat_front * 0.5f);
```

---

### Mathematical Analysis

#### Equation 2.1: Lateral Load Transfer
**Analytical Form**:
```text
ΔFz = (m × a_y × h) / t
```

**Literature Reference**:
- **Source**: Milliken & Milliken, *Race Car Vehicle Dynamics*, 3rd ed.
- **Source**: Rill & Castro, *Road Vehicle Dynamics*, 2nd ed.
- **Equation Number**: standard lateral load transfer relationship

**Unit Analysis**:
| Variable | Units | Dimensional Formula |
|----------|-------|---------------------|
| `m` | kg | M |
| `a_y` | m/s² | L T^-2 |
| `h` | m | L |
| `t` | m | L |
| **Result** | N | M L T^-2 |

**Verification**:
```text
kg × (m/s²) × m / m = kg·m/s² = N
✅ Units consistent
```

**Physics Validation**:
- ✅ Two-path split (geometric + elastic) is the right model family.
- ❌ Front/rear lateral sign application is incorrect in the current implementation.
- ❌ Anti-roll-bar stiffness units are inconsistent between config and solver usage.

**Numerical Test**:
```text
Given: left turn, a_y = -8.0 m/s²
Then: dFz_lat_front is negative
Code uses: FL = Static - sign(a_y) × (dFz_lat_front / 2)
sign(a_y) = -1, dFz_lat_front < 0 -> subtracts a positive number
Observed behavior: left wheel loses load in both left and right turns
Expected: left wheel should gain load in a left turn
Status: ❌ Fail
```

---

### Issues & Concerns

#### 🔴 Critical Issues (P0 - Ship Blocker)
1. **Double-signing lateral load transfer** (`VehicleSolver.cpp:1849`, `VehicleSolver.cpp:1852-1867`)
   - **Problem**: `dFz_lat_front` and `dFz_lat_rear` already carry the sign of `a_y`, but `signLat` is applied again during per-corner composition.
   - **Impact**: Right-side wheels gain load and left-side wheels lose load regardless of turn direction.
   - **Root Cause**: Signed transfer terms are treated as unsigned magnitudes.
   - **Fix**:
```cpp
// Current:
AxleData.ComputedLoads_LT[0] = StaticFL - signLat * (dFz_lat_front * 0.5f);

// Corrected:
AxleData.ComputedLoads_LT[0] = StaticFL - (dFz_lat_front * 0.5f);
AxleData.ComputedLoads_LT[1] = StaticFR + (dFz_lat_front * 0.5f);
AxleData.ComputedLoads_LT[2] = StaticRL - (dFz_lat_rear  * 0.5f);
AxleData.ComputedLoads_LT[3] = StaticRR + (dFz_lat_rear  * 0.5f);
```
   - **Verification**: Run left-turn and right-turn skidpad tests and confirm the outside wheels swap correctly.

#### ⚠️ High Priority Issues (P1 - Accuracy)
1. **Anti-roll-bar stiffness unit mismatch** (`AntiRollbarSpecifications.h:20`, `VehicleSolver.cpp:1702`, `VehicleSolver.cpp:1726`, `VehicleSolver.cpp:2861`)
   - **Problem**: Config declares `Bar.Stiffness` as `[N/m]`, but load transfer adds it directly as `[N·m/rad]`.
   - **Impact**: Roll stiffness split is numerically wrong and hard to tune.
   - **Root Cause**: The force law and the load-transfer law use different meanings for the same variable.
   - **Fix**: Either store ARB stiffness in roll-stiffness units everywhere, or convert `k_arb [N/m]` to `K_phi ≈ k_arb × t² / 2` before mixing it with spring roll stiffness.

2. **Roll center tuning data is ignored** (`ChassisConfiguration.h:83`, `ChassisConfiguration.h:103`, `VehicleSolver.cpp:1748-1749`)
   - **Problem**: `RollCenterHeightFront_m` and `RollCenterHeightRear_m` exist in config, but the solver hardcodes `0.05 m` and `0.10 m`.
   - **Impact**: Setup tuning does not actually control the physical model.
   - **Fix**: Read roll center heights from `ChassisConfig_PT`, with hardcoded values only as fallback.

---

### Magic Numbers Audit
| Line | Value | Current Usage | Recommended Action |
|------|-------|---------------|-------------------|
| 1748 | `0.05f` | Front roll center height | Read from `ChassisConfig_PT.RollCenterHeightFront_m` |
| 1749 | `0.10f` | Rear roll center height | Read from `ChassisConfig_PT.RollCenterHeightRear_m` |

---

### Code Quality Assessment

**Readability**: 8/10
- ✅ The derivation is heavily documented.
- ⚠️ The code comments are currently more correct than the actual sign implementation.

**Maintainability**: 6/10
- ✅ Good structure.
- ❌ Configured geometry data is not consistently used.

**Performance**: 8/10
- ✅ The math itself is cheap.
- ⚠️ Accuracy bugs here are much more serious than runtime cost.

---

## PASS 3: AERODYNAMICS MODEL (VehicleSolver.cpp 2286-2560)

### Function Rating
**Overall Score**: 58%  
**Star Rating**: ⭐⭐☆☆☆  
**Reviewed By**: Codex on 2026-03-30  
**Status**: ⚠️ Concerns

---

### Code Snippet
```cpp
const FVector Vel_World = VelocityDir_World * V;
const float Vx = Vel_World.X;
const float Vy = Vel_World.Y;
const float Vz = Vel_World.Z;

const float DragX_N = 0.5f * Rho * FMath::Abs(Vx) * Vx * Body.FrontalArea_m2 * Body.CoeffDrag;
const float DragY_N = 0.5f * Rho * FMath::Abs(Vy) * Vy * Body.SideArea_m2 * Body.CoeffSideForce;
```

---

### Mathematical Analysis

#### Equation 3.1: Aerodynamic Force
**Analytical Form**:
```text
F = 0.5 × ρ × V² × A × C
```

**Literature Reference**:
- **Source**: Katz, *Race Car Aerodynamics*
- **Source**: Hucho, *Aerodynamics of Road Vehicles*

**Unit Analysis**:
| Variable | Units | Dimensional Formula |
|----------|-------|---------------------|
| `ρ` | kg/m³ | M L^-3 |
| `V²` | m²/s² | L² T^-2 |
| `A` | m² | L² |
| `C` | - | 1 |
| **Result** | N | M L T^-2 |

**Verification**:
```text
kg/m³ × m²/s² × m² = kg·m/s² = N
✅ Units consistent
```

**Physics Validation**:
- ✅ Force magnitudes are built from SI-consistent dynamic pressure.
- ❌ Body drag decomposition is resolved in world axes, not body axes.
- ❌ Side force and yaw/roll moments therefore depend on world heading.

**Numerical Test**:
```text
Given:
  Speed = 50 m/s
  Vehicle rotated so its nose points along world +Y
Expected:
  Body drag should remain primarily longitudinal in vehicle body axes

Current code:
  Vx = 0, Vy = 50, Vz = 0
  Longitudinal drag goes to ~0
  Lateral side force becomes dominant

Status: ❌ Fail
```

---

### Issues & Concerns

#### 🔴 Critical Issues (P0 - Ship Blocker)
1. **Body aero is computed in world axes instead of body axes** (`VehicleSolver.cpp:2479-2486`)
   - **Problem**: The solver assumes world `X/Y/Z` correspond to vehicle longitudinal/lateral/vertical axes.
   - **Impact**: A car rotated in the world receives the wrong drag, side force, and yaw/roll moments.
   - **Fix**: Pass chassis basis vectors into `ComputeAerodynamicForces()` and project velocity into body space before applying longitudinal/lateral drag coefficients.

#### ⚠️ High Priority Issues (P1 - Accuracy)
1. **Front/rear aero split is incomplete** (`VehicleSolver.cpp:2505-2506`)
   - **Problem**: `FrontDownforce_N` / `RearDownforce_N` do not include every downforce contributor present in `TotalDownforce_N`.
   - **Impact**: Wheel-load distribution can diverge from total aero model.
   - **Fix**: Either explicitly assign every aero contributor to front/rear or compute axle split from force application points.

#### 💡 Medium Priority Issues (P2 - Polish)
1. **Hardcoded GT-R GT3 aero package** (`VehicleSolver.cpp:1913-2030`)
   - **Problem**: The framework seeds a very specific car's aero coefficients into a general solver.
   - **Impact**: Good for one test car, risky as a default framework baseline.
   - **Fix**: Move these to data assets/presets and make the solver physics car-agnostic by default.

---

### Magic Numbers Audit
| Line | Value | Current Usage | Recommended Action |
|------|-------|---------------|-------------------|
| 2488 | `1.2f` | Vertical drag multiplier | Make data-driven or document source |
| 2505 | `0.45f` | Underbody split to front axle | Derive from center-of-pressure |
| 2506 | `0.55f` | Underbody split to rear axle | Derive from center-of-pressure |

---

### Code Quality Assessment

**Readability**: 8/10
- ✅ The subsystem is well documented.
- ⚠️ Some comments imply body-frame behavior that the code does not yet implement.

**Maintainability**: 7/10
- ✅ Force aggregation is centralized.
- ❌ Force direction, axle split, and preset data are still entangled.

**Performance**: 7/10
- ✅ Aero is computed once per physics step and reused.
- ⚠️ Correctness bugs matter more than cost here.

---

## PASS 4: WHEEL DYNAMICS / SLOPE HOLD / STATIC FRICTION (VehicleSolver.cpp 4982-5249)

### Function Rating
**Overall Score**: 60%  
**Star Rating**: ⭐⭐☆☆☆  
**Reviewed By**: Codex on 2026-03-30  
**Status**: ⚠️ Concerns

---

### Code Snippet
```cpp
const FVector F_gravity_world = FVector(0.0f, 0.0f, -Fz);
const FVector F_slope_N = FVector::VectorPlaneProject(F_gravity_world, GroundNormal);
const float F_slope_long = FVector::DotProduct(F_slope_N, WheelForward);
const float F_slope_lat = FVector::DotProduct(F_slope_N, WheelRight);

const float EffectiveMass = Fz / GRAVITY_MS2;
```

---

### Mathematical Analysis

#### Equation 4.1: Grade Force Along Slope
**Analytical Form**:
```text
F_parallel = m × g × sin(θ)
```

**Literature Reference**:
- **Source**: Gillespie, *Fundamentals of Vehicle Dynamics*
- **Source**: Wong, *Theory of Ground Vehicles*

**Unit Analysis**:
| Variable | Units | Dimensional Formula |
|----------|-------|---------------------|
| `m` | kg | M |
| `g` | m/s² | L T^-2 |
| `sin(θ)` | - | 1 |
| **Result** | N | M L T^-2 |

**Verification**:
```text
kg × m/s² = N
✅ Units consistent
```

**Physics Validation**:
- ✅ The Karnopp-style static/kinetic split is a reasonable model family.
- ❌ `Fz` is normal load, not total wheel weight, so it should not be used as gravity magnitude.
- ❌ `EffectiveMass = Fz / g` makes breakaway behavior depend on aero and transient load transfer instead of actual supported mass.

**Numerical Test**:
```text
Given:
  Per-wheel supported mass = 350 kg
  Slope angle = 30°

True grade force:
  F_true = 350 × 9.8 × sin(30°) = 1715 N

Code path:
  Fz = 350 × 9.8 × cos(30°) = 2970 N
  Projected parallel force magnitude ≈ Fz × sin(30°) = 1485 N

Error:
  -13.4%

Status: ❌ Fail
```

---

### Issues & Concerns

#### ⚠️ High Priority Issues (P1 - Accuracy)
1. **Slope force is derived from normal load instead of true supported weight** (`VehicleSolver.cpp:4982-4985`)
   - **Problem**: `F_gravity_world` is built from `-Fz`, but `Fz` is contact normal load.
   - **Impact**: Grade force is angle-dependent in the wrong way and becomes coupled to load transfer and aero downforce.
   - **Fix**: Track supported mass per wheel and build gravity from `m_wheel × g`, not from `Fz`.

2. **Static-hold inertia term uses pseudo-mass from normal load** (`VehicleSolver.cpp:4994`)
   - **Problem**: `EffectiveMass = Fz / g`.
   - **Impact**: Brake-hold and breakaway thresholds change with downforce and transient load in a non-physical way.
   - **Fix**: Use sprung/unsprung mass allocation rather than back-solving mass from instantaneous normal load.

#### 💡 Medium Priority Issues (P2 - Polish)
1. **Static friction state machine is highly capable but difficult to validate** (`VehicleSolver.cpp:5002-5249`)
   - **Problem**: The logic is carefully documented but complex enough that regression risk is high without targeted slope and wedge-contact tests.
   - **Fix**: Add a validation matrix for 0°, 5°, 10°, 15°, 20°, 30° grades with straight, side-slope, and yaw-offset cases.

---

### Code Quality Assessment

**Readability**: 7/10
- ✅ Excellent explanation of intent.
- ⚠️ The documentation currently overstates the physical correctness of the grade-force derivation.

**Maintainability**: 6/10
- ✅ Logical branching is explicit.
- ❌ Complexity is high enough that more automated tests are needed.

**Performance**: 7/10
- ✅ Mostly scalar math.
- ⚠️ The issue here is correctness, not throughput.

---

## PASS 5: CONTACT / SLIP / HOT-PATH PERFORMANCE (VehicleSolver.cpp 438-715, 874-905, 3231-3450, 3851-5900; Telemetry/PerformanceLog.csv)

### Function Rating
**Overall Score**: 72%  
**Star Rating**: ⭐⭐⭐☆☆  
**Reviewed By**: Codex on 2026-03-30  
**Status**: ⚠️ Concerns

---

### Code Snippet
```cpp
const int32 AnglesCount = (static_cast<int32>(MaxAngle / AngleStepLocal) * 2) + 1;
Ax.TracePattern.NumRays = DepthCountLocal * AnglesCount;

for (int32 i = 0; i < WheelCount; ++i) { SolveContactSlip(i, DeltaTime, Rec, AxleData_PT); }

// Later, inside SolvePowertrain wheel dynamics:
SolveContactSlip(i, DeltaTime, Rec, AxleData);
```

---

### Mathematical Analysis

#### Equation 5.1: Raycast Count Per Vehicle
**Analytical Form**:
```text
R_vehicle = N_wheels × DepthCount × (2 × floor(MaxAngle / AngleStep) + 1)
```

**Unit Analysis**:
| Variable | Units | Dimensional Formula |
|----------|-------|---------------------|
| `N_wheels` | count | 1 |
| `DepthCount` | count | 1 |
| `AnglesCount` | count | 1 |
| **Result** | traces / step | 1 |

**Verification**:
```text
Default settings:
  N_wheels = 4
  DepthCount = 1
  MaxAngle = 45°
  AngleStep = 5°
  AnglesCount = 19

R_vehicle = 4 × 1 × 19 = 76 traces / physics step
✅ Arithmetic verified
```

**Numerical Test**:
```text
Given:
  50 cars
  120 physics Hz
  76 traces per car per step

Trace rate:
  50 × 120 × 76 = 456,000 line traces / second

Existing local telemetry:
  PerformanceLog.csv -> TotalFrame_us avg = 1546.47
  PerformanceLog.csv -> SuspDisp_us avg = 1482.91

Status: ⚠️ Too heavy for "many cars" without LOD / simplification
```

---

### Issues & Concerns

#### ⚠️ High Priority Issues (P1 - Accuracy / Performance)
1. **Default "fast mode" is not actually 1 ray per wheel** (`VehicleSolver.h:1231-1243`, `VehicleSolver.cpp:883-884`)
   - **Problem**: Comments imply a simplified single-ray fast path, but the default configuration still creates a 19-angle fan per wheel.
   - **Impact**: Suspension displacement dominates the logged frame cost.
   - **Fix**: Add an actual one-ray mode, or use a lower-angle contact LOD for distant/non-player cars.

2. **Slip solver is executed twice per frame** (`VehicleSolver.cpp:645`, `VehicleSolver.cpp:5217`, `VehicleSolver.cpp:5231`, `VehicleSolver.cpp:5247`)
   - **Problem**: `SolveContactSlip()` runs once in `OnPreSimulate_Internal()` and again inside wheel dynamics branches.
   - **Impact**: Wasted Pacejka and Newton iterations; the first pass can even use stale drive torque from the previous frame.
   - **Fix**: Compute slip once after drive/brake torque inputs are final, or split kinematics from final force solve explicitly.

3. **Self-aligning torque is computed during every combined-slip evaluation even when the caller only needs Fx/Fy** (`VehicleSolver.cpp:3205`, `VehicleSolver.cpp:3325`, `VehicleSolver.cpp:3346`, `VehicleSolver.cpp:3355`, `VehicleSolver.cpp:3393`)
   - **Problem**: `ComputeCombinedPacejkaForces()` always computes `Mz`, but `SolveContactSlip()` discards it.
   - **Impact**: Extra hot-path work inside Newton iterations.
   - **Fix**: Add an Fx/Fy-only variant for the iterative solver.

#### 💡 Medium Priority Issues (P2 - Polish)
1. **Wheel inertia is hardcoded in two separate places** (`VehicleSolver.cpp:4133`, `VehicleSolver.cpp:4858`)
   - **Problem**: `1.2 kg·m²` is used for reflected inertia and wheel spin integration instead of tire/wheel data.
   - **Impact**: Shift feel, clutch response, and wheel angular acceleration are harder to tune consistently.
   - **Fix**: Source wheel inertia from `TireSpecifications` or a dedicated wheel assembly spec.

2. **Performance telemetry is incomplete** (`Telemetry/PerformanceLog.csv`)
   - **Problem**: `TraceCount` is always zero in the supplied capture even though tracing cost is clearly present.
   - **Impact**: Harder to trust the profiler when optimizing.
   - **Fix**: Populate trace-count and Pacejka-call counters from the actual hot paths.

---

### Magic Numbers Audit
| Line | Value | Current Usage | Recommended Action |
|------|-------|---------------|-------------------|
| 883 | `45.0f` | Max suspension fan angle | Create per-LOD contact presets |
| 4133 | `1.2f` | Reflected wheel inertia | Move to wheel/tire assembly data |
| 4858 | `1.2f` | Wheel rotational inertia | Move to wheel/tire assembly data |

---

### Code Quality Assessment

**Readability**: 8/10
- ✅ Good separation of major phases.
- ⚠️ Some comments currently describe a cheaper path than the code actually executes.

**Maintainability**: 7/10
- ✅ The SoA layout and caches are sensible.
- ⚠️ There is now enough duplication in the slip path that future fixes will be easy to apply inconsistently.

**Performance**: 5/10
- ✅ Precomputed Pacejka caches and the physics-thread design are good choices.
- ❌ Current contact tracing and duplicate slip evaluation will limit "many-car" scalability.

---

## Overall Recommendation

Do not treat the current implementation as "physically correct and multiplayer-ready" yet. The architecture is promising, but the following should be fixed before wider tuning:

1. Correct the lateral load transfer sign error.
2. Resolve the anti-roll-bar unit mismatch.
3. Use configured roll center heights instead of hardcoded values.
4. Rework body aero into vehicle body coordinates.
5. Replace the slope/hold `Fz -> gravity` shortcut with true supported-mass gravity projection.
6. Remove duplicate slip solving and add a genuine low-cost contact LOD for non-player vehicles.
7. Redesign networking so the owning client has prediction/reconciliation instead of pure server-transform interpolation.

---

## References

1. Milliken, W.F. and Milliken, D.L., *Race Car Vehicle Dynamics*, SAE International.
2. Rill, G. and Castro, A., *Road Vehicle Dynamics*, CRC Press.
3. Pacejka, H.B., *Tire and Vehicle Dynamics*, 3rd ed.
4. Gillespie, T.D., *Fundamentals of Vehicle Dynamics*, SAE International.
5. Wong, J.Y., *Theory of Ground Vehicles*.
6. Katz, J., *Race Car Aerodynamics*.
7. Hucho, W.-H., *Aerodynamics of Road Vehicles*.
8. Local runtime evidence: `Telemetry/PerformanceLog.csv`, `Telemetry/FrictionStateMachineLog.csv`, `Telemetry/AerodynamicsLog.csv`.
