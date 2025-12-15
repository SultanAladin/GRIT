# GRIT Vehicle Framework - Codebase Summary

**Version:** 1.1
**Last Updated:** 2025-12-15
**Purpose:** AI Context Document - Eliminates need for repeated codebase explanations

---

## 1. ARCHITECTURE OVERVIEW

### 1.1 Core Philosophy
The GRIT VehicleFramework implements **AAA-quality vehicle simulation** using a physics-first approach with thread-safe architecture. All physics computations run on the **Chaos Physics Thread (PT)** with immutable specifications and mutable state vectors.

### 1.2 Directory Structure
```
VehicleFramework/
├── VehicleSolver.cpp/h          # Main physics solver (5700+ lines)
├── VehicleConstruct.cpp/h       # Actor initialization, camera setup
├── Components/                  # Vehicle component specifications
│   ├── TireSpecifications.h             # Pacejka MF6.1 tire model
│   ├── SuspensionSpecifications.h       # Progressive spring/damper
│   ├── DrivetrainSpecifications.h       # Engine/Trans/Diff aggregator
│   ├── EngineSpecifications.h           # Engine torque curves, thermal
│   ├── TurbochargerSpecifications.h     # Twin-scroll turbo, wastegate, BOV
│   ├── SuperchargerSpecifications.h     # Centrifugal/Roots supercharger
│   ├── ElectricMotorSpecifications.h    # EV/Hybrid motor model
│   ├── BatterySpecifications.h          # Li-ion battery pack simulation
│   ├── ClutchSpecifications.h           # Sigmoid clutch engagement
│   ├── TransmissionSpecifications.h     # Sequential/H-pattern gearbox
│   ├── DifferentialSpecifications.h     # Open/LSD/Lock/TV diff modes
│   ├── AerodynamicSpecifications.h      # Comprehensive aero package
│   ├── AxleSpecifications.h             # Per-wheel assembly config
│   ├── BrakingSpecifications.h          # Hydraulic brake, thermal fade
│   ├── SteeringAssembly.h               # Ackermann steering, speed-sense
│   ├── ChassisConfiguration.h           # Wheelbase, track, roll centers
│   ├── AntiRollbarSpecifications.h      # ARB stiffness config
│   ├── FuelControlSystem.h              # Fuel injection & AFR
│   ├── CoolantSpecifications.h          # Cooling system model
│   ├── EngineOilSpecifications.h        # Oil viscosity, temp effects
│   ├── TireSpecifications/              # .tyrx tire data files
│   └── Constructs/                      # Tire construct actors
│       └── TireConstruct.cpp/h          # Tire source/mesh provider
├── Controllers/                 # Input handling
│   └── VehicleController.cpp/h          # Enhanced Input System bindings
├── Input/                       # Input tensor definitions
│   └── InputTensor.h                    # Comprehensive input structure
├── Telemetry/                   # CSV logging for debugging
│   └── TelemetryLogger.cpp/h            # FTelemetrySample, FPerformanceSample
├── UserInterfaces/              # UI components
│   └── GraphicalUserInterface.cpp/h     # Base UI widget with theming
└── Docs/                        # Documentation
    ├── CODEBASE_SUMMARY.md              # This file
    ├── VehiclePhysicsReplication_Guide.md
    ├── JitterFix.md
    └── Networking/
        └── P2P_Physics_Replication_Guide.md
```

### 1.3 Thread Model
| Component | Thread | Data Structure |
|-----------|--------|----------------|
| Specifications | Any (Immutable) | `F*Specifications` |
| Physics State | Physics Thread | `F*StateVector`, `*_PT` suffix |
| Game State | Game Thread | `*_GT` suffix |
| Telemetry | Game Thread (Buffered) | `F*Sample` |

---

## 2. CORE CLASSES

### 2.1 AVehicleSolver (Actor)
**Location:** `VehicleSolver.cpp/h`

The main vehicle physics actor. Owns all vehicle state and orchestrates the physics simulation.

**Key Responsibilities:**
- Registers `FVehicleSolverCallback` with Chaos physics
- Manages Game Thread ↔ Physics Thread synchronization
- Handles telemetry buffering and CSV export
- Provides debug visualization

**Important Members:**
```cpp
UStaticMeshComponent* VehicleHull;           // Collision geometry
FVehicleSolverCallback* PhysicsCallback;     // Physics thread delegate
FInstantaneousVehicleRecord VehicleRecord_PhysicsThread; // Kinematic state
TArray<FTelemetrySample> TelemetrySampleBuffer;
```

### 2.2 FVehicleSolverCallback (Chaos Callback)
**Location:** `VehicleSolver.cpp:100-5241`

The Chaos physics callback that executes all per-frame physics computations.

**Execution Order (OnPreSimulate_Internal):**
1. Input Processing → `ProcessSteering()`
2. Suspension Displacement → Ray/sweep traces
3. Suspension Forces → Progressive spring model
4. Load Transfer → `ComputeLoadTransferRealtime()`
5. Anti-Roll Bars → `ComputeAntiRollbarForces()`
6. Tire Forces → `SolveContactSlip()` + Pacejka MF6.1
7. Powertrain → `SolvePowertrain()` (Engine, Clutch, Transmission, Diff)
8. Aerodynamics → `ComputeAerodynamicForces()`
9. Force Application → `RigidBody->AddForce/AddTorque`
10. State Persistence → Update `*_PT` arrays

---

## 3. PHYSICS MODELS

### 3.1 Tire Model (Pacejka MF6.1)
**Files:** `TireSpecifications.h`, `VehicleSolver.cpp:3005-3651`

**Core Functions:**
- `ComputePacejkaLongitudinalForce()` - Fx from slip ratio κ
- `ComputePacejkaLateralForce()` - Fy from slip angle α
- `ComputeSelfAligningTorque()` - Mz with combined slip correction
- `ApplyCombinedSlip()` - MF6.1 ellipse-based weighting

**Key Innovation - Implicit Newton Solver:**
```cpp
// VehicleSolver.cpp:3088-3113
// Solves: κ = f(Ω_next, Fx(κ)) simultaneously
// Benefits: Zero-lag transient response, stable at peak force
```

**Precomputed Cache:**
```cpp
FPacejkaPrecomputedCache {
    float Cx_scaled, Cy_scaled;      // Shape factors × scaling
    float pDx1_Lx, pDy1_Ly;          // Peak factors × scaling
    float InvFz0;                     // Precomputed 1/Fz0
    // ... 70%+ of per-frame calculations eliminated
};
```

### 3.2 Suspension Model
**File:** `SuspensionSpecifications.h`

**Progressive Spring (Cubic Polynomial):**
```
F = k₁·x + k₂·x² + k₃·x³
k_eff = dF/dx = k₁ + 2k₂·x + 3k₃·x²
```

**Curve Types:**
| Type | Behavior | Use Case |
|------|----------|----------|
| Linear | Constant rate | Basic |
| Progressive | Rising rate | GT3 race cars |
| Digressive | Falling rate | Comfort |
| DualRate | Two-stage | Rally |

### 3.3 Powertrain Model
**File:** `VehicleSolver.cpp:3656-5241`

**Components (in order):**
1. **Turbocharger** (Lines 3728-3904)
   - Isentropic turbine/compressor model
   - Internal heat transfer (Serrano et al. 2007)
   - BOV surge protection
   - Wastegate control

2. **Engine** (Lines 3906-4034)
   - Torque curves sampled from data
   - Viscosity-dependent friction
   - RK4 integration (decoupled mode)
   - Lumped inertia (locked mode)

3. **Clutch** (Lines 4067-4203)
   - Sigmoid progressive engagement
   - Stiffness-based slip model
   - Lock-up detection

4. **Transmission** (Lines 4204-4376)
   - Dynamic torque-based upshift
   - Braking downshift aggression
   - Reverse gear sign handling

5. **Differential** (Lines 4388-4613)
   - Open, LSD, Locking, Torque Vectoring modes
   - Center/Front/Rear topology
   - Deadband to prevent asymmetric pull

### 3.4 Aerodynamics Model
**Files:** `AerodynamicSpecifications.h`, `VehicleSolver.cpp:4615-4649`

**Components:**
- Body (Cd, Cl, frontal/side area)
- Rear Wing (adaptive angle, brake deploy)
- Front Splitter (ride height sensitive)
- Canards (damage modeling)
- Underbody/Diffuser (ground effect)
- Side Skirts (underbody sealing)
- Vortex Generators (diffuser enhancement)

**Ride Height Effect:**
```
h < h_critical: Force fades linearly (porpoising)
h_critical < h < h_optimal: Transition zone (60% → 100%)
h > h_optimal: Inverse power law decay (∝ 1/h^1.2)
```

### 3.5 Stiction State Machine
**File:** `VehicleSolver.cpp:4653-4961`

**States:**
| State | Condition | Behavior |
|-------|-----------|----------|
| Kinetic | v > 0.2 m/s | Pacejka active |
| Static_Lateral | v < 0.2, Lat hold OK | Fy clamped, Fx kinetic |
| Static_Full | v < 0.2, Both hold OK | Full slope compensation |

**Key Feature:** Friction circle budget prioritizes slope hold over damping.

---

## 4. DATA FLOW

### 4.1 Input Pipeline
```
VehicleController::Tick()
    ↓ Enhanced Input System
FInputTensor { Throttle, Brake, Steering, Handbrake, bReverseRequest }
    ↓ (Game Thread → Physics Thread copy)
FVehicleSolverCallback::OnPreSimulate_Internal()
```

### 4.2 Force Propagation
```
Engine Torque
    ↓ Clutch (engagement × slip)
Clutch Torque
    ↓ Transmission (GearRatio × FinalDrive)
Trans Output Torque
    ↓ Center Differential (AWD: FrontRearBias)
Prop Torques [Front, Rear]
    ↓ Axle Differentials (Open/LSD/Lock/TV)
Wheel Torques [FL, FR, RL, RR]
    ↓ Tire Model (Pacejka Fx)
Ground Forces
    ↓ RigidBody->AddForce()
```

### 4.3 Vehicle State Record
```cpp
FInstantaneousVehicleRecord {
    // Kinematics
    FVector ν_linearCms;        // [cm/s] Linear velocity
    FVector ω_angularRads;      // [rad/s] Angular velocity
    float ν_magnitudeMs;        // [m/s] Speed
    float ν_forwardMs;          // [m/s] Longitudinal
    float ν_lateralMs;          // [m/s] Lateral

    // Reference Frames
    FVector ê_longitudinal;     // Forward unit vector
    FVector ê_lateral;          // Right unit vector
    FVector ê_vertical;         // Up unit vector
    FVector σ_centerOfMass;     // CoM world position [cm]

    // Mass
    float μ_mass;               // [kg]

    // Acceleration
    float α_g_longitudinal;     // [g] Forward accel
    float α_g_lateral;          // [g] Lateral accel
};
```

---

## 5. KEY CONSTANTS

### 5.1 Unit Conversions
| Constant | Value | Description |
|----------|-------|-------------|
| `GravityM` | 9.81 | m/s² |
| `RadSToRPM` | 9.5493 | rad/s → RPM |
| `RPMToRadS` | 0.1047 | RPM → rad/s |
| Chaos scale | 100 | 1 m = 100 cm (UU) |

### 5.2 Solver Parameters
| Parameter | Value | Purpose |
|-----------|-------|---------|
| Newton iterations | 12 | Slip solver convergence |
| κ clamp | [-1.0, 1.5] | Prevent Pacejka wraparound |
| α clamp | ±1.48 rad | ~85° max slip angle |
| Stop threshold | 0.2 m/s | Stiction activation |
| Static μ | 1.0 | Holding coefficient |

### 5.3 GT3 Preset Values
| Component | Front | Rear |
|-----------|-------|------|
| Spring Freq | 2.5 Hz | 2.3 Hz |
| Damping Ratio | 0.7 | 0.65 |
| Hardening | 1.1× | 1.6× |
| ARB | 60 kN/m | 95 kN/m |
| Brake Pressure | 12 MPa | 8 MPa |

---

## 6. TELEMETRY SYSTEM

### 6.1 Available Logs
| File | Content | Trigger |
|------|---------|---------|
| `PowertrainLog.csv` | Engine, trans, clutch, wheel states | `WriteTelemetryCsv()` |
| `PerformanceLog.csv` | Per-pass timing in μs | `WritePerformanceCsv()` |
| `AerodynamicsLog.csv` | Forces, ride height, balance | `WriteAerodynamicsCsv()` |
| `FrictionState.csv` | Stiction state machine debug | Frame counter modulo |

### 6.2 Key Telemetry Fields
```cpp
FTelemetrySample {
    float EngineRPM, EngineTorque_Nm, EngineLoad_Nm;
    float BoostRatio, TurboShaftRPM;
    int32 GearCurrent, GearTarget; bool bIsShifting;
    float ClutchTorque_Nm, ClutchEngagement;
    float Wheel_SlipRatio[4], Wheel_SlipAngle_deg[4];
    float Wheel_Load_N[4], Wheel_Fx_N[4], Wheel_Fy_N[4];
};
```

---

## 7. OPTIMIZATION STRATEGIES

### 7.1 Precomputation
- `FPacejkaPrecomputedCache`: All load-independent tire coefficients
- `FSteeringGeometryCache_PT`: Wheelbase, track width
- Progressive spring coefficients: k₁, k₂, k₃ computed once

### 7.2 Fast Math
```cpp
// TireSpecifications.h
FORCEINLINE float FastAtan(float x);  // 3.1× faster, max error 0.0002
FORCEINLINE float FastSin(float x);   // 2.8× faster
FORCEINLINE float FastCos(float x);   // 2.8× faster
```

### 7.3 Avoid Per-Frame
- No `TArray` reallocation in hot paths
- `SetNum()` used once at initialization
- Branch prediction hints via `[[likely]]`/`[[unlikely]]`

---

## 8. COMMON PATTERNS

### 8.1 Physics Thread Safety
```cpp
// Mutable state: *_PT suffix
FVehicleSolverAxleData_PT AxleData_PT;

// Read from specifications (immutable)
const FDrivetrainSpecifications& Specs = DrivetrainSpecs_PT;

// Write to state vectors
DrivetrainState_PT.EngineState.CurrentEngineRPM = NewRPM;
```

### 8.2 Unit Annotation
```cpp
const float Fz = WheelLoad;                    // [N]
const float Fz_kN = Fz * 0.001f;               // [kN]
const float dfz = (Fz_kN - Fz0) * InvFz0;      // [-]
```

### 8.3 Reason Comments
```cpp
if (Count == 1) // Reason: single spring, entire mass supported by one point
{
    OutSprungMasses[0] = TotalMass;
    return true;
} // End if (single spring)
```

---

## 9. DEBUGGING TIPS

### 9.1 Enable Telemetry
```cpp
// In VehicleSolver.h or console
GEnableTelemetryLogging = true;
GEnableFrictionStateLogging = true;
```

### 9.2 Debug Visualization
```cpp
// Aerodynamics debug (editor only)
AVehicleSolver::DrawAerodynamicsDebug();
```

### 9.3 Common Issues
| Symptom | Likely Cause | Check |
|---------|--------------|-------|
| Brake acceleration | Fx sign error | `VehicleSolver.cpp:4946` clamp |
| Uphill slide | Stiction threshold | Slope force vs grip budget |
| RPM oscillation | Clutch stiffness | `BreakawaySlipRPM` tuning |
| Asymmetric pull | Diff deadband | LSD/TV deadband constants |

---

## 10. REFERENCES

### Academic
- Pacejka, H.B. (2012) "Tire and Vehicle Dynamics" 3rd Edition
- Heywood, J.B. (1988) "Internal Combustion Engine Fundamentals"
- Milliken & Milliken (2020) "Race Car Vehicle Dynamics" 3rd Edition

### Standards
- ISO 8855:2011 - Road vehicle dynamics terminology
- SAE J2564 - Power Steering Systems
- SAE 2000-01-1633 - Bosch ESP Development

---

---

## 11. ADDITIONAL SYSTEMS (Not in Original)

### 11.1 Input System
**File:** `Input/InputTensor.h`

**FInputTensor** - Comprehensive input structure:
- Analog: Throttle, Brake, Steering, Handbrake, Clutch
- Transmission: ShiftUp, ShiftDown, GearUp, GearDown, TransModeToggle
- Engine Control: EngineToggle, Boost, OverDrive
- Traction: DiffLockToggle, TCSToggle, ABSToggle, StabilityToggle
- Advanced: LaunchControl, DriftMode, ResetVehicle, AerodynamicBraking
- Device Metadata: DeviceType, InputMagnitude, Timestamp

### 11.2 Aerodynamics Package
**File:** `Components/AerodynamicSpecifications.h`

**Components (Game Thread → Physics Thread):**
- `FWing_GT/PT` - Rear wing (adaptive, brake deploy)
- `FSplitter_GT/PT` - Front splitter, air dam
- `FCanard_GT/PT` - Front canards (damage modeling)
- `FUnderbody_GT/PT` - Floor + diffuser (ground effect)
- `FSideSkirt_GT/PT` - Side skirts (underbody sealing)
- `FVortexGenerators_GT/PT` - Boundary layer control
- `FBody_GT/PT` - Body drag/lift/side force

**Force Output:** `FAerodynamicForces_PT`
- Total drag/downforce, front/rear distribution
- Per-component breakdown for telemetry
- Pitch/roll/yaw moments

### 11.3 Full Drivetrain Component List
**File:** `Components/DrivetrainSpecifications.h`

The `FDrivetrainSpecifications` aggregates:
1. `FEngineSpecifications` - Torque curves, thermal model
2. `FEngineOilSpecifications` - Viscosity, temperature effects
3. `FTurbochargerSpecifications` - Twin-scroll, wastegate, BOV
4. `FSuperchargerSpecifications` - Centrifugal/Roots modes
5. `FElectricMotorSpecifications` - EV/Hybrid motor
6. `FBatterySpecifications` - Li-ion pack simulation
7. `FClutchSpecifications` - Sigmoid engagement, lock-up
8. `FTransmissionSpecifications` - Sequential/H-pattern
9. `FDifferentialSpecifications` (×3) - Center, Front, Rear
10. `FFuelControlSystem` - Injection, AFR control

### 11.4 VehicleController
**File:** `Controllers/VehicleController.h`

**Enhanced Input System Integration:**
- Input Actions: Throttle, Brake, Steer, Handbrake, GearUp/Down, Clutch, OverDrive
- Signal smoothing rates (ThrottleRate, BrakeRate, SteeringRate)
- Drive mode switching (reverse mode via double-tap)
- Aerodynamic braking toggle

### 11.5 Telemetry System
**File:** `Telemetry/TelemetryLogger.h`

**Sample Structures (Dev/Editor only):**
- `FTelemetrySample` - Complete vehicle state snapshot
- `FAerodynamicsSample` - Aero forces, load distribution
- `FPerformanceSample` - Per-function timing (μs)

**Helper Classes:**
- `FScopedTimer` - RAII timer for profiling
- `FTelemetryHelper::PopulateSample()` - State vector → sample

### 11.6 User Interface Foundation
**File:** `UserInterfaces/GraphicalUserInterface.h`

**UGraphicalUserInterface** - Base widget class:
- `FUIThemeConfiguration` - Colors, typography, layout
- Theme management: `ApplyTheme()`, `UpdateTheme()`
- Visibility: `ShowUI()`, `HideUI()`, `ToggleUI()`
- Blueprint events: `OnThemeApplied`, `OnUIShown`, `OnUIHidden`

---

## 12. CLASS HIERARCHY

```
APawn
└── AVehicleSolver          # Main physics solver pawn
    └── AVehicleConstruct   # Player vehicle with camera

UUserWidget
└── UGraphicalUserInterface # Base UI widget with theming

APlayerController
└── AVehicleController      # Enhanced Input System controller

AActor
└── ATireConstruct          # Tire specification/mesh provider
```

---

## 13. NETWORKING ARCHITECTURE

### 13.1 P2P Replication
**File:** `Docs/Networking/P2P_Physics_Replication_Guide.md`

**Architecture:**
- Host runs full physics simulation
- Clients receive state via `FReplicatedVehicleState_Compact` (48 bytes)
- Clients interpolate between received states
- Input sent from client → host via Server RPC

**Bandwidth (50+ vehicles):**
| Vehicles | 30 Hz | 20 Hz |
|----------|-------|-------|
| 50 | 72 KB/s | 48 KB/s |
| 100 | 144 KB/s | 96 KB/s |

**Optimizations:**
- Distance-based LOD (adaptive update rates)
- Delta compression
- Relevancy culling
- Batched updates

---

*This document is auto-generated from codebase analysis. Update after major refactors.*
