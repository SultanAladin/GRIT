#pragma once

#include "CoreMinimal.h"

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                              🌪️ TURBOCHARGER SYSTEM
//----------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                  🧩 Turbocharger Specifications (Immutable)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FTurbochargerSpecifications
{
    FTurbochargerSpecifications(int32 PresetID = 1)
    {
        TurboInertia = 0.02f;                       // [kg·m²]
        InvTurboInertia = 50.0f;                    // [kg⁻¹·m⁻²]
        TurboFrictionCoeff = 0.002f;                // [N·m·s·rad⁻¹]
        TurboFrictionQuadratic = 0.00005f;          // [N·m·s²·rad⁻²]
        WastegateMaxBoost_Base = 1.2f;              // [Bar]
        WastegateMaxBoost_Race = 1.6f;              // [Bar]
        BaseTuneOctane = 93.0f;                     // [-]
        RaceTuneOctane = 100.0f;                    // [-]
        BovPressureReleaseRate = 10.0f;             // [Bar·s⁻¹]
        TurbineEfficiency = 0.65f;                  // [-]
        CompressorEfficiency = 0.70f;               // [-] - Peak compressor efficiency
        MechanicalEfficiency = 0.98f;               // [-] - Bearing losses (journal + thrust)
        
        ConfigureTurbo(PresetID);
    }

    /* ================================================= PHYSICAL PROPERTIES ================================================= */
    
    float TurboInertia;                             // [kg·m²]
    float InvTurboInertia;                          // [kg⁻¹·m⁻²]
    float TurboFrictionCoeff;                       // [N·m·s·rad⁻¹]
    float TurboFrictionQuadratic;                   // [N·m·s²·rad⁻²]

    /* ================================================= CONTROL SYSTEMS ================================================= */
    
    float WastegateMaxBoost_Base;                   // [Bar]
    float WastegateMaxBoost_Race;                   // [Bar]
    float BaseTuneOctane;                           // [-]
    float RaceTuneOctane;                           // [-]
    float BovPressureReleaseRate;                   // [Bar·s⁻¹]

    /* ================================================= EFFICIENCY ================================================= */
    
    float TurbineEfficiency;                        // [-] - Turbine isentropic efficiency
    float CompressorEfficiency;                     // [-] - Compressor isentropic efficiency  
    float MechanicalEfficiency;                     // [-] - Shaft mechanical efficiency (bearing losses)

    /* ================================================= PERFORMANCE CURVES ================================================= */
    
    TArray<FVector2D> BoostPressureCurve;           // [rev·min⁻¹, Bar]
    TArray<FVector2D> TorqueMultiplierCurve;        // [Bar, -]
    TArray<FVector2D> CompressorMap;                // [kg·s⁻¹, PressureRatio] - Compressor performance map

    /* ================================================= CONFIGURATION ================================================= */
    
    void ConfigureTurbo(int32 InPresetID)
    {
        if (InPresetID == 1)
        {
            TurboInertia = 0.02f;                   // [kg·m²]
            InvTurboInertia = 1.0f / TurboInertia;  // [kg⁻¹·m⁻²]
            TurboFrictionCoeff = 0.002f;            // [N·m·s·rad⁻¹]
            TurboFrictionQuadratic = 0.00005f;      // [N·m·s²·rad⁻²]
            WastegateMaxBoost_Base = 1.2f;          // [Bar]
            WastegateMaxBoost_Race = 1.6f;          // [Bar]
            BaseTuneOctane = 93.0f;                 // [-]
            RaceTuneOctane = 100.0f;                // [-]
            BovPressureReleaseRate = 10.0f;         // [Bar·s⁻¹]
            TurbineEfficiency = 0.65f;              // [-] - Radial turbine isentropic efficiency (0.60-0.72 typical)
            CompressorEfficiency = 0.70f;           // [-] - Centrifugal compressor isentropic efficiency (0.70-0.80 typical)
            MechanicalEfficiency = 0.98f;           // [-] - Bearing mechanical efficiency (~2% losses)
            
            BoostPressureCurve = {
                FVector2D(1000.0f, 0.0f),           // [rev·min⁻¹, Bar]
                FVector2D(20000.0f, 0.2f),          // [rev·min⁻¹, Bar]
                FVector2D(50000.0f, 0.8f),          // [rev·min⁻¹, Bar]
                FVector2D(90000.0f, 1.2f),          // [rev·min⁻¹, Bar]
                FVector2D(120000.0f, 1.4f),         // [rev·min⁻¹, Bar]
                FVector2D(150000.0f, 1.3f)          // [rev·min⁻¹, Bar]
            };
            
            TorqueMultiplierCurve = {
                FVector2D(0.0f, 1.0f),              // [Bar, -]
                FVector2D(0.5f, 1.3f),              // [Bar, -]
                FVector2D(1.0f, 1.7f),              // [Bar, -]
                FVector2D(1.2f, 1.85f)              // [Bar, -]
            };
            
            CompressorMap = {
                FVector2D(0.0f, 1.0f),              // [kg·s⁻¹, PressureRatio] - Zero flow, no compression
                FVector2D(0.05f, 1.5f),             // [kg·s⁻¹, PressureRatio] - Low flow
                FVector2D(0.10f, 2.0f),             // [kg·s⁻¹, PressureRatio] - Mid flow
                FVector2D(0.15f, 2.2f),             // [kg·s⁻¹, PressureRatio] - High flow
                FVector2D(0.20f, 2.0f)              // [kg·s⁻¹, PressureRatio] - Surge limit
            };
            
            TraceConfiguration();
        }
    }

    /* ================================================= CURVE EVALUATION ================================================= */
    
    float EvalBoostPressure(float TurboRPM) const
    {
        if (BoostPressureCurve.Num() < 2) return 0.0f;
        if (TurboRPM <= BoostPressureCurve[0].X) return BoostPressureCurve[0].Y;
        if (TurboRPM >= BoostPressureCurve.Last().X) return BoostPressureCurve.Last().Y;

        int32 Low = 0;
        int32 High = BoostPressureCurve.Num() - 1;
        while (High - Low > 1)
        {
            int32 Mid = (Low + High) / 2;
            if (TurboRPM < BoostPressureCurve[Mid].X) High = Mid;
            else Low = Mid;
        }

        float RPM0 = BoostPressureCurve[Low].X;     // [rev·min⁻¹]
        float RPM1 = BoostPressureCurve[High].X;    // [rev·min⁻¹]
        float Boost0 = BoostPressureCurve[Low].Y;   // [Bar]
        float Boost1 = BoostPressureCurve[High].Y;  // [Bar]
        float Alpha = (TurboRPM - RPM0) / (RPM1 - RPM0); // [-]
        return FMath::Lerp(Boost0, Boost1, Alpha);  // [Bar]
    }

    float EvalTorqueMultiplier(float BoostPressure) const
    {
        if (TorqueMultiplierCurve.Num() < 2) return 1.0f;
        if (BoostPressure <= TorqueMultiplierCurve[0].X) return TorqueMultiplierCurve[0].Y;
        if (BoostPressure >= TorqueMultiplierCurve.Last().X) return TorqueMultiplierCurve.Last().Y;

        int32 Low = 0;
        int32 High = TorqueMultiplierCurve.Num() - 1;
        while (High - Low > 1)
        {
            int32 Mid = (Low + High) / 2;
            if (BoostPressure < TorqueMultiplierCurve[Mid].X) High = Mid;
            else Low = Mid;
        }

        float Boost0 = TorqueMultiplierCurve[Low].X;    // [Bar]
        float Boost1 = TorqueMultiplierCurve[High].X;   // [Bar]
        float Mult0 = TorqueMultiplierCurve[Low].Y;     // [-]
        float Mult1 = TorqueMultiplierCurve[High].Y;    // [-]
        float Alpha = (BoostPressure - Boost0) / (Boost1 - Boost0); // [-]
        return FMath::Lerp(Mult0, Mult1, Alpha);        // [-]
    }

    /* ================================================= DEBUGGING ================================================= */
    
    void TraceConfiguration() const
    {
        UE_LOG(LogTemp, Log, TEXT("--- Turbocharger Configuration ---"));
        UE_LOG(LogTemp, Log, TEXT("Inertia: %.3f kg·m² | Friction: %.4f N·m·s·rad^-1 | Quadratic: %.6f"), TurboInertia, TurboFrictionCoeff, TurboFrictionQuadratic);
        UE_LOG(LogTemp, Log, TEXT("Wastegate Base: %.2f Bar | Race: %.2f Bar"), WastegateMaxBoost_Base, WastegateMaxBoost_Race);
        UE_LOG(LogTemp, Log, TEXT("Turbine η: %.2f | Compressor η: %.2f | Mechanical η: %.2f"), TurbineEfficiency, CompressorEfficiency, MechanicalEfficiency);
        UE_LOG(LogTemp, Log, TEXT("BOV Rate: %.1f Bar·s^-1"), BovPressureReleaseRate);
        UE_LOG(LogTemp, Log, TEXT("Boost Curve Points: %d | Multiplier Points: %d | Compressor Map Points: %d"), BoostPressureCurve.Num(), TorqueMultiplierCurve.Num(), CompressorMap.Num());
        UE_LOG(LogTemp, Log, TEXT("----------------------------------"));
    }
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                              ⚡ Turbocharger Runtime State (Mutable, Physics Thread)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FTurbochargerStateVector
{
    FTurbochargerStateVector()
        : TurboShaftRPM(0.0f)                       // [rev·min⁻¹]
        , BoostPressure(0.0f)                       // [Bar]
        , CurrentBoostPressure(0.0f)                // [Bar] - Filtered boost pressure (with BOV bleed)
        , WastegatePosition(0.0f)                   // [-]
        , BovPosition(0.0f)                         // [-]
        , TurbineInletTemp(293.15f)                 // [K]
    {}

    float TurboShaftRPM;                            // [rev·min⁻¹]
    float BoostPressure;                            // [Bar]
    float CurrentBoostPressure;                     // [Bar] - Filtered boost pressure (with BOV bleed)
    float WastegatePosition;                        // [-]
    float BovPosition;                              // [-]
    float TurbineInletTemp;                         // [K]
};