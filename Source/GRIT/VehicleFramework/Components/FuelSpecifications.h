#pragma once

#include "CoreMinimal.h"

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                              ⛽ FUEL SYSTEM
//----------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                  🧩 Fuel Specifications (Immutable)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FFuelSpecifications
{
    FFuelSpecifications(int32 PresetID = 1)
    {
        Density_kg_L = 0.745f;                      // [kg·L⁻¹]
        EnergyDensity_MJ_kg = 43.5f;                // [MJ·kg⁻¹]
        OctaneRating = 93.0f;                       // [-]
        EnergyDensity_J_kg = 43500000.0f;           // [J·kg⁻¹]
        
        ConfigureFuel(PresetID);
    }

    /* ================================================= PHYSICAL PROPERTIES ================================================= */
    
    float Density_kg_L;                             // [kg·L⁻¹]
    float EnergyDensity_MJ_kg;                      // [MJ·kg⁻¹]
    float OctaneRating;                             // [-]
    float EnergyDensity_J_kg;                       // [J·kg⁻¹]

    /* ================================================= CONFIGURATION ================================================= */
    
    void ConfigureFuel(int32 InPresetID)
    {
        if (InPresetID == 1)
        {
            Density_kg_L = 0.745f;                  // [kg·L⁻¹]
            EnergyDensity_MJ_kg = 43.5f;            // [MJ·kg⁻¹]
            OctaneRating = 93.0f;                   // [-]
            EnergyDensity_J_kg = EnergyDensity_MJ_kg * 1000000.0f; // [J·kg⁻¹]
            
            TraceConfiguration();
        }
    }

    /* ================================================= CONVERSIONS ================================================= */
    
    float VolToMass(float Liters) const
    {
        return Liters * Density_kg_L;               // [kg]
    }

    float MassToVol(float Mass_kg) const
    {
        if (Density_kg_L <= 0.0f) return 0.0f;
        return Mass_kg / Density_kg_L;              // [L]
    }

    /* ================================================= DEBUGGING ================================================= */
    
    void TraceConfiguration() const
    {
        UE_LOG(LogTemp, Log, TEXT("--- Fuel Specifications ---"));
        UE_LOG(LogTemp, Log, TEXT("Density: %.3f kg·L^-1 | Energy: %.1f MJ·kg^-1 | Octane: %.0f"), Density_kg_L, EnergyDensity_MJ_kg, OctaneRating);
        UE_LOG(LogTemp, Log, TEXT("---------------------------"));
    }
};