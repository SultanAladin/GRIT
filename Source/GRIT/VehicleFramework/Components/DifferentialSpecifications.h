#pragma once
#include "CoreMinimal.h"
#include "Containers/Array.h"

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                              ⚙️ DIFFERENTIAL SYSTEM
//----------------------------------------------------------------------------------------------------------------------------------------

/** Differential architecture */
UENUM(BlueprintType)
enum class EDifferentialType : uint8
{
    Open,
    LimitedSlip,
    Locking,
    TorqueVectoring
};

/** Vehicle drive layout */
UENUM(BlueprintType)
enum class EDriveConfiguration : uint8
{
    RWD,
    FWD,
    AWD
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                  🧩 Differential Specifications (Immutable)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FDifferentialSpecifications
{
    FDifferentialSpecifications(int32 PresetID = 1)
    {
        Type = EDifferentialType::Open;
        DriveConfig = EDriveConfiguration::RWD;
        DiffInertia = 0.05f;
        DiffEfficiency = 0.97f;
        FinalDriveRatio = 3.55f;
        FrontRearBias = 0.5f;
        LeftRightBias = 0.0f;
        LSDPreloadTorque = 0.0f;
        LSDLockingCoefficient = 0.0f;
        TVMaxTorqueTransfer = 0.0f;

        ConfigureDifferential(PresetID);
    }

    /* ================================================= ARCHITECTURE ================================================= */
    
    EDifferentialType Type;                             // [-] - Differential mechanism type
    EDriveConfiguration DriveConfig;                    // [-] - Drive layout configuration
    
    /* ================================================= PHYSICAL PROPERTIES ================================================= */
    
    float DiffInertia;                                  // [kg·m²] - Differential rotational inertia
    float DiffEfficiency;                               // [-] - Mechanical efficiency
    float FinalDriveRatio;                              // [ratio] - Final drive gear ratio
    
    /* ================================================= TORQUE DISTRIBUTION ================================================= */
    
    float FrontRearBias;                                // [0-1] - Front axle torque fraction
    float LeftRightBias;                                // [-1 to 1] - Left/right torque bias
    float LSDPreloadTorque;                             // [N·m] - Limited slip preload
    float LSDLockingCoefficient;                        // [0-1] - Limited slip locking factor
    float TVMaxTorqueTransfer;                          // [N·m] - Max torque vectoring transfer
    TArray<FVector2D> TVResponseCurve;                  // [slip_ratio, torque_factor] - Torque vectoring response
    
    /* ================================================= CONFIGURATION ================================================= */
    
    void ConfigureDifferential(int32 InPresetID)
    {
        if (InPresetID == 1) // Reason: GT-R preset with torque vectoring
        {
            Type = EDifferentialType::TorqueVectoring;
            DriveConfig = EDriveConfiguration::AWD;
            DiffInertia = 0.05f;
            DiffEfficiency = 0.97f;
            FinalDriveRatio = 3.55f;
            FrontRearBias = 0.35f;
            LeftRightBias = 0.0f;
            LSDPreloadTorque = 120.0f;
            LSDLockingCoefficient = 0.6f;
            TVMaxTorqueTransfer = 400.0f;

            TVResponseCurve = {FVector2D(0.0f, 0.0f), FVector2D(0.1f, 0.2f), FVector2D(0.3f, 0.6f), FVector2D(0.6f, 1.0f)};

            TraceConfiguration();
        } // End if (preset 1 configuration)

        else if (InPresetID == 2) // Reason: Front Axle LSD
        {
            Type = EDifferentialType::LimitedSlip;
            DriveConfig = EDriveConfiguration::FWD;
            LSDPreloadTorque = 40.0f;
            LSDLockingCoefficient = 0.4f;
        } // End if (preset 2 configuration)

        else if (InPresetID == 3) // Reason: Rear Axle LSD
        {
            Type = EDifferentialType::LimitedSlip;
            DriveConfig = EDriveConfiguration::RWD;
            LSDPreloadTorque = 80.0f;
            LSDLockingCoefficient = 0.5f;
        } // End if (preset 3 configuration)
    }
    
    /* ================================================= TORQUE CALCULATIONS ================================================= */
    
    /**
     * Calculates output torques for a differential based on input torque and output shaft speeds.
     * Handles center (front/rear) and axle (left/right) diffs uniformly.
     */
    void CalculateOutputTorques(float InputTorque, float OmegaA, float OmegaB, float& OutTorqueA, float& OutTorqueB) const
    {
        float BaseSplitA = 0.5f;
        if (DriveConfig == EDriveConfiguration::AWD)
        {
            BaseSplitA = FrontRearBias;
        }
        else
        {
            BaseSplitA = 0.5f - (LeftRightBias * 0.5f);
        }

        float BaseTorqueA = InputTorque * BaseSplitA;
        float BaseTorqueB = InputTorque * (1.0f - BaseSplitA);

        if (Type == EDifferentialType::Open || FMath::Abs(InputTorque) < KINDA_SMALL_NUMBER)
        {
            OutTorqueA = BaseTorqueA;
            OutTorqueB = BaseTorqueB;
            return;
        }

        const float DeltaOmega = OmegaA - OmegaB;
        const float LockingFactor = LSDLockingCoefficient * 1000.0f;
        float LockingTorque = LSDPreloadTorque + (FMath::Abs(DeltaOmega) * LockingFactor);

        if (Type == EDifferentialType::Locking)
        {
            LockingTorque = FMath::Max(LockingTorque, FMath::Abs(InputTorque));
        }

        const float MaxTransfer = FMath::Abs(InputTorque * 0.5f);
        LockingTorque = FMath::Clamp(LockingTorque, 0.0f, MaxTransfer);

        if (DeltaOmega > 0.01f)
        {
            OutTorqueA = BaseTorqueA - LockingTorque;
            OutTorqueB = BaseTorqueB + LockingTorque;
        }
        else if (DeltaOmega < -0.01f)
        {
            OutTorqueA = BaseTorqueA + LockingTorque;
            OutTorqueB = BaseTorqueB - LockingTorque;
        }
        else
        {
            OutTorqueA = BaseTorqueA;
            OutTorqueB = BaseTorqueB;
        }
    }
    
    /* ================================================= DEBUGGING ================================================= */
    
    void TraceConfiguration() const
    {
        UE_LOG(LogTemp, Log, TEXT("--- Differential Configuration ---"));
        UE_LOG(LogTemp, Log, TEXT("Type: %d | Drive: %d | Final Drive: %.2f"), (int32)Type, (int32)DriveConfig, FinalDriveRatio);
        UE_LOG(LogTemp, Log, TEXT("Front/Rear Bias: %.2f | Left/Right Bias: %.2f"), FrontRearBias, LeftRightBias);
        UE_LOG(LogTemp, Log, TEXT("LSD Preload: %.1f N·m | Lock: %.2f | TV Max: %.1f N·m"), LSDPreloadTorque, LSDLockingCoefficient, TVMaxTorqueTransfer);
        UE_LOG(LogTemp, Log, TEXT("----------------------------------"));
    }
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                  ⚡ Differential Runtime State (Mutable)
//----------------------------------------------------------------------------------------------------------------------------------------

/** Differential state vector - Runtime state (physics thread only) */
struct FDifferentialStateVector
{
    float CurrentTorque = 0.0f;                         // [N·m] - Current input torque
    float FrontAxleTorque = 0.0f;                       // [N·m] - Front axle output torque
    float RearAxleTorque = 0.0f;                        // [N·m] - Rear axle output torque
    float LeftWheelTorque = 0.0f;                       // [N·m] - Left wheel torque
    float RightWheelTorque = 0.0f;                      // [N·m] - Right wheel torque
    float SlipRatio = 0.0f;                             // [-] - Current slip ratio
    bool bConfigurationChanged = false;                 // [-] - Configuration change flag

    FDifferentialStateVector() = default;
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                  🛡️ Traction Control System (TCS)
//----------------------------------------------------------------------------------------------------------------------------------------

/** TCS operating mode */
UENUM(BlueprintType)
enum class ETCSMode : uint8
{
    Off,           // Disabled
    Sport,         // High slip threshold (0.15-0.25)
    Comfort,       // Medium slip threshold (0.10-0.15)
    Snow,          // Low slip threshold (0.05-0.10)
    Track          // Very aggressive (0.20-0.30)
};

/** TCS control method */
UENUM(BlueprintType)
enum class ETCSMethod : uint8
{
    EngineOnly,    // Throttle/torque reduction only
    BrakeOnly,     // Wheel braking only
    Combined       // Coordinated engine + brake
};

/** TCS specifications - Immutable configuration */
struct FTCSSpecifications
{
    bool bEnabled = true;                              // [-] - System enabled
    ETCSMode Mode = ETCSMode::Sport;                   // [-] - Operating mode
    ETCSMethod Method = ETCSMethod::Combined;          // [-] - Control method
    
    float SlipThresholdLow = 0.10f;                    // [-] - Activation threshold
    float SlipThresholdHigh = 0.20f;                   // [-] - Max allowed slip
    float TargetSlipRatio = 0.12f;                     // [-] - Target operating point
    
    float EngineGainP = 800.0f;                        // [N·m/slip] - Proportional gain
    float EngineGainD = 200.0f;                        // [N·m·s/slip] - Derivative gain
    
    float BrakeGainP = 400.0f;                         // [N·m/slip] - Proportional gain
    float BrakeMaxTorque = 2000.0f;                    // [N·m] - Max brake torque
    
    float UpdateRate_Hz = 50.0f;                       // [Hz] - Control loop frequency
    float MinVehicleSpeed_ms = 1.0f;                   // [m/s] - Min activation speed

    FTCSSpecifications() = default;
    
    /** Get slip thresholds based on mode */
    void GetModeThresholds(float& OutLow, float& OutTarget, float& OutHigh) const
    {
        switch (Mode)
        {
            case ETCSMode::Off:
                OutLow = 1.0f; OutTarget = 1.0f; OutHigh = 1.0f;
                break;
            case ETCSMode::Snow:
                OutLow = 0.05f; OutTarget = 0.08f; OutHigh = 0.12f;
                break;
            case ETCSMode::Comfort:
                OutLow = 0.08f; OutTarget = 0.12f; OutHigh = 0.18f;
                break;
            case ETCSMode::Sport:
                OutLow = 0.12f; OutTarget = 0.15f; OutHigh = 0.22f;
                break;
            case ETCSMode::Track:
                OutLow = 0.18f; OutTarget = 0.22f; OutHigh = 0.30f;
                break;
            default:
                OutLow = SlipThresholdLow;
                OutTarget = TargetSlipRatio;
                OutHigh = SlipThresholdHigh;
                break;
        }
    }
};

/** TCS runtime state - Mutable physics thread state */
struct FTCSState
{
    bool bActive = false;                              // [-] - Currently intervening
    float PrevSlipError = 0.0f;                        // [-] - Previous error (for derivative)
    float EngineTorqueReduction = 0.0f;                // [N·m] - Current reduction
    TArray<float> WheelBrakeTorques;                   // [N·m] - Per-wheel brake commands
    float TimeSinceLastUpdate = 0.0f;                  // [s] - Update timing
    float TorqueScaleFactor = 1.0f;                    // [-] - Engine torque multiplier (0-1)

    FTCSState() = default;
    
    void Initialize(int32 WheelCount)
    {
        WheelBrakeTorques.SetNum(WheelCount);
        for (int32 i = 0; i < WheelCount; ++i)
        {
            WheelBrakeTorques[i] = 0.0f;
        }
    }
    
    void Reset()
    {
        bActive = false;
        PrevSlipError = 0.0f;
        EngineTorqueReduction = 0.0f;
        TimeSinceLastUpdate = 0.0f;
        TorqueScaleFactor = 1.0f;
        for (float& BrakeTq : WheelBrakeTorques)
        {
            BrakeTq = 0.0f;
        }
    }
};