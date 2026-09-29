#pragma once

#include "CoreMinimal.h"

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                              TURBOCHARGER SYSTEM
//----------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                  Turbocharger Specifications (Immutable)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FTurbochargerSpecifications
{
    FTurbochargerSpecifications(int32 PresetID = 1)
    {
        TurboInertia = 0.00004f;
        InvTurboInertia = 25000.0f;
        TurboFrictionCoeff = 0.00008f;
        TurboFrictionQuadratic = 0.0000002f;
        WastegateMaxBoost_Base = 1.2f;
        WastegateMaxBoost_Race = 1.6f;
        BaseTuneOctane = 93.0f;
        RaceTuneOctane = 100.0f;
        BovPressureReleaseRate = 10.0f;
        TurbineEfficiency = 0.65f;
        CompressorEfficiency = 0.70f;
        MechanicalEfficiency = 0.98f;

        ConfigureTurbo(PresetID);
    }

    float TurboInertia;
    float InvTurboInertia;
    float TurboFrictionCoeff;
    float TurboFrictionQuadratic;

    float WastegateMaxBoost_Base;
    float WastegateMaxBoost_Race;
    float BaseTuneOctane;
    float RaceTuneOctane;
    float BovPressureReleaseRate;

    float TurbineEfficiency;
    float CompressorEfficiency;
    float MechanicalEfficiency;

    TArray<FVector2D> BoostPressureCurve;
    TArray<FVector2D> TorqueMultiplierCurve;
    TArray<FVector2D> CompressorMap;

    void ConfigureTurbo(int32 InPresetID)
    {
        if (InPresetID == 1)
        {
            TurboInertia = 0.00004f;
            InvTurboInertia = 1.0f / TurboInertia;
            TurboFrictionCoeff = 0.00008f;
            TurboFrictionQuadratic = 0.0000002f;
            WastegateMaxBoost_Base = 1.2f;
            WastegateMaxBoost_Race = 1.6f;
            BaseTuneOctane = 93.0f;
            RaceTuneOctane = 100.0f;
            BovPressureReleaseRate = 10.0f;
            TurbineEfficiency = 0.65f;
            CompressorEfficiency = 0.70f;
            MechanicalEfficiency = 0.98f;

            BoostPressureCurve = {
                FVector2D(20000.0f, 0.00f),
                FVector2D(35000.0f, 0.08f),
                FVector2D(50000.0f, 0.24f),
                FVector2D(65000.0f, 0.48f),
                FVector2D(90000.0f, 0.85f),
                FVector2D(120000.0f, 1.15f),
                FVector2D(150000.0f, 1.30f),
                FVector2D(180000.0f, 1.22f)
            };

            TorqueMultiplierCurve = {
                FVector2D(0.0f, 1.0f),
                FVector2D(0.5f, 1.3f),
                FVector2D(1.0f, 1.7f),
                FVector2D(1.2f, 1.85f)
            };

            // Compact compressor map covering the actual mass-flow range seen in the
            // supplied telemetry, with a choke-side falloff to avoid over-optimistic PR.
            CompressorMap = {
                FVector2D(0.00f, 1.00f),
                FVector2D(0.08f, 1.55f),
                FVector2D(0.18f, 2.10f),
                FVector2D(0.30f, 2.25f),
                FVector2D(0.40f, 1.90f)
            };

            TraceConfiguration();
        }
    }

    float EvalBoostPressure(float TurboRPM) const
    {
        if (BoostPressureCurve.Num() < 2) return 0.0f;
        if (TurboRPM <= BoostPressureCurve[0].X) return BoostPressureCurve[0].Y;
        if (TurboRPM >= BoostPressureCurve.Last().X) return BoostPressureCurve.Last().Y;

        int32 Low = 0;
        int32 High = BoostPressureCurve.Num() - 1;
        while (High - Low > 1)
        {
            const int32 Mid = (Low + High) / 2;
            if (TurboRPM < BoostPressureCurve[Mid].X) High = Mid;
            else Low = Mid;
        }

        const float RPM0 = BoostPressureCurve[Low].X;
        const float RPM1 = BoostPressureCurve[High].X;
        const float Boost0 = BoostPressureCurve[Low].Y;
        const float Boost1 = BoostPressureCurve[High].Y;
        const float Alpha = (TurboRPM - RPM0) / (RPM1 - RPM0);
        return FMath::Lerp(Boost0, Boost1, Alpha);
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
            const int32 Mid = (Low + High) / 2;
            if (BoostPressure < TorqueMultiplierCurve[Mid].X) High = Mid;
            else Low = Mid;
        }

        const float Boost0 = TorqueMultiplierCurve[Low].X;
        const float Boost1 = TorqueMultiplierCurve[High].X;
        const float Mult0 = TorqueMultiplierCurve[Low].Y;
        const float Mult1 = TorqueMultiplierCurve[High].Y;
        const float Alpha = (BoostPressure - Boost0) / (Boost1 - Boost0);
        return FMath::Lerp(Mult0, Mult1, Alpha);
    }

    void TraceConfiguration() const
    {
        UE_LOG(LogTemp, Log, TEXT("--- Turbocharger Configuration ---"));
        UE_LOG(LogTemp, Log, TEXT("Inertia: %.3f kg*m^2 | Friction: %.4f N*m*s*rad^-1 | Quadratic: %.6f"), TurboInertia, TurboFrictionCoeff, TurboFrictionQuadratic);
        UE_LOG(LogTemp, Log, TEXT("Wastegate Base: %.2f Bar | Race: %.2f Bar"), WastegateMaxBoost_Base, WastegateMaxBoost_Race);
        UE_LOG(LogTemp, Log, TEXT("Turbine Eff: %.2f | Compressor Eff: %.2f | Mechanical Eff: %.2f"), TurbineEfficiency, CompressorEfficiency, MechanicalEfficiency);
        UE_LOG(LogTemp, Log, TEXT("BOV Rate: %.1f Bar*s^-1"), BovPressureReleaseRate);
        UE_LOG(LogTemp, Log, TEXT("Boost Curve Points: %d | Multiplier Points: %d | Compressor Map Points: %d"), BoostPressureCurve.Num(), TorqueMultiplierCurve.Num(), CompressorMap.Num());
        UE_LOG(LogTemp, Log, TEXT("----------------------------------"));
    }
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                              Turbocharger Runtime State (Mutable, Physics Thread)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FTurbochargerStateVector
{
    FTurbochargerStateVector()
        : TurboShaftRPM(0.0f)
        , BoostPressure(0.0f)
        , CurrentBoostPressure(0.0f)
        , WastegatePosition(0.0f)
        , BovPosition(0.0f)
        , TurbineInletTemp(293.15f)
    {}

    float TurboShaftRPM;
    float BoostPressure;
    float CurrentBoostPressure;
    float WastegatePosition;
    float BovPosition;
    float TurbineInletTemp;
};
