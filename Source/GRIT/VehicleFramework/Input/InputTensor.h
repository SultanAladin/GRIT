#pragma once

#include "CoreMinimal.h"
#include "InputTensor.generated.h"

//------------------------------------------------------------------------------
//                              input device types
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
//                              input flag bitmask
//------------------------------------------------------------------------------

/** Packed digital input flags — single uint32 replicated instead of 17 bools */
enum class EInputFlag : uint32
{
    None               = 0,
    ReverseRequest     = 1 << 0,   // [deprecated] reverse is now reached via sequential ShiftDown past Neutral; bit kept to preserve replicated wire format
    ShiftUp            = 1 << 1,   // Sequential upshift pulse
    ShiftDown          = 1 << 2,   // Sequential downshift pulse
    GearUp             = 1 << 3,   // Manual gear up command
    GearDown           = 1 << 4,   // Manual gear down command
    TransModeToggle    = 1 << 5,   // Auto/manual transmission toggle
    EngineToggle       = 1 << 6,   // Engine start/stop
    Boost              = 1 << 7,   // Nitro / turbo activation
    OverDrive          = 1 << 8,   // Context-sensitive performance trigger
    DiffLockToggle     = 1 << 9,   // Cycle differential lock mode
    TCSToggle          = 1 << 10,  // Traction control toggle
    ABSToggle          = 1 << 11,  // Anti-lock braking toggle
    StabilityToggle    = 1 << 12,  // Stability control toggle
    LaunchControl      = 1 << 13,  // Launch control activation
    DriftMode          = 1 << 14,  // Drift mode toggle
    ResetVehicle       = 1 << 15,  // Reset vehicle to upright
    AerodynamicBraking = 1 << 16,  // Air brake activation
};
ENUM_CLASS_FLAGS(EInputFlag);

//------------------------------------------------------------------------------
//                              input tensor
//------------------------------------------------------------------------------

/** FInputTensor - Final processed input snapshot for physics thread */
USTRUCT(BlueprintType)
struct GRIT_API FInputTensor
{
    GENERATED_BODY()

    //------------------------------------------------------------------------------
    //                              analog inputs
    //------------------------------------------------------------------------------

    /** Throttle analog (0.0 = none, 1.0 = full) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Analog")
    float Throttle = 0.0f; // [-] (unitless)

    /** Brake analog (0.0 = none, 1.0 = full) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Analog")
    float Brake = 0.0f; // [-] (unitless)

    /** Steering analog (-1.0 left .. 0 .. 1.0 right) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Analog")
    float Steering = 0.0f; // [-] (unitless)

    /** Handbrake analog (0.0..1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Analog")
    float Handbrake = 0.0f; // [-] (unitless)

    /** Clutch pedal input (0=engaged, 1=disengaged) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Analog")
    float Clutch = 0.0f; // [-] (unitless)

    //------------------------------------------------------------------------------
    //                          digital input flags
    //------------------------------------------------------------------------------

    /** Packed bitmask of all digital inputs (see EInputFlag) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Flags")
    int32 InputFlags = 0;

    //------------------------------------------------------------------------------
    //                          flag accessors
    //------------------------------------------------------------------------------

    FORCEINLINE bool HasFlag(EInputFlag F) const { return (InputFlags & static_cast<int32>(F)) != 0; }
    FORCEINLINE void SetFlag(EInputFlag F) { InputFlags |= static_cast<int32>(F); }
    FORCEINLINE void ClearFlag(EInputFlag F) { InputFlags &= ~static_cast<int32>(F); }
    FORCEINLINE void SetFlagValue(EInputFlag F, bool bOn) { if (bOn) SetFlag(F); else ClearFlag(F); }

    //------------------------------------------------------------------------------
    //                          device metadata
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
    //                              utilities
    //------------------------------------------------------------------------------

    /** Reset all inputs to default state */
    void FlushInputs()
    {
        Throttle = 0.0f;
        Brake = 0.0f;
        Steering = 0.0f;
        Handbrake = 0.0f;
        Clutch = 0.0f;
        InputFlags = 0;
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
            && AnalogEqual(Steering, Other.Steering)
            && AnalogEqual(Handbrake, Other.Handbrake)
            && AnalogEqual(Clutch, Other.Clutch)
            && InputFlags == Other.InputFlags
            && DeviceType == Other.DeviceType
            && AnalogEqual(InputMagnitude, Other.InputMagnitude);
    }
};

//------------------------------------------------------------------------------
//                          input configuration
//------------------------------------------------------------------------------

/** FVehicleInputConfig - Configuration for input sensitivity and dead zones */
USTRUCT(BlueprintType)
struct GRIT_API FVehicleInputConfig
{
    GENERATED_BODY()

    //------------------------------------------------------------------------------
    //                          sensitivity settings
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
    //                          dead zone settings
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
    //                          device detection settings
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
    //                          steering wheel settings
    //------------------------------------------------------------------------------

    /** Force feedback strength for steering wheels */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Wheel", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FFBStrength = 0.7f; // [-] (dimensionless)

    /** Steering wheel rotation range [deg] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Wheel", meta = (ClampMin = "90.0", ClampMax = "1080.0"))
    float WheelRotationRange = 900.0f; // [deg]
};
