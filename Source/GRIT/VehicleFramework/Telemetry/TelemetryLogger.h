#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformTime.h"

//====================================================================================================================================================
//                                              TELEMETRY SAMPLE STRUCTURE
//                                    High-frequency vehicle state capture (60+ Hz)
//                                    Only compiled in non-shipping builds
//====================================================================================================================================================

#if !UE_BUILD_SHIPPING

struct FTelemetrySample
{
    //--------------------------------------------------------------------------
    // TIMING
    //--------------------------------------------------------------------------
    float Time_s = 0.0f;                    // [s] - Simulation time
    float DeltaTime_s = 0.0f;               // [s] - Frame delta time

    //--------------------------------------------------------------------------
    // ENGINE STATE
    //--------------------------------------------------------------------------
    float EngineRPM = 0.0f;                 // [rpm] - Current engine speed
    float EngineTorque_Nm = 0.0f;           // [Nm] - Output torque
    float EngineLoad_Nm = 0.0f;             // [Nm] - Load torque
    float EngineBraking_Nm = 0.0f;          // [Nm] - Engine braking torque
    float EnginePower_kW = 0.0f;            // [kW] - Output power

    //--------------------------------------------------------------------------
    // THERMAL STATE
    //--------------------------------------------------------------------------
    float CoolantTemp_K = 293.15f;          // [K] - Coolant temperature
    float OilTemp_K = 293.15f;              // [K] - Oil temperature

    //--------------------------------------------------------------------------
    // TURBO STATE
    //--------------------------------------------------------------------------
    float BoostGauge_bar = 0.0f;            // [bar] - Boost pressure above ambient
    float TurboShaftRPM = 0.0f;             // [rpm] - Turbo shaft speed
    float ManifoldPressure_kPa = 101.325f;  // [kPa] - Manifold absolute pressure

    //--------------------------------------------------------------------------
    // VEHICLE KINEMATICS
    //--------------------------------------------------------------------------
    float Speed_kmh = 0.0f;                 // [km/h] - Vehicle speed magnitude
    float ForwardSpeed_kmh = 0.0f;          // [km/h] - Longitudinal speed
    float LateralSpeed_kmh = 0.0f;          // [km/h] - Lateral speed
    float Acceleration_G = 0.0f;            // [G] - Longitudinal acceleration
    float LateralAccel_G = 0.0f;            // [G] - Lateral acceleration


    //--------------------------------------------------------------------------
    // TRANSMISSION STATE
    //--------------------------------------------------------------------------
    int32 GearCurrent = 0;                  // [-] - Current gear
    int32 GearTarget = 0;                   // [-] - Target gear
    bool bIsShifting = false;               // [-] - Shift in progress
    float TxCombinedRatio = 0.0f;           // [-] - Combined gear ratio

    //--------------------------------------------------------------------------
    // CLUTCH STATE
    //--------------------------------------------------------------------------
    float ClutchTorque_Nm = 0.0f;           // [Nm] - Transferred torque
    float ClutchEngagement = 0.0f;          // [0-1] - Engagement ratio
    float ClutchLockup = 0.0f;              // [0-1] - Lockup state
    float SlipRPM = 0.0f;                   // [rpm] - Clutch slip speed

    //--------------------------------------------------------------------------
    // INPUT STATE
    //--------------------------------------------------------------------------
    float Throttle = 0.0f;                  // [0-1] - Throttle input
    float Brake = 0.0f;                     // [0-1] - Brake input
    float Steering = 0.0f;                  // [-1,1] - Steering input
    float Handbrake = 0.0f;                 // [0-1] - Handbrake input

    //--------------------------------------------------------------------------
    // PER-WHEEL DATA (4 wheels: FL, FR, RL, RR)
    //--------------------------------------------------------------------------
    float Wheel_DriveTorque_Nm[4] = {0};    // [Nm] - Drive torque per wheel
    float Wheel_BrakeTorque_Nm[4] = {0};    // [Nm] - Brake torque per wheel
    float Wheel_Omega_rad_s[4] = {0};       // [rad/s] - Angular velocity
    float Wheel_Steer_deg[4] = {0};         // [deg] - Steer angle
    float Wheel_SlipRatio[4] = {0};         // [-] - Longitudinal slip
    float Wheel_SlipAngle_deg[4] = {0};     // [deg] - Lateral slip angle
    float Wheel_Load_N[4] = {0};            // [N] - Vertical load (total)
    float Wheel_StaticLoad_N[4] = {0};      // [N] - Static vertical load
    float Wheel_LoadTransfer_N[4] = {0};    // [N] - Dynamic load transfer
    float Wheel_Fx_N[4] = {0};              // [N] - Longitudinal force
    float Wheel_Fy_N[4] = {0};              // [N] - Lateral force
    float Wheel_SelfAlignTorque_Nm[4] = {0};// [Nm] - Self-aligning torque
    float Wheel_PneumaticTrail_m[4] = {0};  // [m] - Pneumatic trail
    bool Wheel_InContact[4] = {false};      // [-] - Ground contact state
    int32 Wheel_Code[4] = {0};              // [-] - Wheel position code (FL/FR/RL/RR raw int8 from E_WheelCode)

    //--------------------------------------------------------------------------
    // BRAKE THERMAL STATE
    //--------------------------------------------------------------------------
    float Brake_Pressure_Pa[4] = {0};       // [Pa] - Brake line pressure
    float Brake_Temp_K[4] = {0};            // [K] - Rotor temperature
    float Brake_FrictionCoeff[4] = {0};     // [-] - Temperature-dependent mu

    //--------------------------------------------------------------------------
    // SUSPENSION STATE
    //--------------------------------------------------------------------------
    float Susp_Displacement_cm[4] = {0};    // [cm] - Suspension travel
    float Susp_Velocity_cm_s[4] = {0};      // [cm/s] - Suspension velocity
    float Susp_Force_N[4] = {0};            // [N] - Suspension force

    //--------------------------------------------------------------------------
    // AERODYNAMICS
    //--------------------------------------------------------------------------
    float AeroDrag_N = 0.0f;                // [N] - Aerodynamic drag
    float AeroDownforce_N = 0.0f;           // [N] - Aerodynamic downforce

    //--------------------------------------------------------------------------
    // EVENT FLAGS
    //--------------------------------------------------------------------------
    int32 GearEvent = 0;                    // [-] - Gear change event
    int32 ClutchEvent = 0;                  // [-] - Clutch event
    int32 RpmBand = 0;                      // [-] - RPM band indicator
};


//====================================================================================================================================================
//                                              AERODYNAMICS SAMPLE STRUCTURE
//                                    Records aerodynamic forces and load distribution
//====================================================================================================================================================

struct FAerodynamicsSample
{
    //--------------------------------------------------------------------------
    // TIMING
    //--------------------------------------------------------------------------
    float Time_s = 0.0f;                    // [s] - Simulation time
    
    //--------------------------------------------------------------------------
    // VEHICLE STATE
    //--------------------------------------------------------------------------
    float Speed_kmh = 0.0f;                 // [km/h] - Vehicle speed
    float Speed_ms = 0.0f;                  // [m/s] - Vehicle speed (SI)
    float DynamicPressure_Pa = 0.0f;        // [Pa] - Dynamic pressure (q = 0.5*ρ*V²)
    
    //--------------------------------------------------------------------------
    // AERODYNAMIC TOTALS
    //--------------------------------------------------------------------------
    float TotalDrag_N = 0.0f;               // [N] - Total aerodynamic drag
    float TotalDownforce_N = 0.0f;          // [N] - Total aerodynamic downforce
    float AeroEfficiency = 0.0f;            // [-] - L/D ratio (downforce/drag)
    
    //--------------------------------------------------------------------------
    // BODY AERODYNAMICS
    //--------------------------------------------------------------------------
    float BodyDrag_N = 0.0f;                // [N] - Body drag force
    float BodyDownforce_N = 0.0f;           // [N] - Body downforce
    
    //--------------------------------------------------------------------------
    // COMPONENT FORCES
    //--------------------------------------------------------------------------
    float WingDownforce_N = 0.0f;           // [N] - Rear wing downforce
    float WingDrag_N = 0.0f;                // [N] - Rear wing drag
    float CanardDownforce_N = 0.0f;         // [N] - Total canards downforce
    float CanardDrag_N = 0.0f;              // [N] - Total canards drag
    float SplitterDownforce_N = 0.0f;       // [N] - Splitter downforce
    float SplitterDrag_N = 0.0f;            // [N] - Splitter drag
    float LeftSideSkirtDownforce_N = 0.0f;  // [N] - Left side skirt downforce
    float LeftSideSkirtDrag_N = 0.0f;       // [N] - Left side skirt drag
    float RightSideSkirtDownforce_N = 0.0f; // [N] - Right side skirt downforce
    float RightSideSkirtDrag_N = 0.0f;      // [N] - Right side skirt drag
    float VortexGenDownforceBonus_N = 0.0f; // [N] - Vortex generator downforce bonus
    float VortexGenDrag_N = 0.0f;           // [N] - Vortex generator drag
    
    //--------------------------------------------------------------------------
    // UNDERBODY / GROUND EFFECT
    //--------------------------------------------------------------------------
    float UnderbodyDrag_N = 0.0f;           // [N] - Underbody drag
    float UnderbodyDownforce_N = 0.0f;      // [N] - Underbody downforce (ground effect)
    float MinRideHeight_cm = 0.0f;          // [cm] - Minimum ride height
    float RideHeightModifier = 1.0f;        // [-] - Ground effect multiplier
    
    //--------------------------------------------------------------------------
    // AERO BALANCE (Front/Rear Distribution)
    //--------------------------------------------------------------------------
    float FrontDownforce_N = 0.0f;          // [N] - Front axle downforce
    float RearDownforce_N = 0.0f;           // [N] - Rear axle downforce
    float AeroBalance_Pct = 50.0f;          // [%] - Front aero balance (0-100)
    
    //--------------------------------------------------------------------------
    // PER-WHEEL DOWNFORCE (4 wheels: FL, FR, RL, RR)
    //--------------------------------------------------------------------------
    float Wheel_AeroDownforce_N[4] = {0};   // [N] - Aero downforce per wheel
    
    //--------------------------------------------------------------------------
    // PER-WHEEL LOADS (4 wheels: FL, FR, RL, RR)
    //--------------------------------------------------------------------------
    float Wheel_StaticLoad_N[4] = {0};      // [N] - Static load per wheel
    float Wheel_TotalLoad_N[4] = {0};       // [N] - Total load (static + aero + transfer)
    float Wheel_LoadTransfer_N[4] = {0};    // [N] - Dynamic load transfer
    float Wheel_AeroContribution_N[4] = {0};// [N] - Aero contribution to load
    
    //--------------------------------------------------------------------------
    // LOAD TRANSFER COMPONENTS
    //--------------------------------------------------------------------------
    float LongLoadTransfer_N = 0.0f;        // [N] - Longitudinal load transfer (total)
    float LatLoadTransfer_N = 0.0f;         // [N] - Lateral load transfer (total)
    float LongAccel_G = 0.0f;               // [G] - Longitudinal acceleration
    float LatAccel_G = 0.0f;                // [G] - Lateral acceleration
};

//====================================================================================================================================================
//                                              PERFORMANCE SAMPLE STRUCTURE
//                                    Records execution time of each physics function
//====================================================================================================================================================

struct FPerformanceSample
{
    float Time_s = 0.0f;                        // [s] - Simulation time
    float DeltaTime_ms = 0.0f;                  // [ms] - Frame delta time
    
    // Physics pipeline timing (microseconds)
    float SuspensionDisplacements_us = 0.0f;    // [μs] - ComputeSuspensionDisplacements
    float SuspensionForces_us = 0.0f;           // [μs] - ComputeSuspensionForces
    float LoadTransfer_us = 0.0f;               // [μs] - ComputeLoadTransferLagrange
    float AntiRollbar_us = 0.0f;                // [μs] - ComputeAntiRollbarForces
    float Steering_us = 0.0f;                   // [μs] - ProcessSteering
    float ContactSlip_us = 0.0f;                // [μs] - SolveContactSlip (all wheels)
    float Powertrain_us = 0.0f;                 // [μs] - SolvePowertrain
    float Aerodynamics_us = 0.0f;               // [μs] - SolveAerodynamics (body + underbody)
    float PacejkaForces_us = 0.0f;              // [μs] - ComputeCombinedPacejkaForces
    float ForceApplication_us = 0.0f;           // [μs] - Force/torque application
    float TotalFrame_us = 0.0f;                 // [μs] - Total physics tick time
    
    // Call counts per frame
    int32 PacejkaCalls = 0;                     // [-] - Number of Pacejka evaluations
    int32 TraceCount = 0;                       // [-] - Number of line traces
};

//====================================================================================================================================================
//                                              TRANSMISSION/CLUTCH EVENT SAMPLE
//                                    Replaces runtime UE_LOG spam in VehicleSolver
//                                    (Audio, Launch, Clutch, ShiftGate, DriveSplit, Handbrake)
//====================================================================================================================================================

struct FTransmissionTelemetrySample
{
    // Source tag: 0=Audio, 1=Launch, 2=Clutch, 3=ShiftGate, 4=DriveSplit, 5=Handbrake
    uint8 EventType = 0;

    // Common timing
    float Time_s = 0.0f;

    // Engine / drivetrain
    float EngineRPM = 0.0f;
    float EngineTorque_Nm = 0.0f;
    float Boost_bar = 0.0f;
    int32 GearCurrent = 0;
    int32 GearTarget = 0;
    uint8 bIsShifting = 0;
    float ClutchEngagement = 0.0f;
    float ClutchEngagement_Prev = 0.0f;     // Launch-only
    float ClutchTargetEngagement = 0.0f;    // Launch-only
    float ClutchInterpSpeed = 0.0f;         // Launch-only
    float ClutchCapacity_Nm = 0.0f;         // Clutch-only
    float ClutchTorque_Nm = 0.0f;
    float SlipRPM = 0.0f;
    float TransInputRPM = 0.0f;             // Clutch-only
    uint8 bClutchLocked = 0;                // Clutch-only
    uint8 bSaturated = 0;                   // Clutch-only

    // Inputs / motion
    float Throttle = 0.0f;
    float Handbrake = 0.0f;                 // Handbrake-only
    float ForwardSpeed_ms = 0.0f;
    float LateralSpeed_ms = 0.0f;

    // ShiftGate-specific
    float UpshiftRPM = 0.0f;
    float PredictedNextRPM = 0.0f;
    float MinRequiredRPM = 0.0f;
    float GearHysteresisTimer = 0.0f;
    uint8 bShouldUpshift = 0;

    // DriveSplit-specific
    float TransOutputTorque_Nm = 0.0f;
    float TorqueToFront_Nm = 0.0f;
    float TorqueToRear_Nm = 0.0f;
    float RearSplit_K = 0.0f;
    float RearSplit_V = 0.0f;
    float DriveTq_W[4] = {0};
    int32 WheelCodes[4] = {0};
    int32 FL_n = 0;
    int32 FR_n = 0;
    int32 RL_n = 0;
    int32 RR_n = 0;
    uint8 bRearBlockRan = 0;
    int32 CenterDiff_DriveConfig = 0;
    int32 CenterDiff_Type = 0;
    float CenterDiff_FrontRearBias = 0.0f;

    // Handbrake-specific
    int32 WheelIndex = 0;
    uint8 bIsRearWheel = 0;
    float HandbrakeTarget_Pa = 0.0f;
    float BrakePressure_Pa = 0.0f;
    float BrakeTorque_Nm = 0.0f;
    float WheelOmega_rads = 0.0f;
    float Speed_ms = 0.0f;
    float BrakeTemp_K = 0.0f;
    float BrakeFricCoeff = 0.0f;
};

//====================================================================================================================================================
//                                              SCOPED TIMER HELPER
//                                    RAII timer for measuring function execution
//====================================================================================================================================================

class FScopedTimer
{
public:
    FScopedTimer(float& OutMicroseconds)
        : Output(OutMicroseconds)
        , StartCycles(FPlatformTime::Cycles64())
    {}

    ~FScopedTimer()
    {
        uint64 EndCycles = FPlatformTime::Cycles64();
        Output = FPlatformTime::ToSeconds64(EndCycles - StartCycles) * 1000000.0;
    }

private:
    float& Output;
    uint64 StartCycles;
};

// Macro for easy scoped timing
#define SCOPE_TIMER(OutVar) FScopedTimer _ScopedTimer_##__LINE__(OutVar)


//====================================================================================================================================================
//                                              TELEMETRY HELPER
//                                    Populates samples from vehicle state vectors
//====================================================================================================================================================

// Forward declarations
struct FInstantaneousVehicleRecord;
struct FInputTensor;
struct FEngineStateVector;
struct FTransmissionStateVector;
struct FClutchStateVector;
struct FVehicleSolverAxleData_PT;
struct FBrakingStateVector;

class FTelemetryHelper
{
public:
    /** Populate a telemetry sample from vehicle state */
    static void PopulateSample(
        FTelemetrySample& Sample,
        float SimTime,
        float DeltaTime,
        const FInstantaneousVehicleRecord& Rec,
        const FInputTensor& Input,
        const FEngineStateVector& EngineState,
        const FTransmissionStateVector& TransState,
        const FClutchStateVector& ClutchState,
        const FVehicleSolverAxleData_PT& AxleData,
        const TArray<FBrakingStateVector>& BrakeStates,
        float BoostGaugeBar,
        float TurboRPM,
        float ManifoldPressure_Pa,
        float CoolantTemp_K,
        float OilTemp_K,
        const TArray<float>& StaticLoads = TArray<float>()
    );
};

#endif // !UE_BUILD_SHIPPING
