# VehicleSolver Engineering Review (Telemetry-Calibrated)

**Files**: `VehicleSolver.cpp`, `VehicleSolver.h`, `Controllers/VehicleController.cpp`, `Telemetry/TelemetryLogger.cpp`, `Components/TurbochargerSpecifications.h`, `Components/SuperchargerSpecifications.h`, `Components/AerodynamicSpecifications.h`, `Components/AudioComponent/*`
**Reviewer**: Codex (GPT-5)
**Review Date**: 2026-04-03
**Review ID**: CODEX_VEHICLESOLVER_2026-04-03_LOGFIRST
**Supersedes**: `Docs/Reviews/VehicleSolver_Review_Codex_2026-03-30.md`
**Telemetry Inputs**: `PowertrainLog.csv`, `AerodynamicsLog.csv`, `FrictionStateMachineLog.csv`, `PerformanceLog.csv`, `Multiplayer_PlayerBP_C_0.csv`
**Scope**: Full review of the main `VehicleSolver` path, forced induction, tires, aerodynamics, networking, telemetry integrity, and engine audio, using the captured logs first and then verifying conclusions directly against the code.

---

## Executive Summary

| Metric | Score | Status |
|--------|-------|--------|
| Telemetry Integrity | 38% | CRITICAL |
| Suspension / Contact Solve | 78% | OK |
| Tire Model Accuracy | 71% | WARN |
| Forced Induction Accuracy | 31% | CRITICAL |
| Aerodynamic Accuracy | 46% | WARN |
| Networking Architecture | 40% | CRITICAL |
| Engine Audio Architecture | 67% | WARN |
| Runtime Performance | 78% | OK |
| **OVERALL RATING** | **56%** | **WARN** |

**Key Findings**: The solver is fast enough for a single vehicle and has a solid stage order: steering, suspension, anti-roll, aero, load transfer, slip solve, then powertrain. The telemetry, however, is not yet trustworthy enough to support a purely log-driven accuracy review: several derived kinematic fields are never populated, manifold pressure is logged with the wrong equation, and important performance counters are never written. Once those logging gaps are separated out, the biggest physics issues are clear: the supercharger model is not implemented in the main solve, the turbo is parameterized far below real spool speeds, and the aero ride-height path is using suspension travel as if it were floor clearance.

---

## Telemetry-First Validation

### What this capture actually covers

| Log | Samples | Time Span | What it validates | What it does not validate |
|-----|---------|-----------|-------------------|---------------------------|
| `PowertrainLog.csv` | 1452 | 0.019 s -> 10.945 s | Launch, braking, shifts, clutch behavior, wheel slip, straight-line turbo spool | Sustained cornering, steering authority, combined slip under lateral load |
| `AerodynamicsLog.csv` | 1452 | 0.019 s -> 10.945 s | Straight-line drag/downforce scaling, ride-height telemetry output | Crosswind, yawed aero, drift, banking, high-speed GT aero beyond 94 km/h |
| `FrictionStateMachineLog.csv` | 1143 | same session | Static-vs-kinetic logging path in principle | Hybrid lateral-static state, because current logger cannot emit it |
| `PerformanceLog.csv` | 1452 | same session | Stage timing for major passes | Pacejka inner-loop cost, trace counts, force-apply cost, because counters are never populated |
| `Multiplayer_PlayerBP_C_0.csv` | 374 | 0.019 s -> 10.945 s | Authority-side controller sampling only | Actual client prediction, packet flow, latency response, remote vehicle smoothing |

### Observed run summary

- Vehicle speed rises from `0.0` to `93.95 km/h`.
- Engine speed rises from idle to `7162.4 rpm`.
- There are `4` non-zero gear events and `58` samples marked as shifting.
- Steering is `0.0` for the entire capture, so this is a straight-line run.
- Turbo shaft speed peaks at only `10527.6 rpm`, with boost peaking at `0.0892 bar`.
- Total aero drag reaches `1159.57 N` and total downforce reaches `2472.89 N`, but only up to `93.95 km/h`.
- Mean physics cost is `742.3 us`, with `p95 = 1245.1 us`.
- `ComputeSuspensionDisplacements` dominates runtime at `638.8 us` mean and `1079.0 us` p95.

### Telemetry integrity problems revealed by the logs

1. `Acceleration_G`, `LateralAccel_G`, `LongAccel_G`, and `LatAccel_G` are `0.0000` for the full capture, but finite-difference speed from the same CSV reaches about `-0.43 g` during braking.
2. `ManifoldPressure_kPa` is logged as `5-9 kPa` at `0.05-0.09 bar` boost, where real absolute manifold pressure should be roughly `106-110 kPa`.
3. `Pacejka_us`, `ForceApply_us`, `PacejkaCalls`, and `TraceCount` stay at zero for every performance sample even though those stages are clearly executing.
4. `FrictionStateMachineLog.csv` only contains `KINETIC` and `STATIC_FULL`, but the runtime code has a hybrid regime; the logger is not preserving the actual state machine state.

That means the previous review should not have treated these logs as a complete validation set. This rewrite uses the logs to bound what the capture proves, then uses direct code verification for the rest.

---

## Findings

### 1. Derived body-axis kinematics are never populated, so multiple systems run on zeros

- **Evidence from logs**: all longitudinal and lateral acceleration columns remain zero for the full capture, despite visible speed changes and wheel-force changes.
- **Code verification**: `OnPreSimulate_Internal()` fills `Rec.mu_mass`, `Rec.nu_linearMs`, `Rec.nu_magnitudeMs`, and `Rec.alpha_linear`, but never assigns `Rec.nu_forwardMs`, `Rec.nu_lateralMs`, `Rec.nu_verticalMs`, `Rec.alpha_longitudinal`, `Rec.alpha_lateral`, `Rec.alpha_g_longitudinal`, `Rec.alpha_g_lateral`, `Rec.psi_yaw_rate`, `Rec.theta_pitch_rate`, or `Rec.phi_roll_rate` (`VehicleSolver.cpp:541-556`, `VehicleSolver.h:57-90`).
- **Consumers affected**:
  - audio conduit publishing (`VehicleSolver.cpp:742-745`)
  - steering stability limiter (`VehicleSolver.cpp:3189-3198`)
  - shift corner-hold logic (`VehicleSolver.cpp:4533`)
  - telemetry logger (`TelemetryLogger.cpp:71-72`)
- **Impact**: telemetry is misleading, audio misses actual acceleration cues, and steering/shift heuristics do not have the state they think they have.
- **Verdict**: this is the most important cross-cutting correctness issue in the current snapshot.

### 2. Supercharger physics is not present in the main solve

- **Evidence from logs**: there is no powertrain telemetry proving any supercharger torque, parasitic drag, or bypass behavior.
- **Code verification**:
  - `FSuperchargerSpecifications` and `FSuperchargerStateVector` are defined in `SuperchargerSpecifications.h`.
  - `VehicleSolver.cpp` never evaluates `EvalBoostPressure()`, `EvalTorqueMultiplier()`, or `EvalParasiticDrag()` for the supercharger.
  - The only runtime usages are audio/profile plumbing (`VehicleSolver.cpp:189-193`, `VehicleSolver.cpp:750-754`).
- **Impact**: the solver cannot currently be reviewed as a physically implemented supercharged powertrain, because that branch is effectively absent.
- **Verdict**: any claim that the current supercharger is "accurate to real life" would be incorrect.

### 3. Manifold pressure telemetry uses the wrong equation

- **Evidence from logs**: at `0.0501 bar` boost the CSV reports `5.07 kPa`; at `0.0880 bar` boost it reports `8.91 kPa`. Absolute manifold pressure should be near atmospheric plus boost, not atmospheric multiplied by boost.
- **Code verification**:
  - `CollectTelemetrySample()` passes `TurboState.CurrentBoostPressure` into the helper (`VehicleSolver.cpp:6043`).
  - `TelemetryLogger.cpp` then computes `Sample.ManifoldPressure_kPa = 101.325f * BoostRatio` (`TelemetryLogger.cpp:57-58`).
- **Correct relation**:
  - If the logged value is gauge boost in bar, absolute MAP in kPa should be approximately `(1.01325 + BoostBar) * 100.0`.
- **Impact**: the MAP column cannot be used for calibration until this is fixed.

### 4. Turbo spool is far slower than a real automotive turbocharger

- **Evidence from logs**:
  - peak turbo shaft speed is `10527.6 rpm`
  - peak boost is only `0.0892 bar`
  - engine still reaches `7162.4 rpm` and vehicle reaches `93.95 km/h`
- **Code verification**:
  - the turbo map expects `0.2 bar` by `20000 rpm`, `0.8 bar` by `50000 rpm`, and `1.2 bar` by `90000 rpm` (`TurbochargerSpecifications.h:79-85`)
  - runtime turbo inertia is `0.02 kg m^2` (`TurbochargerSpecifications.h:17`, `TurbochargerSpecifications.h:66`)
  - the shaft is integrated directly from net shaft power in `SolvePowertrain()` (`VehicleSolver.cpp:4159-4164`)
- **Real-world accuracy assessment**: the current spool state is not in the right operating decade. Real passenger/performance turbos live in the tens to hundreds of thousands of rpm, not ten thousand.
- **Impact**: the turbo torque multiplier is effectively a near-zero correction during this pull.

### 5. Aero ride height is not actual underfloor clearance, so ground effect is over-active

- **Evidence from logs**:
  - `MinRideHeight_cm` stays between `43.41` and `50.00`
  - `RideHeightModifier` is stuck at `0.3000`
  - underbody downforce still reaches `882.24 N`
  - total downforce reaches `2302-2473 N` by only `93.95 km/h`
- **Derived aero coefficients at max-speed sample (`q = 417.12 Pa`)**:
  - `TotalDrag_N / q = 2.610 m^2`
  - `TotalDownforce_N / q = 5.519 m^2`
  - `UnderbodyDownforce_N / q = 2.115 m^2`
- **Code verification**:
  - ride height is derived from `RestLength - SpringDisplacement` per wheel (`VehicleSolver.cpp:641-651`)
  - that is suspension travel, not floor-to-ground clearance
  - underbody force still keeps a minimum penalty floor rather than decaying toward zero (`VehicleSolver.cpp:2520-2535`, `VehicleSolver.cpp:6206-6219`)
- **Real-world accuracy assessment**: a GT-style floor producing strong underbody load while the reported floor clearance is roughly half a meter is not physically credible.

### 6. Body aero is computed in world axes instead of vehicle body axes

- **Evidence from logs**: not exposed by this straight-line run, because the car appears to spend most of the capture aligned with world X.
- **Code verification**:
  - `ComputeAerodynamicForces()` uses `Vel_World.X`, `Vel_World.Y`, and `Vel_World.Z` directly as if they were body-frame longitudinal/lateral/vertical velocity components (`VehicleSolver.cpp:2604-2618`)
- **Impact**: drift, crosswind, yawed braking, and banked-road aero loads will be wrong even if straight-line drag looks reasonable.
- **Verdict**: this is a direct code issue, even though this capture does not stress it.

### 7. Networking is still server-only transform replication, not a responsive replicated vehicle model

- **Evidence from logs**:
  - only one authority-side controller is present
  - `HasNetConnection = 0` for every multiplayer sample
  - packet and byte counters stay at zero
- **Code verification**:
  - `bAlwaysRelevant = true`, `SetNetUpdateFrequency(60.0f)`, `SetMinNetUpdateFrequency(30.0f)` (`VehicleSolver.cpp:47-50`)
  - transform and velocity are replicated as raw fields (`VehicleSolver.cpp:82-85`)
  - clients disable physics in `BeginPlay()` (`VehicleSolver.cpp:145-167`)
  - clients interpolate and early-return in `Tick()` (`VehicleSolver.cpp:233-251`)
- **Impact**:
  - no autonomous-proxy prediction
  - no reconciliation
  - no compact replicated vehicle state
  - poor scalability once multiple cars are active
- **Verdict**: the current runtime matches the old P2P guide's "host simulates, clients interpolate" shape, not a low-latency drivable multiplayer vehicle.

### 8. Engine audio is architecturally promising, but it is not yet multiplayer-ready or fully mix-correct

- **Strengths**:
  - clean double-buffered profile/state handoff (`EngineAudioSolver.cpp:37-55`)
  - modular synthesis split into engine, tire, brake, suspension, and forced induction (`VehicleAudioMixer.cpp:3-13`)
  - sample clamping and callback health stats are good defensive engineering (`EngineAudioSolver.cpp:69-155`)
- **Problems**:
  - clients return from `BeginPlay()` before audio is created, so only the authority path gets `EngineAudioSolver` initialization (`VehicleSolver.cpp:145-167`, `VehicleSolver.cpp:178-200`)
  - the component is configured as a non-spatial UI sound (`EngineAudioSolver.cpp:23-28`)
  - `SuperchargerGain` exists in the profile but is never used; all forced-induction audio is mixed through `TurboGain` (`VehicleAudioMixer.cpp:39`, `VehicleAudioTypes.h:104-105`)
  - brake, suspension, and forced-induction filters are retuned every sample, which is unnecessary audio-thread cost (`ForcedInductionAudioModel.cpp:83`, `ForcedInductionAudioModel.cpp:113`, `BrakeAudioModel.cpp:64-65`, `SuspensionAudioModel.cpp:65-66`)
  - the audio feed itself inherits the zeroed acceleration/lateral-state problem from finding 1
- **Impact**: cockpit synthesis is workable on the host, but remote vehicles and world-mix realism are not solved yet.

### 9. Performance and friction telemetry are missing the counters needed for a trustworthy hotspot review

- **Evidence from logs**:
  - `Pacejka_us = 0.0` for all samples
  - `ForceApply_us = 0.0` for all samples
  - `PacejkaCalls = 0` for all samples
  - `TraceCount = 0` for all samples
  - `FrictionStateMachineLog.csv` never emits `STATIC_LAT`
- **Code verification**:
  - only a subset of `CurrentPerfSample` fields are wrapped in timers (`VehicleSolver.cpp:564-715`, `VehicleSolver.cpp:805`)
  - no assignments exist for call counts
  - the friction logger writes `Sample.StateEnum = AxleData.WheelLocked[i] ? 2 : 0`, which collapses the state machine to `STATIC_FULL` or `KINETIC` only (`VehicleSolver.cpp:5820`)
- **Impact**: the current logs under-report both solver work and state-machine coverage.

---

## Function-by-Function Verdict

### `OnPreSimulate_Internal()`

- **Verdict**: good stage ordering and low overhead, but it is the source of the missing derived-state problem.
- **What the logs show**: the pipeline is stable enough to produce 1452 samples in 10.9 s and keep total mean physics time under 1 ms for one vehicle.
- **What to improve**: populate all body-axis velocities and accelerations before steering, telemetry, and audio consume them.

### `ComputeSuspensionDisplacements()`

- **Verdict**: technically solid and the main physics cost center.
- **What the logs show**: `638.8 us` mean, `1079.0 us` p95, `2912.7 us` max. This pass dominates total frame time for the captured single car.
- **What is good**: cached metrics, pre-rotated wheel directions, and multi-ray averaging are all appropriate for a higher-fidelity chassis.
- **What to improve**: instrument trace count properly, add contact caching or trace LOD, and consider reducing ray density at low speed or when suspension velocity is near zero.

### `ComputeSuspensionForces()`

- **Verdict**: numerically cheap and stable.
- **What the logs show**: `5.0 us` mean and `8.3 us` p95, so this stage is not a CPU problem.
- **Accuracy note**: the log set does not contain a damper sweep or curb-strike scenario, so real-world validation here is limited.

### `ComputeAntiRollbarForces()`

- **Verdict**: computationally fine, but still needs a unit-consistency cleanup in the broader load-transfer path.
- **What the logs show**: `1.41 us` mean, `2.5 us` p95.
- **Code note**: `AxleSpecifications.h` comments bar stiffness as `[N/m]`, while `ComputeLoadTransferRealtime()` sums `Bar.Stiffness` as if it were already `[N m/rad]`. That mismatch should be resolved before using bar stiffness to calibrate real roll gradients.

### `ProcessSteering()`

- **Verdict**: the steering architecture is richer than the capture can validate.
- **What the logs show**: steering input is zero for the entire run, so the stability limiter and Ackermann branches are not exercised.
- **Code note**: the stability limiter depends on `Rec.nu_forwardMs` and `Rec.nu_lateralMs`, which are currently never populated, so this subsystem is under-fed even before a steering test is run.

### `ComputeLoadTransferRealtime()`

- **Verdict**: the structure is good, but this specific log set only validates straight-line longitudinal transfer and aero load addition.
- **What the logs show**:
  - front/rear load variation is active
  - lateral acceleration channels are unusable because they stay at zero
  - aero loads are definitely entering wheel loads
- **Code note**:
  - hard-coded roll-center heights (`0.05 m` front, `0.10 m` rear) should eventually be configuration data
  - the current cornering sign behavior should be rechecked once a real steering/cornering log is captured

### `ComputeAerodynamicForces()`

- **Verdict**: body drag baseline is sensible; ground effect and coordinate handling are not yet ready for a realism claim.
- **What the logs show**:
  - body drag at max-speed sample implies `CdA ~= 0.644 m^2`, which is reasonable
  - total aero is much larger because the floor and wing package are generating very large low-speed load
- **What to improve**: use actual floor clearance, body-frame velocity decomposition, and component-level calibration against target downforce-vs-speed curves.

### `SolveContactSlip()`, `ComputePacejkaLongitudinalForce()`, `ComputePacejkaLateralForce()`, `ComputeSelfAligningTorque()`, `ApplyCombinedSlip()`

- **Verdict**: the longitudinal behavior in this capture is plausible for the default GT3-style tire parameters, but the full tire model is richer on paper than in active use.
- **What the logs show**:
  - peak longitudinal force ratios are about `mu_x = 1.90-1.92` at slip ratios near `0.18-0.21`
  - that is credible for a slick or very aggressive semi-slick, and it matches the default tire data better than the old review suggested
- **Code limits still present**:
  - camber is hard-coded to `0.0f` in the slip and force path
  - tire thermal, wear, pressure, and aquaplaning properties exist in `TireSpecifications.h` but are not active in the main solve path reviewed here
- **Conclusion**: straight-line tire force generation is the best-calibrated part of the current solver; lateral/combined-slip realism still needs a cornering log to verify.

### `SolvePowertrain()`

- **Verdict**: fast, organized, and mostly coherent for engine-clutch-gear-diff-wheel flow, but incomplete for forced induction realism.
- **What the logs show**:
  - engine rpm, clutch slip, and shift timing all move in believable directions
  - the turbo barely contributes
  - no supercharger behavior is present
- **Performance**: `42.7 us` mean and `105.9 us` p95, so there is no immediate CPU pressure here.
- **What to improve**:
  - implement supercharger physics
  - recalibrate turbo inertia and map scaling
  - stop feeding shift corner-hold from zeroed lateral speed

### Networking path: `GetLifetimeReplicatedProps()`, `ServerUpdateInput()`, `BeginPlay()`, `Tick()`, controller RPC routing

- **Verdict**: functional authority flow, but not a finished multiplayer vehicle architecture.
- **What the logs show**: only an authority-side run. No client evidence exists in the capture.
- **What to improve**:
  - local prediction and reconciliation for owning clients
  - compact replicated state instead of raw replicated transforms plus component replication
  - relevancy-based update rate rather than `bAlwaysRelevant` at `60 Hz`

### Audio path: `EngineAudioSolver`, `EngineAudioModel`, `ForcedInductionAudioModel`, `TireAudioModel`, `BrakeAudioModel`, `SuspensionAudioModel`, `VehicleAudioMixer`

- **Verdict**: solid modular design, but still missing a clean separation between cockpit synth, world spatial sound, and replicated ownership.
- **What the logs show**: current audio feed is missing real acceleration cues because the source telemetry is missing them.
- **What to improve**:
  - create the solver on relevant clients too
  - decide whether this synth is cockpit-only or world-audible
  - retune filters at control rate
  - use separate gain routing for turbo vs supercharger

---

## Performance Review

### Measured single-vehicle cost

| Stage | Mean | P95 | Max | Notes |
|-------|------|-----|-----|------|
| Suspension displacement | `638.8 us` | `1079.0 us` | `2912.7 us` | Main cost center |
| Suspension forces | `5.0 us` | `8.3 us` | `298.3 us` | Cheap |
| Load transfer | `2.53 us` | `3.6 us` | `282.7 us` | Cheap |
| Anti-rollbar | `1.41 us` | `2.5 us` | `79.1 us` | Cheap |
| Steering | `2.11 us` | `4.3 us` | `91.0 us` | Cheap |
| Contact slip | `40.2 us` | `97.2 us` | `720.5 us` | Moderate but acceptable |
| Powertrain | `42.7 us` | `105.9 us` | `498.3 us` | Acceptable |
| Total frame | `742.3 us` | `1245.1 us` | `3009.6 us` | Good for one vehicle |

### Performance verdict

- The solver is not CPU-bound in `SolvePowertrain()` or Pacejka for this one-car capture.
- The real hot path is suspension tracing, which is expected.
- The performance logs are incomplete enough that a full hotspot ranking is still impossible.
- The audio side likely wastes more CPU than needed by retuning several filters every generated sample.

---

## Real-World Accuracy Verdict

### Closest to real-life today

- Straight-line tire longitudinal grip and slip-force shape are reasonably believable for the default GT3-style tire parameters.
- Clutch slip, shift timing, and differential topology are structured like a serious sim rather than an arcade abstraction.
- Body drag baseline is in the right neighborhood.

### Furthest from real-life today

- Turbo shaft speed and boost buildup are much too low.
- Supercharger physics is missing.
- Ground effect is too active for the reported ride height.
- Networking is not yet representative of how a responsive multiplayer vehicle should feel.
- Audio replication and world presentation are not yet aligned with a networked vehicle game.

---

## Recommended Fix Order

1. Fix the telemetry and record-population layer first.
   - Populate all derived kinematic fields in `FInstantaneousVehicleRecord`.
   - Correct manifold pressure logging.
   - Instrument real `PacejkaCalls`, `TraceCount`, `Pacejka_us`, and force-apply timing.
   - Log the actual friction regime enum, not `WheelLocked`.

2. Fix forced induction next.
   - Re-parameterize turbo inertia and validate spool against target shaft-speed and boost traces.
   - Implement the missing supercharger branch in `SolvePowertrain()`.

3. Fix aero after telemetry is trustworthy.
   - Measure real floor clearance from aero sockets, not suspension travel.
   - Resolve body aero in body axes.
   - Refit total downforce-vs-speed to a target real-world envelope.

4. Fix networking and audio together.
   - Add autonomous-proxy prediction and reconciliation.
   - Stop treating vehicle audio as authority-only UI sound.
   - Split turbo and supercharger gains and move filter retuning off the per-sample path.

---

## Final Verdict

The previous review was too willing to generalize from incomplete telemetry. This capture is good enough to say that the solver is performant, the longitudinal tire model is materially better than the earlier review gave it credit for, and the main remaining realism gap is not "everything" but a specific cluster: missing derived kinematics, incomplete forced induction, over-active ground effect, and unfinished networking/audio ownership. Once the logging layer is fixed, the next review should be done with three targeted captures: a hard braking run, a steady-state cornering run, and a two-client multiplayer session with real RTT.
