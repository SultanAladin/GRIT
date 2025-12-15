#pragma once

#include "CoreMinimal.h"
#include "SteeringAssembly.generated.h"

//------------------------------------------------------------------------------
// 🧩 Steering Configuration Types
//------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ESteeringConfiguration : uint8
{
    FrontAckerman     UMETA(DisplayName = "Front Ackerman"),
    RearAckerman      UMETA(DisplayName = "Rear Ackerman"),
    FourWheelParallel UMETA(DisplayName = "Four-Wheel Parallel"),
    FourWheelAckerman UMETA(DisplayName = "Four-Wheel Ackerman"),
    CrabSteer         UMETA(DisplayName = "Crab Steer")
};

UENUM(BlueprintType)
enum class ESteerType : uint8
{
    SingleAngle UMETA(DisplayName = "Single Angle"),
    AngleRatio  UMETA(DisplayName = "Angle Ratio"),
    Ackermann   UMETA(DisplayName = "Ackermann")
};

//------------------------------------------------------------------------------
// 🧩 Steering Assembly (UPDATED WITH SPEED-SENSITIVE FEATURES)
//------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FSteeringAssembly
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Setup")
    float MaxSteeringAngle = 35.0f; // [deg] - Maximum steering angle

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Setup")
    float MaxSteerRate = 90.0f; // [deg/s] - Maximum steering rate

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Setup")
    ESteeringConfiguration SteeringGeometry = ESteeringConfiguration::FrontAckerman;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Setup")
    ESteerType SteerType = ESteerType::Ackermann;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Steering")
    float CurrentSteeringAngle = 0.0f; // [deg] - Current steering angle
    
    //------------------------------------------------------------------------------
    // LAYER 1: Variable Steering Ratio (Speed-Sensitive Power Steering)
    //------------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Variable Ratio", meta = (DisplayName = "Enable Variable Ratio"))
    bool bEnableVariableRatio = true; // [-] - Toggle speed-sensitive steering ratio
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Variable Ratio", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
    float VariableRatioStartSpeed = 30.0f; // [km/h] - Speed where ratio reduction begins
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Variable Ratio", meta = (ClampMin = "50.0", ClampMax = "300.0", UIMin = "50.0", UIMax = "300.0"))
    float VariableRatioMaxSpeed = 150.0f; // [km/h] - Speed where ratio reaches minimum
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Variable Ratio", meta = (ClampMin = "0.2", ClampMax = "1.0", UIMin = "0.2", UIMax = "1.0"))
    float VariableRatioMinFactor = 0.4f; // [-] - Minimum steering sensitivity at max speed (0.4 = 40% sensitivity)
    
    //------------------------------------------------------------------------------
    // LAYER 2: Steering Damping (Hydraulic Column Resistance)
    //------------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Damping", meta = (DisplayName = "Enable Steering Damping"))
    bool bEnableSteeringDamping = true; // [-] - Toggle velocity-dependent damping
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Damping", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float DampingCoeffLowSpeed = 0.05f; // [-] - Damping coefficient at low speed (light resistance)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Damping", meta = (ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "2.0"))
    float DampingCoeffHighSpeed = 0.25f; // [-] - Damping coefficient at high speed (heavy resistance)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Damping", meta = (ClampMin = "50.0", ClampMax = "300.0", UIMin = "50.0", UIMax = "300.0"))
    float DampingReferenceSpeed = 150.0f; // [km/h] - Reference speed for damping interpolation
    
    //------------------------------------------------------------------------------
    // LAYER 3: Yaw Stability Control (ESC Integration)
    //------------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Stability", meta = (DisplayName = "Enable Stability Control"))
    bool bEnableStabilityControl = true; // [-] - Toggle slip angle-based steering limiter
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Stability", meta = (ClampMin = "3.0", ClampMax = "15.0", UIMin = "3.0", UIMax = "15.0"))
    float CriticalSlipAngle = 10.0f; // [deg] - Critical slip angle (tire-dependent, 6-8° economy, 10-12° sports)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Stability", meta = (ClampMin = "0.1", ClampMax = "1.0", UIMin = "0.1", UIMax = "1.0"))
    float StabilityMinAuthority = 0.3f; // [-] - Minimum steering authority at critical slip (0.3 = 30% control)
};