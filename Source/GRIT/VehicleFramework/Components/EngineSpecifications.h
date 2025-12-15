#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                              🔥 ENGINE SYSTEM
//----------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                  🧩 Engine Specifications (Immutable)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FEngineSpecifications
{

    FEngineSpecifications(int32 PresetID = 1)
    {
        EngineID = TEXT("DefaultEngine");
        ConfigureEngine(PresetID);
    }


    /* ================================================= CORE IDENTIFIERS ================================================= */
    
    FName EngineID;                                     // [-] - Engine identification
    
    /* ================================================= ENGINE CORE PROPERTIES ================================================= */
    
    float MaxTorque_Nm;                                 // [N·m] - Peak torque at 5800 RPM
    float IdleRPM;                                      // [rev·min⁻¹] - Idle speed
    float RedlineRPM;                                   // [rev·min⁻¹] - Maximum safe RPM
    float MaxRPMIncreasePerSec;                         // [rev·min⁻¹·s⁻¹] - Max allowed RPM rise rate
    float MaxRPMDecreasePerSec;                         // [rev·min⁻¹·s⁻¹] - Max allowed RPM fall rate
    float EngineBrakingCoeff;                           // [N·m·s·rad⁻¹] - Mechanical friction losses
    float EngineBrakingQuadratic;                       // [N·m·s²·rad⁻²] - Pumping losses (vacuum dependent)
    float EngineInertia;                                // [kg·m²] - Rotational inertia (crankshaft + flywheel + reciprocating mass)
    float InvEngineInertia;                             // [kg⁻¹·m⁻²] - Inverse inertia (1 / 0.35)
    
    /* ================================================= THERMAL PROPERTIES ================================================= */
    
    float AmbientTemperature_K;                         // [K] - Ambient air (20°C)
    float EngineThermalMass;                            // [J·K⁻¹] - Engine block + coolant thermal mass
    float HeatInefficiencyFactor;                       // [-] - Inefficient power to heat ratio (2.0 = 1/3 useful, 2/3 waste)
    float PassiveCoolingFactor;                         // [W·K⁻¹] - Passive heat dissipation per Kelvin difference
    float ThermostatOpenTemp_K;                         // [K] - Thermostat begins opening (85°C)
    float ThermostatFullOpenTemp_K;                     // [K] - Thermostat fully open (95°C)
    float BaseFanCoolingFactor;                         // [W·K⁻¹] - Base radiator cooling at 0 speed
    float SpeedCoolingFactor;                           // [W·K⁻¹·m⁻¹·s] - Additional cooling per m/s vehicle speed
    float OverheatTemp_K;                               // [K] - Overheating effects begin (120°C)
    float DamageTemp_K;                                 // [K] - Engine damage threshold (130°C)
    
    /* ================================================= FUEL PROPERTIES ================================================= */
    
    float BaseThermalEfficiency;                        // [0-1] - Peak thermal efficiency at cruise
    float BoostedThermalEfficiency;                     // [0-1] - Thermal efficiency under boost (rich mixture)


    /* ========================== TORQUE CURVE ========================== */
    TArray<FVector2D> TorqueCurve = {
        {800.0f, 234.0f},   // [rev⋅min⁻¹, N⋅m] - Idle
        {1000.0f, 270.0f},  // [rev⋅min⁻¹, N⋅m] - Off idle
        {1500.0f, 322.0f},  // [rev⋅min⁻¹, N⋅m] - Low build
        {2000.0f, 374.0f},  // [rev⋅min⁻¹, N⋅m] - Low range
        {2500.0f, 408.0f},  // [rev⋅min⁻¹, N⋅m] - Transition
        {3000.0f, 442.0f},  // [rev⋅min⁻¹, N⋅m] - Mid-range climb
        {3500.0f, 463.0f},  // [rev⋅min⁻¹, N⋅m] - Mid climb
        {4000.0f, 483.0f},  // [rev⋅min⁻¹, N⋅m] - Building
        {4500.0f, 496.0f},  // [rev⋅min⁻¹, N⋅m] - Approaching peak
        {5000.0f, 509.0f},  // [rev⋅min⁻¹, N⋅m] - Near peak
        {5400.0f, 517.0f},  // [rev⋅min⁻¹, N⋅m] - Peak approach
        {5800.0f, 520.0f},  // [rev⋅min⁻¹, N⋅m] - Peak torque
        {6200.0f, 518.0f},  // [rev⋅min⁻¹, N⋅m] - Peak hold
        {6500.0f, 515.0f},  // [rev⋅min⁻¹, N⋅m] - Power band
        {7000.0f, 505.0f},  // [rev⋅min⁻¹, N⋅m] - High range
        {7500.0f, 490.0f},  // [rev⋅min⁻¹, N⋅m] - High rev
        {8000.0f, 468.0f},  // [rev⋅min⁻¹, N⋅m] - Near redline
        {8500.0f, 442.0f}   // [rev⋅min⁻¹, N⋅m] - Redline
    };

    /* ========================== EXHAUST CURVES ========================== */
    TArray<FVector2D> ExhaustFlowCurve = {
        {800.0f, 0.04f},    // [rev⋅min⁻¹, kg⋅s⁻¹] - Idle
        {2000.0f, 0.10f},   // [rev⋅min⁻¹, kg⋅s⁻¹] - Low
        {4000.0f, 0.22f},   // [rev⋅min⁻¹, kg⋅s⁻¹] - Mid
        {6000.0f, 0.35f},   // [rev⋅min⁻¹, kg⋅s⁻¹] - High
        {8500.0f, 0.48f}    // [rev⋅min⁻¹, kg⋅s⁻¹] - Redline
    };

    TArray<FVector2D> ExhaustTempCurve = {
        {800.0f, 673.0f},   // [rev⋅min⁻¹, K] - Idle (400°C)
        {2000.0f, 873.0f},  // [rev⋅min⁻¹, K] - Low load (600°C)
        {4000.0f, 1023.0f}, // [rev⋅min⁻¹, K] - Mid load (750°C)
        {6000.0f, 1123.0f}, // [rev⋅min⁻¹, K] - High load (850°C)
        {8500.0f, 1173.0f}  // [rev⋅min⁻¹, K] - Redline (900°C)
    };

    /* ========================== ENGINE FUNCTIONS ========================== */
    /** Sample torque from curve */
    FORCEINLINE float SampleTorque(float RPM) const
    {
        // Reason: handle empty curve
        if (TorqueCurve.Num() < 2) return 0.0f; // End if (invalid curve)
        
        // Reason: clamp to curve bounds
        if (RPM <= TorqueCurve[0].X) return TorqueCurve[0].Y; // End if (below idle)
        if (RPM >= TorqueCurve.Last().X) return TorqueCurve.Last().Y; // End if (above redline)
        
        // Reason: binary search for segment
        int32 Low = 0;
        int32 High = TorqueCurve.Num() - 1;
        while (High - Low > 1)
        {
            int32 Mid = (Low + High) / 2;
            if (RPM < TorqueCurve[Mid].X) High = Mid;
            else Low = Mid;
        } // End while (binary search)
        
        float RPM0 = TorqueCurve[Low].X;     // [rev⋅min⁻¹]
        float RPM1 = TorqueCurve[High].X;    // [rev⋅min⁻¹]
        float Torque0 = TorqueCurve[Low].Y;  // [N⋅m]
        float Torque1 = TorqueCurve[High].Y; // [N⋅m]
        float Alpha = (RPM - RPM0) / (RPM1 - RPM0); // [-]
        return FMath::Lerp(Torque0, Torque1, Alpha); // [N⋅m]
    }

    /** Sample exhaust flow rate from curve */
    FORCEINLINE float SampleFlowRate(float RPM) const
    {
        // Reason: handle empty curve
        if (ExhaustFlowCurve.Num() < 2) return 0.0f; // End if (invalid curve)
        
        // Reason: clamp to curve bounds
        if (RPM <= ExhaustFlowCurve[0].X) return ExhaustFlowCurve[0].Y; // End if (below idle)
        if (RPM >= ExhaustFlowCurve.Last().X) return ExhaustFlowCurve.Last().Y; // End if (above redline)
        
        // Reason: binary search for segment
        int32 Low = 0;
        int32 High = ExhaustFlowCurve.Num() - 1;
        while (High - Low > 1)
        {
            int32 Mid = (Low + High) / 2;
            if (RPM < ExhaustFlowCurve[Mid].X) High = Mid;
            else Low = Mid;
        } // End while (binary search)
        
        float RPM0 = ExhaustFlowCurve[Low].X;     // [rev⋅min⁻¹]
        float RPM1 = ExhaustFlowCurve[High].X;    // [rev⋅min⁻¹]
        float Flow0 = ExhaustFlowCurve[Low].Y;    // [kg⋅s⁻¹]
        float Flow1 = ExhaustFlowCurve[High].Y;   // [kg⋅s⁻¹]
        float Alpha = (RPM - RPM0) / (RPM1 - RPM0); // [-]
        return FMath::Lerp(Flow0, Flow1, Alpha); // [kg⋅s⁻¹]
    }

    /** Sample exhaust temperature from curve */
    FORCEINLINE float SampleExhaustTemp(float RPM) const
    {
        // Reason: handle empty curve
        if (ExhaustTempCurve.Num() < 2) return 673.0f; // End if (invalid curve)
        
        // Reason: clamp to curve bounds
        if (RPM <= ExhaustTempCurve[0].X) return ExhaustTempCurve[0].Y; // End if (below idle)
        if (RPM >= ExhaustTempCurve.Last().X) return ExhaustTempCurve.Last().Y; // End if (above redline)
        
        // Reason: binary search for segment
        int32 Low = 0;
        int32 High = ExhaustTempCurve.Num() - 1;
        while (High - Low > 1)
        {
            int32 Mid = (Low + High) / 2;
            if (RPM < ExhaustTempCurve[Mid].X) High = Mid;
            else Low = Mid;
        } // End while (binary search)
        
        float RPM0 = ExhaustTempCurve[Low].X;     // [rev⋅min⁻¹]
        float RPM1 = ExhaustTempCurve[High].X;    // [rev⋅min⁻¹]
        float Temp0 = ExhaustTempCurve[Low].Y;    // [K]
        float Temp1 = ExhaustTempCurve[High].Y;   // [K]
        float Alpha = (RPM - RPM0) / (RPM1 - RPM0); // [-]
        return FMath::Lerp(Temp0, Temp1, Alpha); // [K]
    }

  /** GT-R Nismo engine configuration */
void ConfigureEngine(int32 InPresetID)
{
    if (InPresetID == 1) // Reason: preset 1 configuration check
    {
        EngineID = TEXT("NissanGTRNismo");
        MaxTorque_Nm = 352.0f;       // [N·m] - Peak torque at 3600-5800 RPM
        IdleRPM = 700.0f;            // [rev·min⁻¹]
        RedlineRPM = 7200.0f;        // [rev·min⁻¹]
        MaxRPMIncreasePerSec = 1800.0f; // [rev·min⁻¹·s⁻¹] - Aggressive rise rate
        MaxRPMDecreasePerSec = 2600.0f; // [rev·min⁻¹·s⁻¹] - Faster drop for engine braking
        
        //------------------------------------------------------------------------------
        // ENGINE BRAKING - CORRECTED VALUES (VR38DETT Plasma-Sprayed Low-Friction Engine)
        //------------------------------------------------------------------------------
        // PHYSICS MODEL:
        // T_brake = -(C_linear × ω + C_quad × ω²) × BlendFactor
        // 
        // RESEARCH BASIS (from academic sources):
        // - Modern gasoline FMEP: 0.13-0.77 bar at low load
        // - VR38DETT has plasma-sprayed bores → 30-40% less friction
        // - Twin-turbo low compression (9.0:1) → reduced pumping losses
        // - Hand-built precision engine → smoother operation
        //
        // OLD VALUES (WRONG):
        // EngineBrakingCoeff = 0.38f;       → Caused 418 Nm @ 5000 RPM (64% loss!)
        // EngineBrakingQuadratic = 0.0008f; → Caused 708 Nm @ 7000 RPM (124% loss!)
        //
        // CALCULATION METHOD:
        // Using FMEP approach: T = (FMEP × V_displacement) / (2π × 2)
        // Target FMEP @ 5000 RPM: ~0.8 bar (realistic for twin-turbo)
        // V_d = 3.8L = 0.0038 m³
        // T_target = (80000 Pa × 0.0038 m³) / (4π) ≈ 24 Nm base
        //
        // Linear term dominates at low/mid RPM (bearing + piston friction)
        // Quadratic term grows at high RPM (pumping + windage)
        //------------------------------------------------------------------------------
        
        EngineBrakingCoeff = 0.08f;       // [N·m·s·rad⁻¹] - Linear friction (mechanical)
        EngineBrakingQuadratic = 0.00012f; // [N·m·s²·rad⁻²] - Pumping losses (aerodynamic)
        
        //------------------------------------------------------------------------------
        // EXPECTED BRAKING TORQUE WITH NEW VALUES:
        //------------------------------------------------------------------------------
        // RPM  | ω [rad/s] | Linear [Nm] | Quad [Nm] | Total [Nm] | % of Peak Torque
        // -----|-----------|-------------|-----------|------------|------------------
        // 1000 |   104.7   |     8.4     |    1.3    |    9.7     |      1.5%
        // 2500 |   261.8   |    20.9     |    8.2    |   29.1     |      4.5%
        // 5000 |   523.6   |    41.9     |   32.9    |   74.8     |     11.5%
        // 7000 |   733.0   |    58.6     |   64.5    |  123.1     |     18.9%
        // 7200 |   754.0   |    60.3     |   68.2    |  128.5     |     19.7%
        //
        // COMPARISON TO OLD VALUES:
        // @ 5000 RPM: 74.8 Nm (NEW) vs 418 Nm (OLD) → 82% reduction ✓
        // @ 7000 RPM: 123.1 Nm (NEW) vs 708 Nm (OLD) → 83% reduction ✓
        //------------------------------------------------------------------------------
        
        EngineInertia = 0.38f;       // [kg·m²] - VR38DETT crank + flywheel estimate
        InvEngineInertia = 2.631578947f; // [kg⁻¹·m⁻²] - (1 / 0.38)

        // Thermal properties for VR38DETT
        AmbientTemperature_K = 293.15f;
        EngineThermalMass = 220000.0f;      // [J·K⁻¹] - Larger V6 + intercooler
        HeatInefficiencyFactor = 2.2f;      // [-] - Turbocharged engines run hotter
        PassiveCoolingFactor = 45.0f;       // [W·K⁻¹]
        ThermostatOpenTemp_K = 358.15f;     // [K] - 85°C
        ThermostatFullOpenTemp_K = 368.15f; // [K] - 95°C
        BaseFanCoolingFactor = 250.0f;      // [W·K⁻¹] - Upgraded cooling for performance
        SpeedCoolingFactor = 18.0f;         // [W·K⁻¹·m⁻¹·s]
        OverheatTemp_K = 393.15f;           // [K] - 120°C
        DamageTemp_K = 403.15f;             // [K] - 130°C

        // Fuel properties
        BaseThermalEfficiency = 0.36f;      // [0-1] - Twin-turbo efficiency
        BoostedThermalEfficiency = 0.28f;   // [0-1] - Rich mixture under boost

        // VR38DETT torque curve - 600 hp @ 6800 RPM, 652 Nm from 3600-5800 RPM
        TorqueCurve = {
            FVector2D(700.0f, 220.0f),   // [rev·min⁻¹, N·m] - Idle
            FVector2D(1500.0f, 350.0f),  // [rev·min⁻¹, N·m] - Off idle
            FVector2D(2500.0f, 500.0f),  // [rev·min⁻¹, N·m] - Low build
            FVector2D(3000.0f, 580.0f),  // [rev·min⁻¹, N·m] - Boost building
            FVector2D(3600.0f, 652.0f),  // [rev·min⁻¹, N·m] - Peak torque start
            FVector2D(4000.0f, 652.0f),  // [rev·min⁻¹, N·m] - Peak hold
            FVector2D(4500.0f, 652.0f),  // [rev·min⁻¹, N·m] - Peak hold
            FVector2D(5000.0f, 652.0f),  // [rev·min⁻¹, N·m] - Peak hold
            FVector2D(5800.0f, 652.0f),  // [rev·min⁻¹, N·m] - Peak torque end
            FVector2D(6000.0f, 640.0f),  // [rev·min⁻¹, N·m] - Post peak
            FVector2D(6500.0f, 620.0f),  // [rev·min⁻¹, N·m] - Power band
            FVector2D(6800.0f, 600.0f),  // [rev·min⁻¹, N·m] - Peak power
            FVector2D(7000.0f, 570.0f),  // [rev·min⁻¹, N·m] - High rev
            FVector2D(7200.0f, 520.0f)   // [rev·min⁻¹, N·m] - Redline
        };

        // Exhaust curves for twin-turbo V6
        ExhaustFlowCurve = {
            FVector2D(700.0f, 0.05f),    // [rev·min⁻¹, kg·s⁻¹] - Idle
            FVector2D(2000.0f, 0.12f),   // [rev·min⁻¹, kg·s⁻¹] - Low
            FVector2D(4000.0f, 0.28f),   // [rev·min⁻¹, kg·s⁻¹] - Mid
            FVector2D(6000.0f, 0.42f),   // [rev·min⁻¹, kg·s⁻¹] - High
            FVector2D(7200.0f, 0.52f)    // [rev·min⁻¹, kg·s⁻¹] - Redline
        };

        ExhaustTempCurve = {
            FVector2D(700.0f, 693.0f),   // [rev·min⁻¹, K] - Idle (420°C)
            FVector2D(2000.0f, 923.0f),  // [rev·min⁻¹, K] - Low load (650°C)
            FVector2D(4000.0f, 1073.0f), // [rev·min⁻¹, K] - Mid load (800°C)
            FVector2D(6000.0f, 1173.0f), // [rev·min⁻¹, K] - High load (900°C)
            FVector2D(7200.0f, 1223.0f)  // [rev·min⁻¹, K] - Redline (950°C)
        };

        TraceConfiguration();
    } // End if (preset 1 configuration)
}

    /* ========================== DEBUGGING ========================== */
    void TraceConfiguration() const
    {
        UE_LOG(LogTemp, Log, TEXT("--- Engine Configuration ---"));
        UE_LOG(LogTemp, Log, TEXT("ID: %s | MaxTorque: %.0f Nm | Idle: %.0f RPM | Redline: %.0f RPM"), *EngineID.ToString(), MaxTorque_Nm, IdleRPM, RedlineRPM);
        UE_LOG(LogTemp, Log, TEXT("Inertia: %.3f kg⋅m² | BrakingCoeff: %.3f | BrakingQuad: %.5f"), EngineInertia, EngineBrakingCoeff, EngineBrakingQuadratic);
        UE_LOG(LogTemp, Log, TEXT("ThermalMass: %.0f J·K^-1 | BaseCooling: %.0f W·K^-1 | SpeedCooling: %.0f W·K^-1·m^-1·s"), EngineThermalMass, BaseFanCoolingFactor, SpeedCoolingFactor);
        UE_LOG(LogTemp, Log, TEXT("BaseEfficiency: %.2f | BoostedEfficiency: %.2f"), BaseThermalEfficiency, BoostedThermalEfficiency);
        UE_LOG(LogTemp, Log, TEXT("TorqueCurve points: %d | ExhaustFlow points: %d | ExhaustTemp points: %d"), TorqueCurve.Num(), ExhaustFlowCurve.Num(), ExhaustTempCurve.Num());
        UE_LOG(LogTemp, Log, TEXT("--------------------------"));
    }

    void DisplayEngineMetrics(float CurrentRPM, float LoadTorque, bool bShowRPM, bool bShowTorque, bool bShowPower, FColor TextColor) const
    {
        // Reason: early exit if no engine reference
        if (!GEngine) return; // End if (no engine)
        
        FString Output;
        if (bShowRPM) Output += FString::Printf(TEXT("RPM: %.0f "), CurrentRPM);
        if (bShowTorque) Output += FString::Printf(TEXT("| Torque: %.1f Nm "), SampleTorque(CurrentRPM));
        
        // Reason: power calculation check
        if (bShowPower)
        {
            float PowerW = SampleTorque(CurrentRPM) * CurrentRPM * 2.0f * PI / 60.0f; // [W]
            Output += FString::Printf(TEXT("| Power: %.0f kW "), PowerW / 1000.0f);
        } // End if (power calculation)
        
        Output += FString::Printf(TEXT("| Load: %.1f Nm | Inertia: %.3f kg⋅m²"), LoadTorque, EngineInertia);
        GEngine->AddOnScreenDebugMessage(1111, 0.0f, TextColor, Output);
    }
};

/** Engine state vector - Runtime state for engine operation (physics thread only) */
struct FEngineStateVector
{
    float CurrentEngineRPM = 800.0f;         // [rev⋅min⁻¹] - Current engine speed
    float EngineLoadTorque = 0.0f;           // [N⋅m] - Drivetrain load torque
    float CurrentTorqueOutput = 0.0f;        // [N⋅m] - Current torque output
    float CurrentPowerOutput = 0.0f;         // [kW] - Current power output
    float EngineBrakingTorque = 0.0f;        // [N⋅m] - Engine braking torque
    bool  bEngineRunning = false;            // [-]   - Engine running state
    float EngineTemperature = 293.15f;       // [K]   - Engine block temperature (starts at 20°C)
    float CoolantTempK = 293.15f;            // [K]   - Engine coolant temperature (starts at ambient 20°C)
    float ManifoldPressure = 101325.0f;      // [Pa]  - Intake manifold pressure (diagnostic)
    float ManifoldTempK = 293.15f;           // [K]   - Intake manifold temperature (diagnostic)
    float TurboShaftRPM = 0.0f;              // [rev⋅min⁻¹] - Turbocharger shaft speed (diagnostic)
    float CurrentFuelLiters = 70.0f;         // [L]   - Current fuel in tank (for UI)
    float PrevEngineOmega_rad_s = 0.0f;      // [rad⋅s⁻¹] - Previous engine angular velocity (for filters)
    
    FEngineStateVector()
        : CurrentEngineRPM(800.0f)
        , EngineLoadTorque(0.0f)
        , CurrentTorqueOutput(0.0f)
        , CurrentPowerOutput(0.0f)
        , EngineBrakingTorque(0.0f)
        , bEngineRunning(false)
        , EngineTemperature(293.15f)
        , CoolantTempK(293.15f)
        , ManifoldPressure(101325.0f)
        , ManifoldTempK(293.15f)
        , TurboShaftRPM(0.0f)
        , CurrentFuelLiters(70.0f)
        , PrevEngineOmega_rad_s(0.0f)
    {}
};