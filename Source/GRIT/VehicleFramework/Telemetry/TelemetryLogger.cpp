#include "TelemetryLogger.h"

#if !UE_BUILD_SHIPPING

#include "../VehicleSolver.h"

//====================================================================================================================================================
//                                              TELEMETRY HELPER IMPLEMENTATION
//====================================================================================================================================================

void FTelemetryHelper::PopulateSample(
    FTelemetrySample& Sample,
    float SimTime,
    float DeltaTime,
    const FInstantaneousVehicleRecord& Rec,
    const FInputTensor& Input,
    const FEngineStateVector& EngineState,
    const FTransmissionStateVector& TransState,
    const FClutchStateVector& ClutchState,
    const FVehicleSolverAxleData_PT& AxleData,
    const TArray<FBrakingStateVector>& BrakeStates,
    float BoostRatio,
    float TurboRPM,
    float CoolantTemp_K,
    float OilTemp_K,
    const TArray<float>& StaticLoads
)
{
    //--------------------------------------------------------------------------
    // TIMING
    //--------------------------------------------------------------------------
    Sample.Time_s = SimTime;
    Sample.DeltaTime_s = DeltaTime;

    //--------------------------------------------------------------------------
    // ENGINE STATE
    //--------------------------------------------------------------------------
    Sample.EngineRPM = EngineState.CurrentEngineRPM;
    Sample.EngineTorque_Nm = EngineState.CurrentTorqueOutput;
    Sample.EngineLoad_Nm = EngineState.EngineLoadTorque;
    Sample.EngineBraking_Nm = EngineState.EngineBrakingTorque;
    
    // Calculate power: P = T × ω
    const float EngineOmega = EngineState.CurrentEngineRPM * (2.0f * PI / 60.0f);
    Sample.EnginePower_kW = (EngineState.CurrentTorqueOutput * EngineOmega) / 1000.0f;

    //--------------------------------------------------------------------------
    // THERMAL STATE
    //--------------------------------------------------------------------------
    Sample.CoolantTemp_K = CoolantTemp_K;
    Sample.OilTemp_K = OilTemp_K;

    //--------------------------------------------------------------------------
    // TURBO STATE
    //--------------------------------------------------------------------------
    Sample.BoostRatio = BoostRatio;
    Sample.TurboShaftRPM = TurboRPM;
    Sample.ManifoldPressure_kPa = 101.325f * BoostRatio;


    //--------------------------------------------------------------------------
    // VEHICLE KINEMATICS
    //--------------------------------------------------------------------------
    const float Speed_ms = Rec.ν_magnitudeMs;
    Sample.Speed_kmh = Speed_ms * 3.6f;
    
    Sample.ForwardSpeed_kmh = FVector::DotProduct(Rec.ν_linearMs, Rec.ê_longitudinal) * 3.6f;
    Sample.LateralSpeed_kmh = FVector::DotProduct(Rec.ν_linearMs, Rec.ê_lateral) * 3.6f;
    
    // Acceleration in G's
    Sample.Acceleration_G = Rec.α_g_longitudinal;
    Sample.LateralAccel_G = Rec.α_g_lateral;

    //--------------------------------------------------------------------------
    // TRANSMISSION STATE
    //--------------------------------------------------------------------------
    Sample.GearCurrent = TransState.CurrentGear;
    Sample.GearTarget = TransState.TargetGear;
    Sample.bIsShifting = TransState.bIsShifting;
    Sample.TxCombinedRatio = TransState.CombinedGearRatio;

    //--------------------------------------------------------------------------
    // CLUTCH STATE
    //--------------------------------------------------------------------------
    Sample.ClutchTorque_Nm = ClutchState.TorqueTransferred;
    Sample.ClutchEngagement = ClutchState.ClutchEngagement;
    Sample.ClutchLockup = ClutchState.LockupRatio;
    Sample.SlipRPM = ClutchState.SlipRPM;

    //--------------------------------------------------------------------------
    // INPUT STATE
    //--------------------------------------------------------------------------
    Sample.Throttle = Input.Throttle;
    Sample.Brake = Input.Brake;
    Sample.Steering = Input.Steering;
    Sample.Handbrake = Input.Handbrake;

    //--------------------------------------------------------------------------
    // PER-WHEEL DATA WITH LOAD TRANSFER
    //--------------------------------------------------------------------------
    const int32 WheelCount = FMath::Min(AxleData.AxleIDs.Num(), 4);
    
    for (int32 i = 0; i < WheelCount; ++i)
    {
        // Drive and brake torques
        Sample.Wheel_DriveTorque_Nm[i] = AxleData.DriveTorquesNm[i];
        Sample.Wheel_BrakeTorque_Nm[i] = AxleData.BrakeTorquesNm[i];
        
        // Kinematics
        Sample.Wheel_Omega_rad_s[i] = AxleData.AngularVelocities[i];
        Sample.Wheel_Steer_deg[i] = FMath::RadiansToDegrees(AxleData.SteerAnglesRad[i]);
        
        // Slip values
        Sample.Wheel_SlipRatio[i] = AxleData.LongitudinalSlips[i];
        Sample.Wheel_SlipAngle_deg[i] = FMath::RadiansToDegrees(AxleData.SlipAnglesRad[i]);
        
        // Load with transfer calculation
        Sample.Wheel_Load_N[i] = AxleData.WheelLoads[i];
        
        // Static load and load transfer
        if (StaticLoads.IsValidIndex(i))
        {
            Sample.Wheel_StaticLoad_N[i] = StaticLoads[i];
            Sample.Wheel_LoadTransfer_N[i] = AxleData.WheelLoads[i] - StaticLoads[i];
        }
        else
        {
            // Estimate static load as 1/4 of total weight if not provided
            const float EstimatedStaticLoad = Rec.μ_mass * 9.81f / 4.0f;
            Sample.Wheel_StaticLoad_N[i] = EstimatedStaticLoad;
            Sample.Wheel_LoadTransfer_N[i] = AxleData.WheelLoads[i] - EstimatedStaticLoad;
        }
        
        // Forces
        Sample.Wheel_Fx_N[i] = AxleData.LongitudinalForces[i];
        Sample.Wheel_Fy_N[i] = AxleData.LateralForces[i];
        
        // Self-aligning torque and pneumatic trail
        if (AxleData.SelfAligningTorques.IsValidIndex(i))
        {
            Sample.Wheel_SelfAlignTorque_Nm[i] = AxleData.SelfAligningTorques[i];
            
            // Pneumatic trail: t = Mz / Fy
            if (FMath::Abs(AxleData.LateralForces[i]) > 100.0f)
            {
                Sample.Wheel_PneumaticTrail_m[i] = AxleData.SelfAligningTorques[i] / AxleData.LateralForces[i];
            }
        }
        
        // Contact state
        Sample.Wheel_InContact[i] = AxleData.bIsInContact[i];
        
        // Suspension state
        Sample.Susp_Displacement_cm[i] = AxleData.SpringDisplacements[i];
        Sample.Susp_Velocity_cm_s[i] = AxleData.SpringVelocities[i];
        Sample.Susp_Force_N[i] = AxleData.SpringForces[i];
    }


    //--------------------------------------------------------------------------
    // BRAKE THERMAL STATE
    //--------------------------------------------------------------------------
    for (int32 i = 0; i < WheelCount && i < BrakeStates.Num(); ++i)
    {
        Sample.Brake_Pressure_Pa[i] = BrakeStates[i].CurrentPressure;
        Sample.Brake_Temp_K[i] = BrakeStates[i].CurrentTemperature;
        Sample.Brake_FrictionCoeff[i] = BrakeStates[i].CurrentFrictionCoeff;
    }

    //--------------------------------------------------------------------------
    // EVENT FLAGS
    //--------------------------------------------------------------------------
    // Gear change event detection
    static int32 PrevGear = Sample.GearCurrent;
    Sample.GearEvent = (Sample.GearCurrent != PrevGear) ? Sample.GearCurrent : 0;
    PrevGear = Sample.GearCurrent;
    
    // Clutch event detection
    static bool PrevClutchLocked = false;
    bool CurrentClutchLocked = (Sample.ClutchLockup > 0.5f);
    Sample.ClutchEvent = (CurrentClutchLocked != PrevClutchLocked) ? (CurrentClutchLocked ? 1 : -1) : 0;
    PrevClutchLocked = CurrentClutchLocked;
    
    // RPM band indicator
    if (Sample.EngineRPM < 2000.0f) Sample.RpmBand = 1;
    else if (Sample.EngineRPM < 4000.0f) Sample.RpmBand = 2;
    else if (Sample.EngineRPM < 6000.0f) Sample.RpmBand = 3;
    else Sample.RpmBand = 4;
}

#endif // !UE_BUILD_SHIPPING
