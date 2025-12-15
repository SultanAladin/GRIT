#pragma once

#include "CoreMinimal.h"

/*====================================================================================================================================
                                                     🛢️ ENGINE OIL SYSTEM
======================================================================================================================================*/

//----------------------------------------------------------------------------------------------------------------------------------------
//                                               🧩 Engine Oil Specifications (Immutable)
//----------------------------------------------------------------------------------------------------------------------------------------

/** Immutable configuration for the engine oil circuit (sump, pump, viscosity, temperature limits). */
struct FEngineOilSpecifications
{
    FEngineOilSpecifications(int32 PresetID = 1)
    {
        ConfigureOil(PresetID);
    }

    /* ================================================= IDENTIFICATION ================================================= */

    FName OilID;                                       // [-]   - Oil identification / grade label

    /* ================================================= GEOMETRY & CAPACITY ================================================= */

    float SumpCapacity_L;                              // [L]   - Usable sump capacity
    float Density_kg_m3;                               // [kg/m^3] - Oil density

    /* ================================================= THERMAL PROPERTIES ================================================= */

    float SpecificHeat_J_kgK;                          // [J/(kg*K)] - Specific heat capacity
    float OperatingTempMin_K;                          // [K]   - Lower end of normal operating range
    float OperatingTempMax_K;                          // [K]   - Upper end of normal operating range
    float DamageTemp_K;                                // [K]   - Onset of oil breakdown / damage

    /* ================================================= VISCOSITY CHARACTERISTICS ================================================= */

    float Viscosity_cSt_40C;                           // [cSt] - Kinematic viscosity at 40 DegC
    float Viscosity_cSt_100C;                          // [cSt] - Kinematic viscosity at 100 DegC

    /* ================================================= PUMP & PRESSURE SYSTEM ================================================= */

    float PumpFlowRate_L_min;                          // [L/min] - Nominal pump flow at cruise
    float PumpPower_W;                                 // [W]   - Electrical/mechanical load of oil pump
    float ReliefPressure_kPa;                          // [kPa] - Pressure relief valve cracking pressure
    float MinSafePressure_kPa;                         // [kPa] - Minimum safe oil pressure at operating temp
    float MaxSafePressure_kPa;                         // [kPa] - Diagnostic upper bound

    /* ================================================= CONFIGURATION ================================================= */

    void ConfigureOil(int32 InPresetID)
    {
        // Preset 1: High-performance 5W-40 synthetic, ~6.5L sump (VR38-style)
        if (InPresetID == 1)
        {
            OilID = TEXT("Nissan_5W40_Synthetic");

            SumpCapacity_L = 6.5f;                     // [L]
            Density_kg_m3  = 850.0f;                   // [kg/m^3]

            SpecificHeat_J_kgK = 2100.0f;              // [J/(kg*K)] - Typical hydrocarbon oil
            OperatingTempMin_K = 323.15f;              // [K] ~50 DegC
            OperatingTempMax_K = 393.15f;              // [K] ~120 DegC
            DamageTemp_K       = 423.15f;              // [K] ~150 DegC

            Viscosity_cSt_40C  = 90.0f;                // [cSt] - Representative 5W-40 behaviour
            Viscosity_cSt_100C = 14.0f;                // [cSt]

            PumpFlowRate_L_min = 60.0f;                // [L/min] - High-output pump
            PumpPower_W        = 800.0f;               // [W] - Mechanical equivalent load
            ReliefPressure_kPa = 700.0f;               // [kPa] - ~7 bar relief
            MinSafePressure_kPa = 150.0f;              // [kPa]
            MaxSafePressure_kPa = 900.0f;              // [kPa]

            TraceConfiguration();
        }
    }

    /* ================================================= DEBUGGING ================================================= */

    void TraceConfiguration() const
    {
        UE_LOG(LogTemp, Log, TEXT("--- Engine Oil Configuration ---"));
        UE_LOG(LogTemp, Log, TEXT("OilID: %s | Sump: %.1f L | Density: %.1f kg/m^3"), *OilID.ToString(), SumpCapacity_L, Density_kg_m3);
        UE_LOG(LogTemp, Log, TEXT("Viscosity: %.1f cSt @40C | %.1f cSt @100C"), Viscosity_cSt_40C, Viscosity_cSt_100C);
        UE_LOG(LogTemp, Log, TEXT("Pump: %.1f L/min @ %.0f W | Relief: %.0f kPa"), PumpFlowRate_L_min, PumpPower_W, ReliefPressure_kPa);
        UE_LOG(LogTemp, Log, TEXT("Temp Range: %.1f C - %.1f C | Damage: %.1f C"),
            OperatingTempMin_K - 273.15f,
            OperatingTempMax_K - 273.15f,
            DamageTemp_K - 273.15f);
        UE_LOG(LogTemp, Log, TEXT("------------------------------------------------"));
    }
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                               ⚡ Engine Oil Runtime State (Mutable, Physics Thread)
//----------------------------------------------------------------------------------------------------------------------------------------

/** Runtime state of the engine oil circuit (temperature, pressure, level). */
struct FEngineOilStateVector
{
    FEngineOilStateVector()
        : OilTemp_K(293.15f)                          // [K]   - Bulk sump / gallery oil temperature
        , OilPressure_kPa(0.0f)                       // [kPa] - Current gallery pressure
        , SumpLevel_L(0.0f)                           // [L]   - Current oil level estimate
        , Viscosity_cSt(0.0f)                         // [cSt] - Effective viscosity at current temp
        , PumpFlowRate_L_min(0.0f)                    // [L/min] - Instantaneous flow
        , TotalPowerDraw_W(0.0f)                      // [W]   - Pump power draw
        , bLowPressureWarning(false)                  // [-]   - Below MinSafePressure_kPa
    {}

    float OilTemp_K;                                  // [K] - Bulk sump / gallery oil temperature
    float OilPressure_kPa;                            // [kPa]
    float SumpLevel_L;                                // [L]
    float Viscosity_cSt;                              // [cSt]
    float PumpFlowRate_L_min;                         // [L/min]
    float TotalPowerDraw_W;                           // [W]
    bool  bLowPressureWarning;                        // [-]
};