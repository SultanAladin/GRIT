#pragma once

#include "CoreMinimal.h"
#include "FuelSpecifications.h"

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                              ⛽ FUEL CONTROL SYSTEM
//----------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                  🧩 Fuel Control Specifications (Immutable)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FFuelControlSystem
{
    FFuelControlSystem(int32 PresetID = 1)
    {
        TankCapacityLiters = 70.0f;                 // [L]
        FuelWarningLevel = 0.15f;                   // [-]
        FuelCriticalLevel = 0.05f;                  // [-]
        
        ConfigureSystem(PresetID);
    }

    /* ================================================= CAPACITY ================================================= */
    
    float TankCapacityLiters;                       // [L]
    FFuelSpecifications FuelType;                   // [-]

    /* ================================================= WARNING LEVELS ================================================= */
    
    float FuelWarningLevel;                         // [-]
    float FuelCriticalLevel;                        // [-]

    /* ================================================= CONFIGURATION ================================================= */
    
    void ConfigureSystem(int32 InPresetID)
    {
        if (InPresetID == 1)
        {
            TankCapacityLiters = 74.0f;             // [L]
            FuelWarningLevel = 0.15f;               // [-]
            FuelCriticalLevel = 0.05f;              // [-]
            FuelType = FFuelSpecifications(InPresetID);
            
            TraceConfiguration();
        }
        else if (InPresetID == 2)
        {
            TankCapacityLiters = 50.0f;             // [L]
            FuelWarningLevel = 0.15f;               // [-]
            FuelCriticalLevel = 0.05f;              // [-]
            FuelType = FFuelSpecifications(InPresetID);
            
            TraceConfiguration();
        }
    }

    /* ================================================= CALCULATIONS ================================================= */
    
    float GetFuelMass(float CurrentLiters) const
    {
        return FuelType.VolToMass(CurrentLiters);   // [kg]
    }

    float GetFuelLevelPct(float CurrentLiters) const
    {
        if (TankCapacityLiters <= 0.0f) return 0.0f;
        return FMath::Clamp(CurrentLiters / TankCapacityLiters, 0.0f, 1.0f); // [-]
    }

    bool IsFuelWarning(float CurrentLiters) const
    {
        return GetFuelLevelPct(CurrentLiters) <= FuelWarningLevel;
    }

    bool IsFuelCritical(float CurrentLiters) const
    {
        return GetFuelLevelPct(CurrentLiters) <= FuelCriticalLevel;
    }

    float EstimateRange(float CurrentLiters, float CurrentConsumptionRate) const
    {
        if (CurrentConsumptionRate <= 0.0f) return 999999.0f;
        
        float CurrentMass = GetFuelMass(CurrentLiters); // [kg]
        float TimeRemaining = CurrentMass / CurrentConsumptionRate; // [s]
        
        return TimeRemaining;                       // [s]
    }

    /* ================================================= DEBUGGING ================================================= */
    
    void TraceConfiguration() const
    {
        UE_LOG(LogTemp, Log, TEXT("--- Fuel Control System ---"));
        UE_LOG(LogTemp, Log, TEXT("Capacity: %.1f L | Full Mass: %.2f kg"), 
            TankCapacityLiters, GetFuelMass(TankCapacityLiters));
        UE_LOG(LogTemp, Log, TEXT("Warning: %.0f%% | Critical: %.0f%%"), 
            FuelWarningLevel * 100.0f, FuelCriticalLevel * 100.0f);
        UE_LOG(LogTemp, Log, TEXT("---------------------------"));
    }
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                              ⚡ Fuel System Runtime State (Mutable, Physics Thread)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FFuelSystemStateVector
{
    FFuelSystemStateVector()
        : CurrentFuelLiters(70.0f)                  // [L]
        , CurrentFuelMass_kg(52.15f)                // [kg]
        , FuelLevelPct(1.0f)                        // [-]
        , ConsumptionRate(0.0f)                     // [kg·s⁻¹]
        , EstimatedRange(0.0f)                      // [s]
        , bFuelWarning(false)                       // [-]
        , bFuelCritical(false)                      // [-]
        , bFuelEmpty(false)                         // [-]
    {}

    float CurrentFuelLiters;                        // [L]
    float CurrentFuelMass_kg;                       // [kg]
    float FuelLevelPct;                             // [-]
    float ConsumptionRate;                          // [kg·s⁻¹]
    float EstimatedRange;                           // [s]
    bool bFuelWarning;                              // [-]
    bool bFuelCritical;                             // [-]
    bool bFuelEmpty;                                // [-]

    /* ================================================= STATE UPDATE ================================================= */
    
    void UpdateFuel(float MassConsumed_kg, const FFuelSpecifications& FuelSpec, const FFuelControlSystem& FuelControl)
    {
        CurrentFuelMass_kg = FMath::Max(0.0f, CurrentFuelMass_kg - MassConsumed_kg); // [kg]
        CurrentFuelLiters = FuelSpec.MassToVol(CurrentFuelMass_kg); // [L]
        FuelLevelPct = FuelControl.GetFuelLevelPct(CurrentFuelLiters); // [-]
        EstimatedRange = FuelControl.EstimateRange(CurrentFuelLiters, ConsumptionRate); // [s]
        
        bFuelEmpty = CurrentFuelMass_kg <= 0.0f;
        bFuelWarning = FuelControl.IsFuelWarning(CurrentFuelLiters);
        bFuelCritical = FuelControl.IsFuelCritical(CurrentFuelLiters);
    }
};