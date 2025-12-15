# GRIT VehicleFramework - Development Context

**Last Updated:** 2025-12-15
**Purpose:** Track what's done, what's in progress, and what's remaining

---

## Current Status Overview

| System | Status | Completion |
|--------|--------|------------|
| VehicleSolver (Physics) | Done | 90% |
| Suspension | Done | 100% |
| Tire Model (Pacejka MF6.1) | Done | 100% |
| Powertrain | Done | 100% |
| Aerodynamics | Done | 100% |
| Input System | Done | 100% |
| Telemetry | Done | 100% |
| **Replication (P2P)** | **Done** | **100%** |
| VehicleConfigurator | Not Started | 0% |
| UI/HUD/HMI | Foundation Only | 10% |

---

## What's Complete

### VehicleSolver (~90%)
- [x] Chaos Physics integration (`FVehicleSolverCallback`)
- [x] Thread-safe GT/PT architecture (`TThreadLock`, `FSyncEventFlags`)
- [x] `FInstantaneousVehicleRecord` - Complete vehicle state snapshot
- [x] `FVehicleSolverAxleData_PT` - SoA physics thread data

### Suspension System (100%)
- [x] Progressive spring model (cubic polynomial)
- [x] Multi-ray suspension tracing (`FTrajectoryAtlas`)
- [x] Per-wheel suspension axis support
- [x] Sprung mass calculation via Lagrange multipliers
- [x] Anti-roll bar system
- [x] Roll center calculation from sockets

### Tire Model (100%)
- [x] Pacejka MF6.1 longitudinal force
- [x] Pacejka MF6.1 lateral force
- [x] Combined slip (MF6.1 weighting functions)
- [x] Self-aligning torque with combined slip correction
- [x] Implicit Newton slip solver (zero-lag transients)
- [x] Precomputed coefficient cache (`FPacejkaPrecomputedCache`)
- [x] Tire thermal model (surface/core temperatures)
- [x] Tire wear model
- [x] Rolling resistance
- [x] `.tyrx` tire data files

### Stiction State Machine (100%)
- [x] Kinetic → Static_Lateral → Static_Full states
- [x] Slope compensation
- [x] Friction circle budget management

### Powertrain (100%)
- [x] Engine (torque curves, thermal, viscosity friction)
- [x] Turbocharger (twin-scroll, wastegate, BOV, surge)
- [x] Supercharger (centrifugal/roots modes)
- [x] Electric motor (EV/hybrid)
- [x] Battery pack simulation
- [x] Clutch (sigmoid engagement, lock-up detection)
- [x] Transmission (sequential/H-pattern, auto shift logic)
- [x] Differentials (Open, LSD, Locking, Torque Vectoring)
- [x] Center/Front/Rear differential topology

### Aerodynamics (100%)
- [x] Body drag/lift/side force
- [x] Rear wing (adaptive, brake deploy)
- [x] Front splitter with air dam
- [x] Canards (damage modeling)
- [x] Underbody/diffuser (ground effect)
- [x] Side skirts (underbody sealing)
- [x] Vortex generators
- [x] Ride height sensitivity

### Load Transfer (100%)
- [x] Longitudinal load transfer
- [x] Lateral load transfer
- [x] Aerodynamic downforce distribution
- [x] Static load calculation

### Input System (100%)
- [x] `FInputTensor` comprehensive structure
- [x] `AVehicleController` Enhanced Input System
- [x] Signal smoothing (throttle, brake, steering rates)
- [x] Drive mode switching (double-tap reverse)
- [x] Device type detection

### Telemetry (100%)
- [x] `FTelemetrySample` - Vehicle state logging
- [x] `FAerodynamicsSample` - Aero forces logging
- [x] `FPerformanceSample` - Function timing (μs)
- [x] CSV export on EndPlay
- [x] `FScopedTimer` RAII profiler

### Actor Structure (100%)
- [x] `AVehicleSolver` - Physics solver pawn
- [x] `AVehicleConstruct` - Player vehicle with camera
- [x] `ATireConstruct` - Tire specification/mesh provider
- [x] `AVehicleController` - Player controller

---

## What Needs Polish (VehicleSolver)

### Minor Issues
- [ ] Edge case: Very low speed clutch engagement
- [ ] Edge case: Extreme camber angles
- [ ] Fine-tuning: Stiction transition thresholds
- [ ] Fine-tuning: Turbo lag response

### Potential Improvements
- [ ] Tire temperature visualization
- [ ] Brake fade visualization
- [ ] Debug draw improvements

---

## TODO - Major Systems

### 1. Replication (Priority: HIGH) - COMPLETE

**Goal:** P2P multiplayer with 50+ vehicles

**Status:** Implemented on 2025-12-15

**Completed Tasks:**
- [x] Add `P2P=1` to `GRIT.Build.cs`
- [x] Implement `FReplicatedVehicleState_Compact` (48 bytes) with pack/unpack
- [x] Implement `FReplicatedVehicleInput` (~16 bytes) for client input
- [x] Add `UPROPERTY(ReplicatedUsing=OnRep_VehicleState)`
- [x] Implement `OnRep_VehicleState()` for clients
- [x] Implement `Server_SendInput()` RPC (unreliable)
- [x] Implement `Multicast_VehicleEvent()` for events (reliable)
- [x] Add `HasPhysicsAuthority()` / `IsSimulatedProxy()` helpers
- [x] Modify `BeginPlay()` for host/client split
- [x] Modify `Tick()` for 60Hz replication timing
- [x] Modify `OnPreSimulate_Internal()` to skip on clients
- [x] Client-side interpolation (Lerp between states)
- [x] `PackReplicatedState()` / `ApplyReplicatedState()` / `InterpolateState()`

**Optional (Not Implemented):**
- [ ] Distance-based LOD for update rates
- [ ] Delta compression
- [ ] Batched updates

**Bandwidth:** 144 KB/s for 50 vehicles @ 60 Hz

---

### 2. VehicleConfigurator (Priority: MEDIUM)

**Goal:** Runtime vehicle configuration and preset management

**Tasks:**
- [ ] Design `FVehicleConfiguration` data structure
- [ ] Create `UVehicleConfigurator` component
- [ ] Preset system (GT3, Rally, Drift, Street, etc.)
- [ ] Component swapping (engine, turbo, tires, etc.)
- [ ] Save/load to JSON or DataAsset
- [ ] Editor utility for creating presets
- [ ] Blueprint-exposed configuration API
- [ ] Validation system (compatible parts)

**Presets to Create:**
- [ ] GT3 Race Car
- [ ] Rally Car (AWD, long travel)
- [ ] Drift Car (high power, RWD)
- [ ] Street Car (balanced)
- [ ] Hypercar (extreme aero)

---

### 3. UI/HUD/HMI (Priority: MEDIUM)

**Goal:** In-game vehicle interface

**Foundation:** `UserInterfaces/GraphicalUserInterface.h` (theme system exists)

**Dashboard HUD:**
- [ ] Speedometer (digital/analog)
- [ ] Tachometer with redline
- [ ] Gear indicator
- [ ] Boost gauge
- [ ] Fuel gauge
- [ ] Temperature gauges (coolant, oil)
- [ ] Lap timer

**Telemetry Overlay:**
- [ ] Real-time tire temps (4-corner)
- [ ] Slip angle visualization
- [ ] G-force meter
- [ ] Throttle/brake/steering bars
- [ ] Suspension travel visualization

**Setup Screen:**
- [ ] Suspension settings (spring rate, damping, ARB)
- [ ] Differential settings (preload, ramp)
- [ ] Aerodynamics (wing angle, ride height)
- [ ] Transmission (gear ratios, final drive)
- [ ] Brake bias

**Menus:**
- [ ] Pause menu
- [ ] Settings menu
- [ ] Vehicle selection

---

## Architecture Notes

### Thread Model
```
Game Thread (GT)                Physics Thread (PT)
     │                               │
     ▼                               ▼
FInputTensor ──TThreadLock──► InputConduitPtr
     │                               │
     ▼                               ▼
Axles_GT ────────────────────► AxleData_PT
     │                               │
     ▼                               ▼
Drivetrain_GT ───────────────► DrivetrainSpecs_PT
     │                               │
     ▼                               ▼
AeroPackage_GT ──────────────► AeroPackage_PT
```

### Force Application Order (per frame)
1. Input Processing (`ProcessSteering`)
2. Suspension Displacement (ray traces)
3. Suspension Forces (spring/damper)
4. Load Transfer (`ComputeLoadTransferRealtime`)
5. Anti-Roll Bars (`ComputeAntiRollbarForces`)
6. Tire Forces (`SolveContactSlip` + Pacejka)
7. Powertrain (`SolvePowertrain`)
8. Aerodynamics (`ComputeAerodynamicForces`)
9. Force Application (`RigidBody->AddForce/AddTorque`)

---

## File Quick Reference

| Component | File |
|-----------|------|
| Main solver | `VehicleSolver.cpp/h` |
| Vehicle actor | `VehicleConstruct.cpp/h` |
| Tire model | `Components/TireSpecifications.h` |
| Suspension | `Components/SuspensionSpecifications.h` |
| Drivetrain | `Components/DrivetrainSpecifications.h` |
| Aerodynamics | `Components/AerodynamicSpecifications.h` |
| Input | `Input/InputTensor.h` |
| Controller | `Controllers/VehicleController.cpp/h` |
| Telemetry | `Telemetry/TelemetryLogger.cpp/h` |
| UI Base | `UserInterfaces/GraphicalUserInterface.cpp/h` |
| P2P Guide | `Docs/Networking/P2P_Physics_Replication_Guide.md` |
| Codebase Summary | `Docs/CODEBASE_SUMMARY.md` |

---

## Session Log

### 2025-12-15 (Session 3)
- **Fixed UHT Compilation Errors** - UPROPERTY/UFUNCTION cannot be inside `#if` blocks
  - Changed `FReplicatedVehicleState_Compact` to `FReplicatedVehicleState` USTRUCT (full precision FVector/FRotator)
  - Changed `FReplicatedVehicleInput` to USTRUCT with GENERATED_BODY()
  - Moved all UPROPERTY/UFUNCTION declarations outside `#if P2P` blocks
  - Put `#if P2P` guards inside function bodies instead of around declarations
  - Updated VehicleSolver.cpp to use new struct names and simplified implementation
- Replication system still complete (100%)

### 2025-12-15 (Session 2)
- **Implemented P2P Replication** - Full implementation complete
  - Added `P2P=1` define to `GRIT.Build.cs`
  - Added `FReplicatedVehicleState` USTRUCT for state replication
  - Added `FReplicatedVehicleInput` USTRUCT for client→host input
  - Added `EP2PVehicleEvent` enum for event broadcasting
  - Added replication members to `AVehicleSolver`: `ReplicatedState`, interpolation state, timing
  - Added `HasPhysicsAuthority()` / `IsSimulatedProxy()` helper functions
  - Added RPCs: `OnRep_VehicleState()`, `Server_SendInput()`, `Multicast_VehicleEvent()`
  - Modified `GetLifetimeReplicatedProps()` with `DOREPLIFETIME_CONDITION`
  - Modified `BeginPlay()` for host/client authority split
  - Modified `Tick()` for 60Hz state packing (host) and interpolation (client)
  - Modified `OnPreSimulate_Internal()` to early-out on clients
  - Implemented `PackReplicatedState()`, `ApplyReplicatedState()`, `InterpolateState()`
- Replication system now complete (100%)
- Remaining: VehicleConfigurator, UI/HUD/HMI

### 2025-12-15 (Session 1)
- Created `CODEBASE_SUMMARY.md` update with all missing components
- Created `Context.md` (this file) for progress tracking
- Created `.claude/CLAUDE.md` for coding preferences memory
- Documented VehicleSolver as ~90% complete
- Identified 3 major remaining systems: Replication, VehicleConfigurator, UI/HUD/HMI
