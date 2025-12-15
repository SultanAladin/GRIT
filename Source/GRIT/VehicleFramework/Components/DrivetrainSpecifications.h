#pragma once
#include "CoreMinimal.h"
#include "EngineSpecifications.h"
#include "TurbochargerSpecifications.h"
#include "SuperchargerSpecifications.h"
#include "ElectricMotorSpecifications.h"
#include "BatterySpecifications.h"
#include "EngineOilSpecifications.h"
#include "ClutchSpecifications.h"
#include "TransmissionSpecifications.h"
#include "DifferentialSpecifications.h"
#include "FuelControlSystem.h"

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                              🔧 DRIVETRAIN SYSTEM
//----------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                  🧩 Drivetrain Specifications (Immutable)
//----------------------------------------------------------------------------------------------------------------------------------------

struct FDrivetrainSpecifications 
{
    FDrivetrainSpecifications(int32 PresetID = 1) 
    { 
        DrivetrainEfficiency = 0.96f;
        PowerLossCoefficient = 0.04f;
        ConfigureDrivetrain(PresetID);
    }

    /* ================================================= DRIVETRAIN COMPONENTS ================================================= */
    
    FEngineSpecifications    Engine;
    FEngineOilSpecifications EngineOil;
    FTurbochargerSpecifications Turbocharger;
    FSuperchargerSpecifications Supercharger;
    FElectricMotorSpecifications ElectricMotor;
    FBatterySpecifications Battery;
    FClutchSpecifications Clutch;
    FTransmissionSpecifications Transmission;
    FDifferentialSpecifications CenterDifferential;
    FDifferentialSpecifications FrontDifferential;
    FDifferentialSpecifications RearDifferential;
    FFuelControlSystem FuelSystem;
    
    /* ================================================= EFFICIENCY ================================================= */
    
    float DrivetrainEfficiency;                         // [-] - Overall drivetrain efficiency factor
    float PowerLossCoefficient;                         // [-] - Power loss factor (1 - efficiency)
    
    /* ================================================= CONFIGURATION ================================================= */
    
    void ConfigureDrivetrain(int32 PresetID)
    {
        if (PresetID == 1) // Reason: preset 1 full drivetrain configuration
        {
            Engine.ConfigureEngine(PresetID);
            EngineOil.ConfigureOil(PresetID);
            Turbocharger.ConfigureTurbo(PresetID);
            Supercharger.ConfigureSupercharger(PresetID);
            ElectricMotor.ConfigureMotor(PresetID);
            Battery.ConfigureBattery(PresetID);
            Clutch.ConfigureClutch(PresetID);
            Transmission.ConfigureTransmission(PresetID);
            CenterDifferential.ConfigureDifferential(1); // Preset 1 = Center diff
            FrontDifferential.ConfigureDifferential(2);  // Preset 2 = Front axle LSD
            RearDifferential.ConfigureDifferential(3);   // Preset 3 = Rear axle LSD
            FuelSystem.ConfigureSystem(PresetID);
            
            DrivetrainEfficiency = 0.96f;
            PowerLossCoefficient = 0.04f;
            
            TraceConfiguration();
        } // End if (preset 1 configuration)
    }
    
    /* ================================================= TORQUE CALCULATIONS ================================================= */
    
    FORCEINLINE float CalcEffectiveTorque(float InputTorque) const { return InputTorque * DrivetrainEfficiency; }
    FORCEINLINE float CalcPowerLoss(float Torque, float RPM) const { return (Torque * RPM * 2.0f * PI / 60.0f) * PowerLossCoefficient; }
    
    /* ================================================= DEBUGGING ================================================= */
    
    void TraceConfiguration() const
    {
        UE_LOG(LogTemp, Log, TEXT("--- Drivetrain Configuration ---"));
        UE_LOG(LogTemp, Log, TEXT("Efficiency: %.3f | Power Loss: %.3f%%"), DrivetrainEfficiency, PowerLossCoefficient * 100.0f);
        UE_LOG(LogTemp, Log, TEXT("--------------------------------"));
    }
};

//----------------------------------------------------------------------------------------------------------------------------------------
//                                                  ⚡ Drivetrain Runtime State (Mutable)
//----------------------------------------------------------------------------------------------------------------------------------------

/** Drivetrain state vector - Runtime state for complete drivetrain operation (physics thread only) */
struct FDrivetrainStateVector
{
    FEngineStateVector    EngineState;
    FEngineOilStateVector EngineOilState;
    FTurbochargerStateVector TurboState;
    FSuperchargerStateVector SuperchargerState;
    FElectricMotorStateVector MotorState;
    FBatteryStateVector BatteryState;
    FClutchStateVector ClutchState;
    FTransmissionStateVector TransmissionState;
    FDifferentialStateVector CenterDiffState;
    FDifferentialStateVector FrontDiffState;
    FDifferentialStateVector RearDiffState;
    FFuelSystemStateVector FuelSystemState;
    
    float TotalPowerOutput = 0.0f;                      // [kW] - Total drivetrain power output
    float TotalTorqueOutput = 0.0f;                     // [N·m] - Total drivetrain torque output
    float PowerLoss = 0.0f;                             // [kW] - Current power loss
    float DrivetrainTemperature = 293.15f;              // [K] - Average drivetrain temperature
    bool bDrivetrainActive = false;                     // [-] - Drivetrain active state

    FDrivetrainStateVector() = default;
};