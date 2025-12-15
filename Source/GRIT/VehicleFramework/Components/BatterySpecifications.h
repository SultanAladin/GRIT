#pragma once

#include "CoreMinimal.h"

/*====================================================================================================================================
                                                    🔋 BATTERY SYSTEM
======================================================================================================================================*/

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                         📋 ENUMERATIONS
//----------------------------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EBatteryChemistry : uint8
{
    LithiumIon,
    LithiumPolymer,
    SolidState
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                              🧩 Battery Specifications (Immutable)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FBatterySpecifications
{
    FBatterySpecifications(int32 PresetID = 1)
        : Chemistry(EBatteryChemistry::LithiumIon)
        , Capacity_kWh(90.0f)                    // [kW·h]
        , Capacity_J(3.24e8f)                    // [J]
        , Voltage(400.0f)                        // [V]
        , MaxDischargePower(400000.0f)           // [W]
        , MaxChargePower(200000.0f)              // [W]
        , InternalResistance(0.05f)              // [Ω]
        , ThermalMass(50000.0f)                  // [J·K⁻¹]
        , PassiveCoolingFactor(15.0f)            // [W·K⁻¹]
    {
        ConfigureBattery(PresetID);
    }

    /* ================================================= BATTERY PROPERTIES ================================================= */
    
    EBatteryChemistry Chemistry;                 // [-]
    float Capacity_kWh;                          // [kW·h]
    float Capacity_J;                            // [J]
    float Voltage;                               // [V]
    float MaxDischargePower;                     // [W]
    float MaxChargePower;                        // [W]
    float InternalResistance;                    // [Ω]
    float ThermalMass;                           // [J·K⁻¹]
    float PassiveCoolingFactor;                  // [W·K⁻¹]

    /* ================================================= CONFIGURATION ================================================= */
    
    void ConfigureBattery(int32 InPresetID)
    {
        if (InPresetID == 1)
        {
            Chemistry = EBatteryChemistry::LithiumIon;
            Capacity_kWh = 90.0f;                // [kW·h]
            Capacity_J = Capacity_kWh * 3600000.0f; // [J]
            Voltage = 400.0f;                    // [V]
            MaxDischargePower = 400000.0f;       // [W]
            MaxChargePower = 200000.0f;          // [W]
            InternalResistance = 0.05f;          // [Ω]
            ThermalMass = 50000.0f;              // [J·K⁻¹]
            PassiveCoolingFactor = 15.0f;        // [W·K⁻¹]
            
            TraceConfiguration();
        }
    }

    /* ================================================= CALCULATIONS ================================================= */
    
    FORCEINLINE float CalculateSOC(float CurrentCharge_J) const
    {
        return (Capacity_J <= 0.0f) ? 0.0f : FMath::Clamp(CurrentCharge_J / Capacity_J, 0.0f, 1.0f); // [0-1]
    }

    /* ================================================= DEBUG ================================================= */
    
    void TraceConfiguration() const
    {
        UE_LOG(LogTemp, Log, TEXT("--- Battery Configuration ---"));
        UE_LOG(LogTemp, Log, TEXT("Chemistry: %d | Capacity: %.1f kW·h | Voltage: %.1f V"), (int32)Chemistry, Capacity_kWh, Voltage);
        UE_LOG(LogTemp, Log, TEXT("Max Discharge: %.0f W | Max Charge: %.0f W"), MaxDischargePower, MaxChargePower);
        UE_LOG(LogTemp, Log, TEXT("----------------------------"));
    }
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                              ⚡ Battery Runtime State (Mutable, Physics Thread)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FBatteryStateVector
{
    FBatteryStateVector()
        : BatteryCharge_J(3.24e8f)               // [J]
        , BatteryChargePct(1.0f)                 // [0-1]
        , CurrentDraw(0.0f)                      // [A]
        , CellTemperature(293.15f)               // [K]
        , PowerDraw(0.0f)                        // [W]
        , VoltageActual(400.0f)                  // [V]
    {}

    float BatteryCharge_J;                       // [J]
    float BatteryChargePct;                      // [0-1]
    float CurrentDraw;                           // [A]
    float CellTemperature;                       // [K]
    float PowerDraw;                             // [W]
    float VoltageActual;                         // [V]
};