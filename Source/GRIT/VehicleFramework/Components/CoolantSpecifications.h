#pragma once

#include "CoreMinimal.h"

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                              ❄️ THERMAL MANAGEMENT SYSTEM
//----------------------------------------------------------------------------------------------------------------------------------------

/** Coolant medium types */
UENUM()
enum class ECoolantMedium : uint8
{
    Air_Ambient,           // [-] - Passive air cooling (fan-assisted)
    Chemical_MEG,          // [-] - Monoethylene Glycol Solution (50/50 water mix)
    Chemical_PG,           // [-] - Propylene Glycol Heat Transfer Fluid
    Chemical_EG,           // [-] - Ethane-1,2-diol Solution (pure ethylene glycol)
    Chemical_IGC,          // [-] - Inhibited Glycol Concentrate
    Liquid_Water,          // [-] - Deionized water circuit (PC-style)
    Cryogenic_LN2          // [-] - Liquid Nitrogen (extreme performance)
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                  🧊 Coolant Medium Properties (Immutable)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FCoolantMediumProperties
{
    ECoolantMedium MediumType;                      // [-] - Coolant type identifier
    float SpecificHeat_J_kgK;                       // [J·kg⁻¹·K⁻¹] - Specific heat capacity
    float ThermalConductivity_W_mK;                 // [W·m⁻¹·K⁻¹] - Thermal conductivity
    float Density_kg_m3;                            // [kg·m⁻³] - Mass density at 20°C
    float BoilingPoint_K;                           // [K] - Boiling point at 1 atm
    float FreezingPoint_K;                          // [K] - Freezing point at 1 atm
    float HeatTransferCoeff_W_m2K;                  // [W·m⁻²·K⁻¹] - Convective heat transfer coefficient
    float PowerDraw_W;                              // [W] - Electrical power required for pumping/compression
    float MaxFlowRate_L_min;                        // [L·min⁻¹] - Maximum volumetric flow rate
    float DepletionRate_kg_s;                       // [kg·s⁻¹] - Mass depletion rate (for consumables like LN2)
    
    // Note: ReservoirCapacity_kg removed. Capacity is now calculated from Vehicle Volume (L) * Density.
    
    FCoolantMediumProperties()
        : MediumType(ECoolantMedium::Chemical_MEG)
        , SpecificHeat_J_kgK(3500.0f)
        , ThermalConductivity_W_mK(0.4f)
        , Density_kg_m3(1070.0f)
        , BoilingPoint_K(380.0f)
        , FreezingPoint_K(235.0f)
        , HeatTransferCoeff_W_m2K(5000.0f)
        , PowerDraw_W(150.0f)
        , MaxFlowRate_L_min(80.0f)
        , DepletionRate_kg_s(0.0f)
    {}

    /** Configure properties based on coolant type */
    void ConfigureMedium(ECoolantMedium InMediumType)
    {
        MediumType = InMediumType;
        
        switch (MediumType)
        {
            case ECoolantMedium::Air_Ambient:
                SpecificHeat_J_kgK = 1005.0f;           
                ThermalConductivity_W_mK = 0.026f;      
                Density_kg_m3 = 1.2f;                   
                BoilingPoint_K = 0.0f;                  
                FreezingPoint_K = 0.0f;                 
                HeatTransferCoeff_W_m2K = 25.0f;        
                PowerDraw_W = 300.0f;                   
                MaxFlowRate_L_min = 0.0f;               
                DepletionRate_kg_s = 0.0f;              
                break;
                
            case ECoolantMedium::Chemical_MEG:
                SpecificHeat_J_kgK = 3500.0f;           
                ThermalConductivity_W_mK = 0.4f;        
                Density_kg_m3 = 1070.0f;                
                BoilingPoint_K = 380.0f;                
                FreezingPoint_K = 235.0f;               
                HeatTransferCoeff_W_m2K = 5000.0f;      
                PowerDraw_W = 150.0f;                   
                MaxFlowRate_L_min = 80.0f;              
                DepletionRate_kg_s = 0.0f;              
                break;
                
            case ECoolantMedium::Chemical_PG:
                SpecificHeat_J_kgK = 3800.0f;           
                ThermalConductivity_W_mK = 0.35f;       
                Density_kg_m3 = 1040.0f;                
                BoilingPoint_K = 376.0f;                
                FreezingPoint_K = 240.0f;               
                HeatTransferCoeff_W_m2K = 4800.0f;      
                PowerDraw_W = 160.0f;                   
                MaxFlowRate_L_min = 75.0f;              
                DepletionRate_kg_s = 0.0f;              
                break;
                
            case ECoolantMedium::Chemical_EG:
                SpecificHeat_J_kgK = 2400.0f;           
                ThermalConductivity_W_mK = 0.25f;       
                Density_kg_m3 = 1113.0f;                
                BoilingPoint_K = 470.0f;                
                FreezingPoint_K = 260.0f;               
                HeatTransferCoeff_W_m2K = 4500.0f;      
                PowerDraw_W = 180.0f;                   
                MaxFlowRate_L_min = 65.0f;              
                DepletionRate_kg_s = 0.0f;              
                break;
                
            case ECoolantMedium::Chemical_IGC:
                SpecificHeat_J_kgK = 3600.0f;           
                ThermalConductivity_W_mK = 0.42f;       
                Density_kg_m3 = 1080.0f;                
                BoilingPoint_K = 385.0f;                
                FreezingPoint_K = 233.0f;               
                HeatTransferCoeff_W_m2K = 5200.0f;      
                PowerDraw_W = 145.0f;                   
                MaxFlowRate_L_min = 85.0f;              
                DepletionRate_kg_s = 0.0f;              
                break;
                
            case ECoolantMedium::Liquid_Water:
                SpecificHeat_J_kgK = 4186.0f;           
                ThermalConductivity_W_mK = 0.6f;        
                Density_kg_m3 = 1000.0f;                
                BoilingPoint_K = 373.0f;                
                FreezingPoint_K = 273.0f;               
                HeatTransferCoeff_W_m2K = 6000.0f;      
                PowerDraw_W = 120.0f;                   
                MaxFlowRate_L_min = 100.0f;             
                DepletionRate_kg_s = 0.0f;              
                break;
                
            case ECoolantMedium::Cryogenic_LN2:
                SpecificHeat_J_kgK = 2042.0f;           
                ThermalConductivity_W_mK = 0.14f;       
                Density_kg_m3 = 808.0f;                 
                BoilingPoint_K = 77.0f;                 
                FreezingPoint_K = 63.0f;                
                HeatTransferCoeff_W_m2K = 25000.0f;     
                PowerDraw_W = 2500.0f;                  
                MaxFlowRate_L_min = 40.0f;              
                DepletionRate_kg_s = 0.08f;             
                break;
                
            default:
                ConfigureMedium(ECoolantMedium::Chemical_MEG); 
                break;
        }
    }
    
    float GetEffectivenessMultiplier() const
    {
        switch (MediumType)
        {
            case ECoolantMedium::Air_Ambient:       return 1.0f;   
            case ECoolantMedium::Chemical_MEG:      return 12.0f;  
            case ECoolantMedium::Chemical_PG:       return 11.5f;  
            case ECoolantMedium::Chemical_EG:       return 10.5f;  
            case ECoolantMedium::Chemical_IGC:      return 13.0f;  
            case ECoolantMedium::Liquid_Water:      return 15.0f;  
            case ECoolantMedium::Cryogenic_LN2:     return 80.0f;  
            default:                                return 1.0f;
        }
    }
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                  🌡️ Thermal Management Specifications (Immutable)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FCoolantSpecifications
{
    FCoolantSpecifications(int32 PresetID = 1)
    {
        InitialCoolantVolume_L = 10.0f; // Initialize default
        ConfigureSystem(PresetID);
    }

    /* ================================================= SYSTEM IDENTIFICATION ================================================= */
    
    FName SystemID;                                 // [-] - Thermal system identifier
    
    /* ================================================= COOLANT CONFIGURATION ================================================= */
    
    FCoolantMediumProperties ActiveCoolant;         // [-] - Current coolant medium properties
    
    // [UPDATED] Volume Capacity for the Vehicle
    float InitialCoolantVolume_L;                   // [L] - Total coolant capacity in Liters
    
    /* ================================================= RADIATOR SPECIFICATIONS ================================================= */
    
    float RadiatorArea_m2;                          // [m²] - Effective heat exchange area
    float RadiatorEfficiency;                       // [-] - Heat transfer effectiveness (0-1)
    float FanCFM;                                   // [ft³·min⁻¹] - Fan airflow rating (at max RPM)
    float FanPower_W;                               // [W] - Electric fan power draw
    
    /* ================================================= PUMP SPECIFICATIONS ================================================= */
    
    float PumpPower_W;                              // [W] - Coolant pump electrical load
    float PumpFlowRate_L_min;                       // [L·min⁻¹] - Pump volumetric flow rate
    
    /* ================================================= THERMOSTAT CONTROL ================================================= */
    
    float ThermostatOpen_K;                         // [K] - Thermostat opens (coolant to radiator)
    float ThermostatFull_K;                         // [K] - Thermostat fully open
    
    /* ================================================= THERMAL LIMITS ================================================= */
    
    float OverheatThreshold_K;                      // [K] - Performance degradation begins
    float CriticalThreshold_K;                      // [K] - Damage threshold
    
    /* ================================================= CONFIGURATION ================================================= */
    
    void ConfigureSystem(int32 InPresetID)
    {
        if (InPresetID == 1) // GT-R Nismo thermal system
        {
            SystemID = TEXT("GTR_ThermalMgmt");
            
            // Default to high-performance inhibited glycol
            ActiveCoolant.ConfigureMedium(ECoolantMedium::Chemical_IGC);
            InitialCoolantVolume_L = 10.0f;         // [L] - 10 Liter Capacity
            
            // GT-R Nismo radiator (upgraded dual-core)
            RadiatorArea_m2 = 2.8f;                 // [m²] - Large frontal area
            RadiatorEfficiency = 0.85f;             // [-] - High-efficiency core
            FanCFM = 4500.0f;                       // [ft³·min⁻¹] - High-flow electric fans
            FanPower_W = 300.0f;                    // [W] - Twin fan power
            
            // High-flow pump
            PumpPower_W = 150.0f;                   // [W] - Electric water pump
            PumpFlowRate_L_min = 85.0f;             // [L·min⁻¹] - Racing flow rate
            
            // Thermostat calibration
            ThermostatOpen_K = 358.15f;             // [K] - 85°C opening
            ThermostatFull_K = 368.15f;             // [K] - 95°C fully open
            
            // Thermal limits (VR38DETT)
            OverheatThreshold_K = 393.15f;          // [K] - 120°C warning
            CriticalThreshold_K = 403.15f;          // [K] - 130°C danger
            
            TraceConfiguration();
        }
    }
    
    void SetCoolantMedium(ECoolantMedium NewMedium)
    {
        ActiveCoolant.ConfigureMedium(NewMedium);
        UE_LOG(LogTemp, Log, TEXT("Coolant Medium Changed: %d"), static_cast<int32>(NewMedium));
    }
    
    void TraceConfiguration() const
    {
        UE_LOG(LogTemp, Log, TEXT("--- Thermal Management Configuration ---"));
        UE_LOG(LogTemp, Log, TEXT("SystemID: %s | Coolant: %d"), *SystemID.ToString(), static_cast<int32>(ActiveCoolant.MediumType));
        UE_LOG(LogTemp, Log, TEXT("Capacity: %.1f L"), InitialCoolantVolume_L);
        UE_LOG(LogTemp, Log, TEXT("Radiator: %.2f m² | Efficiency: %.2f | Fan: %.0f CFM @ %.0f W"), RadiatorArea_m2, RadiatorEfficiency, FanCFM, FanPower_W);
        UE_LOG(LogTemp, Log, TEXT("Pump: %.0f L/min @ %.0f W"), PumpFlowRate_L_min, PumpPower_W);
        UE_LOG(LogTemp, Log, TEXT("Thermostat: %.1f°C - %.1f°C"), ThermostatOpen_K - 273.15f, ThermostatFull_K - 273.15f);
        UE_LOG(LogTemp, Log, TEXT("Coolant Props: Cp=%.0f J/kg·K | λ=%.3f W/m·K | ρ=%.0f kg/m³"), ActiveCoolant.SpecificHeat_J_kgK, ActiveCoolant.ThermalConductivity_W_mK, ActiveCoolant.Density_kg_m3);
        UE_LOG(LogTemp, Log, TEXT("Effectiveness: %.1fx | Power: %.0f W | Depletion: %.3f kg/s"), ActiveCoolant.GetEffectivenessMultiplier(), ActiveCoolant.PowerDraw_W, ActiveCoolant.DepletionRate_kg_s);
        UE_LOG(LogTemp, Log, TEXT("----------------------------------------"));
    }
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                              ❄️ Thermal Management Runtime State
//----------------------------------------------------------------------------------------------------------------------------------------

struct FCoolantStateVector
{
    FCoolantStateVector()
        : CoolantTemp_K(293.15f)                    // [K] - Coolant temperature (20°C)
        , CoolantFlowRate_L_min(0.0f)               // [L·min⁻¹] - Current flow rate
        , CoolantMass_kg(0.0f)                      // [kg] - Current coolant mass
        , ThermostatPosition(0.0f)                  // [-] - Thermostat valve position (0=closed, 1=open)
        , FanSpeed(0.0f)                            // [-] - Fan speed fraction (0-1)
        , RadiatorHeatRemoval_W(0.0f)               // [W] - Heat removed by radiator
        , AmbientTemp_K(293.15f)                    // [K] - Outside air temperature
        , VehicleSpeed_ms(0.0f)                     // [m·s⁻¹] - Vehicle speed (for ram air cooling)
        , TotalPowerDraw_W(0.0f)                    // [W] - Total electrical load
        , bSystemActive(false)                      // [-] - System operational state
        , RuntimeRemaining_s(0.0f)                  // [s] - Remaining runtime (for consumables like LN2)
        , bDepleted(false)                          // [-] - Coolant fully depleted flag
    {}

    float CoolantTemp_K;                            // [K] - Current coolant temperature
    float CoolantFlowRate_L_min;                    // [L·min⁻¹] - Actual flow rate
    float CoolantMass_kg;                           // [kg] - Remaining coolant mass
    float ThermostatPosition;                       // [-] - Valve position (0-1)
    float FanSpeed;                                 // [-] - Fan duty cycle (0-1)
    float RadiatorHeatRemoval_W;                    // [W] - Instantaneous heat removal
    float AmbientTemp_K;                            // [K] - Ambient air temperature
    float VehicleSpeed_ms;                          // [m·s⁻¹] - Current vehicle speed
    float TotalPowerDraw_W;                         // [W] - Current electrical consumption
    bool bSystemActive;                             // [-] - Pump/fan operational
    float RuntimeRemaining_s;                       // [s] - Time until coolant depleted (LN2)
    bool bDepleted;                                 // [-] - True when coolant mass reaches zero

    void UpdateThermostat(float OpenTemp_K, float FullTemp_K)
    {
        if (CoolantTemp_K < OpenTemp_K)
        {
            ThermostatPosition = 0.0f;              // [-] - Closed
        }
        else if (CoolantTemp_K >= FullTemp_K)
        {
            ThermostatPosition = 1.0f;              // [-] - Fully open
        }
        else
        {
            ThermostatPosition = (CoolantTemp_K - OpenTemp_K) / (FullTemp_K - OpenTemp_K);
        }
    }
    
    void UpdateFanSpeed(float OverheatTemp_K)
    {
        if (!bSystemActive || ThermostatPosition < 0.1f)
        {
            FanSpeed = 0.0f;
            return;
        }
        
        const float TempRange_K = OverheatTemp_K - 373.15f;
        const float TempExcess_K = FMath::Max(0.0f, CoolantTemp_K - 373.15f);
        FanSpeed = FMath::Clamp(TempExcess_K / TempRange_K, 0.0f, 1.0f);
    }
};