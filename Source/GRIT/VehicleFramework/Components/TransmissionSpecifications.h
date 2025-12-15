#pragma once

#include "CoreMinimal.h"

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                              ⚙️ TRANSMISSION SYSTEM
//----------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                              📋 ENUMERATIONS
//----------------------------------------------------------------------------------------------------------------------------------------

/** Transmission mechanical architecture */
UENUM(BlueprintType)
enum class ETransmissionType : uint8
{
    Manual,         // H-pattern + manual clutch
    Sequential,     // Sequential shifter
    Automatic,      // Torque converter AT
    DCT             // Dual-clutch system
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                  🧩 Transmission Specifications (Immutable)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FTransmissionSpecifications
{
    FTransmissionSpecifications(int32 PresetID = 1)
    {
        Type = ETransmissionType::Automatic;
        FinalDriveRatio = 3.55f;                    // [ratio]
        ShiftTime = 0.08f;                          // [s]
        bReverseAsBrake = true;                     // [-]
        
        ConfigureTransmission(PresetID);
    }

    /* ================================================= ARCHITECTURE ================================================= */
      ETransmissionType Type;                         // [-]

    /* ================================================= GEAR RATIOS ================================================= */
    
    TArray<float> GearRatios;                       // [ratio]
    float FinalDriveRatio;                          // [ratio]

    /* ================================================= TIMING ================================================= */
    
    float ShiftTime;                                // [s]

    /* ================================================= SHIFT MAP ================================================= */
    
    TArray<FVector2D> ShiftMap;                     // [rev·min⁻¹, rev·min⁻¹]

    /* ================================================= BEHAVIOR ================================================= */
    
    bool bReverseAsBrake;                           // [-]

    /* ================================================= CONFIGURATION ================================================= */
    
    void ConfigureTransmission(int32 InPresetID)
    {
        if (InPresetID == 1)
        {
            Type = ETransmissionType::DCT;
            
            GearRatios = {
                -3.8f,  // Reverse 1     [ratio]
                -2.0f,  // Reverse 2     [ratio]
                0.0f,   // Neutral       [ratio]
                4.2f,   // 1st           [ratio]
                3.1f,   // 2nd           [ratio]
                2.5f,   // 3rd           [ratio]
                1.9f,   // 4th           [ratio]
                1.5f,   // 5th           [ratio]
                1.2f    // 6th           [ratio]
            };
            
            FinalDriveRatio = 3.55f;                // [ratio]
            ShiftTime = 0.02f;                      // [s]
            bReverseAsBrake = true;                 // [-]
            
            ShiftMap = {
                FVector2D(4000.0f, 1500.0f),        // Reverse 1     [rev·min⁻¹]
                FVector2D(99999.0f, 1500.0f),       // Reverse 2     [rev·min⁻¹]
                FVector2D(0.0f, 0.0f),              // Neutral       [rev·min⁻¹]
                FVector2D(6500.0f, 2000.0f),        // 1st           [rev·min⁻¹]
                FVector2D(6800.0f, 2200.0f),        // 2nd           [rev·min⁻¹]
                FVector2D(7000.0f, 2500.0f),        // 3rd           [rev·min⁻¹]
                FVector2D(7000.0f, 2800.0f),        // 4th           [rev·min⁻¹]
                FVector2D(7000.0f, 3000.0f),        // 5th           [rev·min⁻¹]
                FVector2D(99999.0f, 3200.0f)        // 6th           [rev·min⁻¹]
            };
            
            TraceConfiguration();
        }
    }

    /* ================================================= CALCULATIONS ================================================= */
    
    float GetCombinedRatio(int32 Gear) const
    {
        if (Gear < 0 || Gear >= GearRatios.Num()) return 0.0f;
        return GearRatios[Gear] * FinalDriveRatio;  // [ratio]
    }

    FVector2D GetShiftPoint(int32 Gear) const
    {
        if (Gear < 0 || Gear >= ShiftMap.Num()) return FVector2D(99999.0f, 0.0f);
        return ShiftMap[Gear];                       // [rev·min⁻¹, rev·min⁻¹]
    }

    /* ================================================= DEBUGGING ================================================= */
    
    void TraceConfiguration() const
    {
        UE_LOG(LogTemp, Log, TEXT("--- Transmission Configuration ---"));
        UE_LOG(LogTemp, Log, TEXT("Type: %d | Gears: %d | Final Drive: %.2f"), (int32)Type, GearRatios.Num(), FinalDriveRatio);
        UE_LOG(LogTemp, Log, TEXT("Shift Time: %.3f s | Reverse as Brake: %s"), ShiftTime, bReverseAsBrake ? TEXT("Yes") : TEXT("No"));
        UE_LOG(LogTemp, Log, TEXT("----------------------------------"));
    }
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                              ⚡ Transmission Runtime State (Mutable, Physics Thread)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FTransmissionStateVector
{
    FTransmissionStateVector()
        : CurrentGear(2)                            // [index]
        , TargetGear(2)                             // [index]
        , CombinedGearRatio(0.0f)                   // [ratio]
        , ShiftTimer(0.0f)                          // [s]
        , bIsShifting(false)                        // [-]
        , PreSelectedGear(3)                        // [index]
        , CurrentUpshiftRPM(99999.0f)               // [rev·min⁻¹]
        , CurrentDownshiftRPM(0.0f)                 // [rev·min⁻¹]
        , PrevInputOmega_rad_s(0.0f)                // [rad·s⁻¹]
        , GearHysteresisTimer(0.0f)                 // [s]
    {}

    int32 CurrentGear;                              // [index]
    int32 TargetGear;                               // [index]
    float CombinedGearRatio;                        // [ratio]
    float ShiftTimer;                               // [s]
    bool bIsShifting;                               // [-]
    int32 PreSelectedGear;                          // [index]
    float CurrentUpshiftRPM;                        // [rev·min⁻¹]
    float CurrentDownshiftRPM;                      // [rev·min⁻¹]
    float PrevInputOmega_rad_s;                     // [rad·s⁻¹] - Previous input shaft speed
    float GearHysteresisTimer;                      // [s] - Auto-shift lockout timer
};