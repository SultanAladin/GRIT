#pragma once
#include "CoreMinimal.h"

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                              ⚡ ELECTRIC MOTOR SYSTEM
//----------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                  🧩 Electric Motor Specifications (Immutable)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FElectricMotorSpecifications
{
    FElectricMotorSpecifications(int32 PresetID = 1)
    {
        MotorInertia = 0.08f;
        MotorDampingCoeff = 0.01f;
        MotorEfficiency = 0.92f;
        RegenEfficiency = 0.85f;
        MaxRegenTorque = 400.0f;
        
        ConfigureMotor(PresetID);
    }

    /* ================================================= PHYSICAL PROPERTIES ================================================= */
    
    float MotorInertia;                                 // [kg·m²] - Rotor rotational inertia
    float MotorDampingCoeff;                            // [N·m·s·rad⁻¹] - Parasitic drag
    
    /* ================================================= EFFICIENCY ================================================= */
    
    float MotorEfficiency;                              // [0-1] - Electrical to mechanical conversion
    float RegenEfficiency;                              // [0-1] - Mechanical to electrical conversion
    
    /* ================================================= PERFORMANCE ================================================= */
    
    float MaxRegenTorque;                               // [N·m] - Max regen braking force
    TArray<FVector2D> MotorTorqueCurve;                 // [RPM, N·m] - Motor performance envelope
    
    /* ================================================= CONFIGURATION ================================================= */
    
    void ConfigureMotor(int32 InPresetID)
    {
        if (InPresetID == 1) // Reason: preset 1 configuration check
        {
            MotorInertia = 0.08f;
            MotorDampingCoeff = 0.01f;
            MotorEfficiency = 0.92f;
            RegenEfficiency = 0.85f;
            MaxRegenTorque = 400.0f;
            
            MotorTorqueCurve = {FVector2D(0.0f, 600.0f), FVector2D(4000.0f, 600.0f), FVector2D(8000.0f, 550.0f), FVector2D(14000.0f, 300.0f)};
            
            TraceConfiguration();
        } // End if (preset 1 configuration)
    }
    
    /* ================================================= TORQUE EVALUATION ================================================= */
    
    float EvalMotorTorque(float RPM) const
    {
        if (MotorTorqueCurve.Num() < 2) return 0.0f; // Reason: handle empty curve
        if (RPM <= MotorTorqueCurve[0].X) return MotorTorqueCurve[0].Y; // Reason: clamp to curve bounds
        if (RPM >= MotorTorqueCurve.Last().X) return MotorTorqueCurve.Last().Y; // Reason: clamp to curve bounds

        // Reason: binary search for segment
        int32 Low = 0;
        int32 High = MotorTorqueCurve.Num() - 1;
        while (High - Low > 1)
        {
            int32 Mid = (Low + High) / 2;
            if (RPM < MotorTorqueCurve[Mid].X) High = Mid;
            else Low = Mid;
        } // End while (binary search)

        float RPM0 = MotorTorqueCurve[Low].X;           // [rev·min⁻¹]
        float RPM1 = MotorTorqueCurve[High].X;          // [rev·min⁻¹]
        float Torque0 = MotorTorqueCurve[Low].Y;        // [N·m]
        float Torque1 = MotorTorqueCurve[High].Y;       // [N·m]
        float Alpha = (RPM - RPM0) / (RPM1 - RPM0);     // [-]
        
        return FMath::Lerp(Torque0, Torque1, Alpha);    // [N·m]
    }
    
    /* ================================================= DEBUGGING ================================================= */
    
    void TraceConfiguration() const
    {
        UE_LOG(LogTemp, Log, TEXT("--- Electric Motor Configuration ---"));
        UE_LOG(LogTemp, Log, TEXT("Inertia: %.3f kg·m² | Damping: %.4f N·m·s·rad^-1"), MotorInertia, MotorDampingCoeff);
        UE_LOG(LogTemp, Log, TEXT("Motor Efficiency: %.2f | Regen Efficiency: %.2f"), MotorEfficiency, RegenEfficiency);
        UE_LOG(LogTemp, Log, TEXT("Max Regen Torque: %.1f N·m | Curve Points: %d"), MaxRegenTorque, MotorTorqueCurve.Num());
        UE_LOG(LogTemp, Log, TEXT("------------------------------------"));
    }
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                  ⚡ Motor Runtime State (Mutable)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FElectricMotorStateVector
{
    float MotorRPM = 0.0f;                              // [rev·min⁻¹] - Electric motor speed
    float MotorTorque = 0.0f;                           // [N·m] - Motor output torque
    float PowerDraw = 0.0f;                             // [W] - Current electrical power draw
    bool bMotorActive = false;                          // [-] - Motor active state
    
    FElectricMotorStateVector() : MotorRPM(0.0f), MotorTorque(0.0f), PowerDraw(0.0f), bMotorActive(false) {}
};