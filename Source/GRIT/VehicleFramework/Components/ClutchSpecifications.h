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
        
        ConfigureClutch(PresetID);
    }

    EClutchControl ControlType;
    float MaxTorqueCapacity;                 // [N·m] - Slip threshold torque
    float EngagementTime;                    // [s] - 0→1 transition duration
    float DisengagementTime;                 // [s] - 1→0 transition duration

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
            
            TraceConfiguration();
        } // End if (preset 1 configuration)
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