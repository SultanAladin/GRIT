#pragma once

#include "CoreMinimal.h"

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                              💨 SUPERCHARGER SYSTEM
//----------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                              📋 ENUMERATIONS
//----------------------------------------------------------------------------------------------------------------------------------------

/** Supercharger drive type */
UENUM(BlueprintType)
enum class ESuperchargerDriveType : uint8
{
    Roots,          // Roots-type positive displacement
    TwinScrew,      // Twin-screw positive displacement
    Centrifugal     // Centrifugal compressor
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                  🧩 Supercharger Specifications (Immutable)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FSuperchargerSpecifications
{
    FSuperchargerSpecifications(int32 PresetID = 1)
    {
        DriveType = ESuperchargerDriveType::TwinScrew; // [-]
        ChargerInertia = 0.015f;                    // [kg·m²]
        InvChargerInertia = 66.67f;                 // [kg⁻¹·m⁻²]
        ChargerFrictionCoeff = 0.003f;              // [N·m·s·rad⁻¹]
        ChargerFrictionQuadratic = 0.00008f;        // [N·m·s²·rad⁻²]
        DriveRatio = 3.2f;                          // [ratio]
        MaxBoost_Base = 0.8f;                       // [Bar]
        MaxBoost_Race = 1.2f;                       // [Bar]
        BaseTuneOctane = 91.0f;                     // [-]
        RaceTuneOctane = 98.0f;                     // [-]
        BypassValveRate = 8.0f;                     // [Bar·s⁻¹]
        CompressorEfficiency = 0.72f;               // [-]
        ParasiticLossFactor = 0.08f;                // [-]
        
        ConfigureSupercharger(PresetID);
    }

    /* ================================================= ARCHITECTURE ================================================= */
    
    ESuperchargerDriveType DriveType;               // [-]

    /* ================================================= PHYSICAL PROPERTIES ================================================= */
    
    float ChargerInertia;                           // [kg·m²]
    float InvChargerInertia;                        // [kg⁻¹·m⁻²]
    float ChargerFrictionCoeff;                     // [N·m·s·rad⁻¹]
    float ChargerFrictionQuadratic;                 // [N·m·s²·rad⁻²]
    float DriveRatio;                               // [ratio]

    /* ================================================= CONTROL SYSTEMS ================================================= */
    
    float MaxBoost_Base;                            // [Bar]
    float MaxBoost_Race;                            // [Bar]
    float BaseTuneOctane;                           // [-]
    float RaceTuneOctane;                           // [-]
    float BypassValveRate;                          // [Bar·s⁻¹]

    /* ================================================= EFFICIENCY ================================================= */
    
    float CompressorEfficiency;                     // [-]
    float ParasiticLossFactor;                      // [-]

    /* ================================================= PERFORMANCE CURVES ================================================= */
    
    TArray<FVector2D> BoostPressureCurve;           // [rev·min⁻¹, Bar]
    TArray<FVector2D> TorqueMultiplierCurve;        // [Bar, -]
    TArray<FVector2D> ParasiticDragCurve;           // [rev·min⁻¹, N·m]

    /* ================================================= CONFIGURATION ================================================= */
    
    void ConfigureSupercharger(int32 InPresetID)
    {
        if (InPresetID == 1)
        {
            DriveType = ESuperchargerDriveType::TwinScrew; // [-]
            ChargerInertia = 0.015f;                // [kg·m²]
            InvChargerInertia = 1.0f / ChargerInertia; // [kg⁻¹·m⁻²]
            ChargerFrictionCoeff = 0.003f;          // [N·m·s·rad⁻¹]
            ChargerFrictionQuadratic = 0.00008f;    // [N·m·s²·rad⁻²]
            DriveRatio = 3.2f;                      // [ratio]
            MaxBoost_Base = 0.8f;                   // [Bar]
            MaxBoost_Race = 1.2f;                   // [Bar]
            BaseTuneOctane = 91.0f;                 // [-]
            RaceTuneOctane = 98.0f;                 // [-]
            BypassValveRate = 8.0f;                 // [Bar·s⁻¹]
            CompressorEfficiency = 0.72f;           // [-]
            ParasiticLossFactor = 0.08f;            // [-]
            
            BoostPressureCurve = {
                FVector2D(2000.0f, 0.0f),           // [rev·min⁻¹, Bar]
                FVector2D(5000.0f, 0.3f),           // [rev·min⁻¹, Bar]
                FVector2D(10000.0f, 0.6f),          // [rev·min⁻¹, Bar]
                FVector2D(15000.0f, 0.8f),          // [rev·min⁻¹, Bar]
                FVector2D(20000.0f, 1.0f),          // [rev·min⁻¹, Bar]
                FVector2D(25000.0f, 1.15f)          // [rev·min⁻¹, Bar]
            };
            
            TorqueMultiplierCurve = {
                FVector2D(0.0f, 1.0f),              // [Bar, -]
                FVector2D(0.4f, 1.25f),             // [Bar, -]
                FVector2D(0.8f, 1.55f),             // [Bar, -]
                FVector2D(1.2f, 1.75f)              // [Bar, -]
            };
            
            ParasiticDragCurve = {
                FVector2D(2000.0f, 5.0f),           // [rev·min⁻¹, N·m]
                FVector2D(5000.0f, 15.0f),          // [rev·min⁻¹, N·m]
                FVector2D(10000.0f, 35.0f),         // [rev·min⁻¹, N·m]
                FVector2D(15000.0f, 60.0f),         // [rev·min⁻¹, N·m]
                FVector2D(20000.0f, 90.0f),         // [rev·min⁻¹, N·m]
                FVector2D(25000.0f, 125.0f)         // [rev·min⁻¹, N·m]
            };
            
            TraceConfiguration();
        }
    }

    /* ================================================= CALCULATIONS ================================================= */
    
    float CalcChargerRPM(float EngineRPM) const
    {
        return EngineRPM * DriveRatio;              // [rev·min⁻¹]
    }

    /* ================================================= CURVE EVALUATION ================================================= */
    
    float EvalBoostPressure(float ChargerRPM) const
    {
        if (BoostPressureCurve.Num() < 2) return 0.0f;
        if (ChargerRPM <= BoostPressureCurve[0].X) return BoostPressureCurve[0].Y;
        if (ChargerRPM >= BoostPressureCurve.Last().X) return BoostPressureCurve.Last().Y;

        int32 Low = 0;
        int32 High = BoostPressureCurve.Num() - 1;
        while (High - Low > 1)
        {
            int32 Mid = (Low + High) / 2;
            if (ChargerRPM < BoostPressureCurve[Mid].X) High = Mid;
            else Low = Mid;
        }

        float RPM0 = BoostPressureCurve[Low].X;     // [rev·min⁻¹]
        float RPM1 = BoostPressureCurve[High].X;    // [rev·min⁻¹]
        float Boost0 = BoostPressureCurve[Low].Y;   // [Bar]
        float Boost1 = BoostPressureCurve[High].Y;  // [Bar]
        float Alpha = (ChargerRPM - RPM0) / (RPM1 - RPM0); // [-]
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

    float EvalParasiticDrag(float ChargerRPM) const
    {
        if (ParasiticDragCurve.Num() < 2) return 0.0f;
        if (ChargerRPM <= ParasiticDragCurve[0].X) return ParasiticDragCurve[0].Y;
        if (ChargerRPM >= ParasiticDragCurve.Last().X) return ParasiticDragCurve.Last().Y;

        int32 Low = 0;
        int32 High = ParasiticDragCurve.Num() - 1;
        while (High - Low > 1)
        {
            int32 Mid = (Low + High) / 2;
            if (ChargerRPM < ParasiticDragCurve[Mid].X) High = Mid;
            else Low = Mid;
        }

        float RPM0 = ParasiticDragCurve[Low].X;     // [rev·min⁻¹]
        float RPM1 = ParasiticDragCurve[High].X;    // [rev·min⁻¹]
        float Drag0 = ParasiticDragCurve[Low].Y;    // [N·m]
        float Drag1 = ParasiticDragCurve[High].Y;   // [N·m]
        float Alpha = (ChargerRPM - RPM0) / (RPM1 - RPM0); // [-]
        return FMath::Lerp(Drag0, Drag1, Alpha);    // [N·m]
    }

    /* ================================================= DEBUGGING ================================================= */
    
    void TraceConfiguration() const
    {
        UE_LOG(LogTemp, Log, TEXT("--- Supercharger Configuration ---"));
        UE_LOG(LogTemp, Log, TEXT("Type: %d | Drive Ratio: %.2f | Inertia: %.3f kg·m²"), (int32)DriveType, DriveRatio, ChargerInertia);
        UE_LOG(LogTemp, Log, TEXT("Max Boost Base: %.2f Bar | Race: %.2f Bar"), MaxBoost_Base, MaxBoost_Race);
        UE_LOG(LogTemp, Log, TEXT("Compressor Efficiency: %.2f | Parasitic Loss: %.2f"), CompressorEfficiency, ParasiticLossFactor);
        UE_LOG(LogTemp, Log, TEXT("Boost Curve Points: %d | Multiplier Points: %d | Drag Points: %d"), BoostPressureCurve.Num(), TorqueMultiplierCurve.Num(), ParasiticDragCurve.Num());
        UE_LOG(LogTemp, Log, TEXT("----------------------------------"));
    }
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                              ⚡ Supercharger Runtime State (Mutable, Physics Thread)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FSuperchargerStateVector
{
    FSuperchargerStateVector()
        : ChargerRPM(0.0f)                          // [rev·min⁻¹]
        , BoostPressure(0.0f)                       // [Bar]
        , BypassValvePosition(0.0f)                 // [-]
        , ParasiticDrag(0.0f)                       // [N·m]
        , ChargerInletTemp(293.15f)                 // [K]
    {}

    float ChargerRPM;                               // [rev·min⁻¹]
    float BoostPressure;                            // [Bar]
    float BypassValvePosition;                      // [-]
    float ParasiticDrag;                            // [N·m]
    float ChargerInletTemp;                         // [K]
};