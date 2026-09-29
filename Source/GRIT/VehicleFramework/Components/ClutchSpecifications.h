#pragma once

#include "CoreMinimal.h"

/*====================================================================================================================================
                                                         🔧 CLUTCH SYSTEM
======================================================================================================================================*/

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                       📋 ENUMERATIONS
//----------------------------------------------------------------------------------------------------------------------------------------

/** Clutch actuation control mode */
UENUM(BlueprintType)
enum class EClutchControl : uint8
{
    Manual,         // Driver pedal input
    Automatic,      // AI-controlled engagement
    DualClutch      // DCT overlap logic
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                     🧩 Clutch Specifications (Immutable)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FClutchSpecifications
{
    FClutchSpecifications(int32 PresetID = 1)
    {
        ControlType = EClutchControl::Manual;
        MaxTorqueCapacity = 1200.0f;         // [N·m]
        EngagementTime = 0.15f;              // [s]
        DisengagementTime = 0.12f;           // [s]
        BreakawaySlipRPM = 300.0f;           // [rev/min]
        LockSlipRPM = 50.0f;                 // [rev/min]
        LaunchBiteEngagement = 0.22f;        // [-]
        LaunchLockRPMDelta = 1500.0f;        // [rev/min above idle]
        LaunchSpeedThreshold_ms = 5.0f;      // [m/s]
        LaunchEngagementRate = 18.0f;        // [1/s]
        CruiseEngagementRate = 25.0f;        // [1/s]
        
        ConfigureClutch(PresetID);
    }

    EClutchControl ControlType;
    float MaxTorqueCapacity;                 // [N·m] - Slip threshold torque
    float EngagementTime;                    // [s] - 0→1 transition duration
    float DisengagementTime;                 // [s] - 1→0 transition duration

    float BreakawaySlipRPM;                  // [rev/min] - Slip where dynamic clutch torque nears capacity
    float LockSlipRPM;                       // [rev/min] - Slip window for rigid lockup
    float LaunchBiteEngagement;              // [-] - Minimum bite engagement for a throttle launch
    float LaunchLockRPMDelta;                // [rev/min] - RPM above idle where launch target reaches full lock
    float LaunchSpeedThreshold_ms;           // [m/s] - Below this speed, use launch/creep engagement
    float LaunchEngagementRate;              // [1/s] - Hydraulic closing rate during launch
    float CruiseEngagementRate;              // [1/s] - Normal non-shift closing rate

    /* ================================================= CLUTCH FUNCTIONS ================================================= */
    /** GT-R clutch configuration */
    void ConfigureClutch(int32 InPresetID)
    {
        if (InPresetID == 1) // Reason: preset 1 configuration check
        {
            ControlType = EClutchControl::DualClutch;
            MaxTorqueCapacity = 1500.0f;     // [N·m]
            EngagementTime = 0.02f;          // [s]
            DisengagementTime = 0.02f;       // [s]
            BreakawaySlipRPM = 300.0f;       // [rev/min]
            LockSlipRPM = 50.0f;             // [rev/min]
            LaunchBiteEngagement = 0.22f;    // [-]
            LaunchLockRPMDelta = 1500.0f;    // [rev/min above idle]
            LaunchSpeedThreshold_ms = 5.0f;  // [m/s]
            LaunchEngagementRate = 18.0f;    // [1/s]
            CruiseEngagementRate = 25.0f;    // [1/s]
            
            TraceConfiguration();
        } // End if (preset 1 configuration)
    }

    static FORCEINLINE float Smooth01(float X)
    {
        const float T = FMath::Clamp(X, 0.0f, 1.0f);
        return T * T * (3.0f - (2.0f * T));
    }

    FORCEINLINE bool IsLaunchWindow(float CurrentGearRatioAbs, float Throttle, float VehicleSpeed_ms) const
    {
        return CurrentGearRatioAbs > 0.001f && Throttle > 0.01f && FMath::Abs(VehicleSpeed_ms) < LaunchSpeedThreshold_ms;
    }

    FORCEINLINE float CalculateTargetEngagement(
        float CurrentGearRatioAbs,
        bool bIsShifting,
        float ShiftClutchPosition,
        float EngineRPM,
        float IdleRPM,
        float Throttle,
        float Brake,
        float VehicleSpeed_ms) const
    {
        const float ThrottleClamped = FMath::Clamp(Throttle, 0.0f, 1.0f);
        const float SpeedAbs = FMath::Abs(VehicleSpeed_ms);

        if (bIsShifting)
        {
            return FMath::Clamp(ShiftClutchPosition, 0.0f, 1.0f);
        }

        if (CurrentGearRatioAbs < 0.001f)
        {
            return 0.0f;
        }

        if (IsLaunchWindow(CurrentGearRatioAbs, ThrottleClamped, VehicleSpeed_ms))
        {
            const float RpmAlpha = Smooth01((EngineRPM - IdleRPM) / FMath::Max(LaunchLockRPMDelta, 1.0f));
            const float ThrottleAlpha = Smooth01((ThrottleClamped - 0.02f) / 0.30f);
            const float LaunchTarget = FMath::Lerp(LaunchBiteEngagement, 1.0f, RpmAlpha);
            return FMath::Clamp(LaunchTarget * ThrottleAlpha, 0.0f, 1.0f);
        }

        if (Brake > 0.1f && SpeedAbs < LaunchSpeedThreshold_ms && ThrottleClamped < 0.05f)
        {
            return 0.0f;
        }

        if (EngineRPM < IdleRPM + 200.0f && ThrottleClamped < 0.05f)
        {
            return 0.0f;
        }

        return 1.0f;
    }

    FORCEINLINE float GetEngagementInterpRate(float CurrentEngagement, float TargetEngagement, bool bLaunchWindow, bool bIsShifting) const
    {
        if (TargetEngagement < CurrentEngagement)
        {
            return 1.0f / FMath::Max(DisengagementTime, 0.005f);
        }

        if (bLaunchWindow)
        {
            return LaunchEngagementRate;
        }

        if (bIsShifting)
        {
            return 1.0f / FMath::Max(EngagementTime, 0.005f);
        }

        return CruiseEngagementRate;
    }

    FORCEINLINE float CalculateTorqueTransfer(float Engagement, float SlipOmegaRadS, float& OutCapacityNm, bool& bOutLocked) const
    {
        const float EngagementClamped = FMath::Clamp(Engagement, 0.0f, 1.0f);
        OutCapacityNm = MaxTorqueCapacity * EngagementClamped;
        const float SlipRPM = FMath::Abs(SlipOmegaRadS) * (60.0f / (2.0f * PI));
        bOutLocked = EngagementClamped > 0.98f && SlipRPM < LockSlipRPM;

        if (OutCapacityNm <= UE_SMALL_NUMBER)
        {
            return 0.0f;
        }

        const float BreakawayRadS = FMath::Max(BreakawaySlipRPM * (2.0f * PI / 60.0f), 1.0f);
        const float Denom = FMath::Sqrt((SlipOmegaRadS * SlipOmegaRadS) + (BreakawayRadS * BreakawayRadS));
        const float Saturation = (Denom > UE_SMALL_NUMBER) ? (SlipOmegaRadS / Denom) : 0.0f;
        return OutCapacityNm * Saturation;
    }

    /** Calculate slip ratio */
    float CalculateSlipRatio(float EngineRPM, float TransmissionRPM) const
    {
        if (EngineRPM < 0.01f) return 0.0f; // Reason: prevent division by zero
        return FMath::Abs(EngineRPM - TransmissionRPM) / EngineRPM; // [-] dimensionless
    }

    /* ================================================= DEBUGGING ================================================= */
    void TraceConfiguration() const
    {
        UE_LOG(LogTemp, Log, TEXT("--- Clutch Configuration ---"));
        UE_LOG(LogTemp, Log, TEXT("Control Type: %d | Max Torque: %.1f N·m"), (int32)ControlType, MaxTorqueCapacity);
        UE_LOG(LogTemp, Log, TEXT("Engagement: %.3f s | Disengagement: %.3f s"), EngagementTime, DisengagementTime);
        UE_LOG(LogTemp, Log, TEXT("---------------------------"));
    }
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                            ⚡ Clutch Runtime State (Mutable, Physics Thread)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FClutchStateVector
{
    float ClutchPosition;                   // [-] - Pedal input or auto target (0-1)
    float ClutchEngagement;                 // [-] - Physical plate engagement (0-1)
    float LockupRatio;                      // [-] - Slip coefficient (0=open, 1=locked)
    float TorqueTransferred;                // [N·m] - Actual transmitted torque
    float SlipRPM;                          // [rev·min⁻¹] - RPM difference across clutch
    
    FClutchStateVector() : ClutchPosition(1.0f), ClutchEngagement(1.0f), LockupRatio(1.0f), TorqueTransferred(0.0f), SlipRPM(0.0f) {}
};
