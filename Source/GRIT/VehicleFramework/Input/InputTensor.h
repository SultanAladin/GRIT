#pragma once

#include "CoreMinimal.h"
#include "InputTensor.generated.h"

//------------------------------------------------------------------------------
// 🧩 Input Device Types
//------------------------------------------------------------------------------

/** Supported input device types */
UENUM(BlueprintType)
enum class EVehicleInputDevice : uint8
{
    None     UMETA(DisplayName = "None"),
    Keyboard UMETA(DisplayName = "Keyboard"),
    Gamepad  UMETA(DisplayName = "Gamepad"),
    Wheel    UMETA(DisplayName = "Steering Wheel")
};

//------------------------------------------------------------------------------
// 🧩 Simple Input Tensor
//------------------------------------------------------------------------------

/** FInputTensor - Final processed input snapshot for physics thread */
USTRUCT(BlueprintType)
struct GRIT_API FInputTensor
{
    GENERATED_BODY()

    //------------------------------------------------------------------------------
    // 🧩 Core Analog Inputs
    //------------------------------------------------------------------------------

    /** Throttle analog (0.0 = none, 1.0 = full) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Analog")
    float Throttle = 0.0f; // [-] (unitless)

    /** Brake analog (0.0 = none, 1.0 = full) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Analog")
    float Brake = 0.0f; // [-] (unitless)

    /** Reverse request pulse (set by double-tapping brake input) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Transmission")
    bool bReverseRequest = false;

    /** Steering analog (-1.0 left .. 0 .. 1.0 right) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Analog")
    float Steering = 0.0f; // [-] (unitless)

    /** Handbrake analog (0.0..1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    float Handbrake = 0.0f; // [-] (unitless)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    bool ShiftUp = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    bool ShiftDown = false;

    /** Clutch pedal input (0=engaged, 1=disengaged) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Transmission")
    float Clutch = 0.0f; // [-] (unitless)

    //------------------------------------------------------------------------------
    // 🧩 Transmission Digital Inputs
    //------------------------------------------------------------------------------

    /** Manual gear up command */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Transmission")
    bool bGearUp = false;

    /** Manual gear down command */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Transmission")
    bool bGearDown = false;

    /** Toggle between automatic/manual mode */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Transmission")
    bool bTransModeToggle = false;

    //------------------------------------------------------------------------------
    // 🧩 Engine Control Inputs
    //------------------------------------------------------------------------------

    /** Engine start/stop toggle */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Engine")
    bool bEngineToggle = false;

    /** Engine boost/turbo button */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Engine")
    bool bBoost = false;

    /** Context-sensitive performance trigger (N->1st when neutral, boost when in gear) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Performance")
    bool bOverDrive = false;

    //------------------------------------------------------------------------------
    // 🧩 Differential & Traction Inputs
    //------------------------------------------------------------------------------

    /** Cycle differential lock mode */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Differential")
    bool bDiffLockToggle = false;

    /** Toggle traction control system */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|TCS")
    bool bTCSToggle = false;

    /** Toggle anti-lock braking system */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|ABS")
    bool bABSToggle = false;

    /** Toggle stability control */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Stability")
    bool bStabilityToggle = false;

    //------------------------------------------------------------------------------
    // 🧩 Advanced Vehicle Inputs
    //------------------------------------------------------------------------------

    /** Launch control activation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Advanced")
    bool bLaunchControl = false;

    /** Drift mode toggle */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Advanced")
    bool bDriftMode = false;

    /** Reset vehicle to upright position */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Advanced")
    bool bResetVehicle = false;

    /** Aerodynamic braking (air brake) activation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Advanced")
    bool bAerodynamicBraking = false;

    //------------------------------------------------------------------------------
    // 🧩 Input Device Metadata
    //------------------------------------------------------------------------------

    /** Current active input device type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Device")
    EVehicleInputDevice DeviceType = EVehicleInputDevice::None;

    /** Raw input magnitude for device detection [0-1] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Device")
    float InputMagnitude = 0.0f; // [-] (dimensionless)

    /** Frame timestamp when input was captured [s] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Device")
    float Timestamp = 0.0f; // [s]

    //------------------------------------------------------------------------------
    // 🧩 Constructors
    //------------------------------------------------------------------------------

    FInputTensor()
        : Throttle(0.0f)
        , Brake(0.0f)
        , bReverseRequest(false)
        , Steering(0.0f)
        , Handbrake(0.0f)
        , ShiftUp(false)
        , ShiftDown(false)
        , Clutch(0.0f)
        , bGearUp(false)
        , bGearDown(false)
        , bTransModeToggle(false)
        , bEngineToggle(false)
        , bBoost(false)
        , bOverDrive(false)  // <-- ADD THIS
        , bDiffLockToggle(false)
        , bTCSToggle(false)
        , bABSToggle(false)
        , bStabilityToggle(false)
        , bLaunchControl(false)
        , bDriftMode(false)
        , bResetVehicle(false)
        , bAerodynamicBraking(false)
        , DeviceType(EVehicleInputDevice::None)
        , InputMagnitude(0.0f)
        , Timestamp(0.0f)
    {
    }

    //------------------------------------------------------------------------------
    // 🧩 Utilities
    //------------------------------------------------------------------------------

    /** Reset all inputs to default state */
    void FlushInputs()
    {
    Throttle = 0.0f;
    Brake = 0.0f;
    bReverseRequest = false;
    Steering = 0.0f;
    Handbrake = 0.0f;
    Clutch = 0.0f;
    bGearUp = false;
    bGearDown = false;
    bTransModeToggle = false;
    bEngineToggle = false;
    bBoost = false;
    bOverDrive = false;
    bDiffLockToggle = false;
    bTCSToggle = false;
    bABSToggle = false;
    bStabilityToggle = false;
    bLaunchControl = false;
    bDriftMode = false;
    bResetVehicle = false;
    bAerodynamicBraking = false;
    InputMagnitude = 0.0f;
    }

    bool NearlyEquals(const FInputTensor& Other, float AnalogTolerance = 1.e-3f) const
    {
        const auto AnalogEqual = [AnalogTolerance](float A, float B)
        {
            return FMath::IsNearlyEqual(A, B, AnalogTolerance);
        };

        return AnalogEqual(Throttle, Other.Throttle)
            && AnalogEqual(Brake, Other.Brake)
            && bReverseRequest == Other.bReverseRequest
            && AnalogEqual(Steering, Other.Steering)
            && AnalogEqual(Handbrake, Other.Handbrake)
            && ShiftUp == Other.ShiftUp
            && ShiftDown == Other.ShiftDown
            && AnalogEqual(Clutch, Other.Clutch)
            && bGearUp == Other.bGearUp
            && bGearDown == Other.bGearDown
            && bTransModeToggle == Other.bTransModeToggle
            && bEngineToggle == Other.bEngineToggle
            && bBoost == Other.bBoost
            && bOverDrive == Other.bOverDrive
            && bDiffLockToggle == Other.bDiffLockToggle
            && bTCSToggle == Other.bTCSToggle
            && bABSToggle == Other.bABSToggle
            && bStabilityToggle == Other.bStabilityToggle
            && bLaunchControl == Other.bLaunchControl
            && bDriftMode == Other.bDriftMode
            && bResetVehicle == Other.bResetVehicle
            && bAerodynamicBraking == Other.bAerodynamicBraking
            && DeviceType == Other.DeviceType
            && AnalogEqual(InputMagnitude, Other.InputMagnitude);
    }
};

//------------------------------------------------------------------------------
// 🧩 Input Configuration
//------------------------------------------------------------------------------

/** FVehicleInputConfig - Configuration for input sensitivity and dead zones */
USTRUCT(BlueprintType)
struct GRIT_API FVehicleInputConfig
{
    GENERATED_BODY()

    //------------------------------------------------------------------------------
    // 🧩 Sensitivity Settings
    //------------------------------------------------------------------------------

    /** Steering sensitivity multiplier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Sensitivity", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float SteeringSensitivity = 1.0f; // [-] (dimensionless)

    /** Throttle sensitivity multiplier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Sensitivity", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float ThrottleSensitivity = 1.0f; // [-] (dimensionless)

    /** Brake sensitivity multiplier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Sensitivity", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float BrakeSensitivity = 1.0f; // [-] (dimensionless)

    //------------------------------------------------------------------------------
    // 🧩 Dead Zone Settings
    //------------------------------------------------------------------------------

    /** Steering dead zone */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|DeadZones", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float SteeringDeadZone = 0.05f; // [-] (dimensionless)

    /** Throttle dead zone */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|DeadZones", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float ThrottleDeadZone = 0.02f; // [-] (dimensionless)

    /** Brake dead zone */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|DeadZones", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float BrakeDeadZone = 0.02f; // [-] (dimensionless)

    //------------------------------------------------------------------------------
    // 🧩 Device Detection Settings
    //------------------------------------------------------------------------------

    /** Current active input device */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Device")
    EVehicleInputDevice ActiveInputDevice = EVehicleInputDevice::Keyboard;

    /** Enable automatic device switching */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Device")
    bool bAutoSwitchInputDevice = true;

    /** Minimum input magnitude to trigger device switch [0-1] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Device", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DeviceSwitchThreshold = 0.1f; // [-] (dimensionless)

    /** Time window for device detection [s] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Device", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float DeviceDetectionWindow = 0.5f; // [s]

    //------------------------------------------------------------------------------
    // 🧩 Steering Wheel Specific Settings
    //------------------------------------------------------------------------------

    /** Force feedback strength for steering wheels */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Wheel", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FFBStrength = 0.7f; // [-] (dimensionless)

    /** Steering wheel rotation range [deg] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Wheel", meta = (ClampMin = "90.0", ClampMax = "1080.0"))
    float WheelRotationRange = 900.0f; // [deg]

    FVehicleInputConfig()
        : SteeringSensitivity(1.0f)
        , ThrottleSensitivity(1.0f)
        , BrakeSensitivity(1.0f)
        , SteeringDeadZone(0.05f)
        , ThrottleDeadZone(0.02f)
        , BrakeDeadZone(0.02f)
        , ActiveInputDevice(EVehicleInputDevice::Keyboard)
        , bAutoSwitchInputDevice(true)
        , DeviceSwitchThreshold(0.1f)
        , DeviceDetectionWindow(0.5f)
        , FFBStrength(0.7f)
        , WheelRotationRange(900.0f)
    {
    }
};

// ⚠️ Inconsistency detected: `QUBIT_API` export macro may not exist in Project Anvil modules; confirm or replace with appropriate module API specifier.
