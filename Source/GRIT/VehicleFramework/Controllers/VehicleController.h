#pragma once

//------------------------------------------------------------------------------
//                              vehicle input orchestrator
//------------------------------------------------------------------------------
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "Input/InputTensor.h"
#include "VehicleController.generated.h"

class UInputMappingContext;
class UInputAction;
class AVehicleSolver;

//------------------------------------------------------------------------------
//                                 log category
//------------------------------------------------------------------------------
DECLARE_LOG_CATEGORY_EXTERN(LogVehicleController, Log, All);

//------------------------------------------------------------------------------
//                                   controller
//------------------------------------------------------------------------------
UCLASS(BlueprintType, Blueprintable)
class GRIT_API AVehicleController : public APlayerController
{
    GENERATED_BODY()

public:
    AVehicleController();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupInputComponent() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

    //------------------------------------------------------------------------------
    //                           enhanced input assets
    //------------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Context")
    UInputMappingContext* VehicleInputMappingContext = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Actions")
    UInputAction* IA_Throttle = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Actions")
    UInputAction* IA_Brake = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Actions")
    UInputAction* IA_Steer = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Actions")
    UInputAction* IA_Handbrake = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Actions")
    UInputAction* IA_GearUp = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Actions")
    UInputAction* IA_GearDown = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Actions")
    UInputAction* IA_Clutch = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Actions")
    UInputAction* IA_OverDrive = nullptr;
    //------------------------------------------------------------------------------
    //                                  vehicle link
    //------------------------------------------------------------------------------
    UPROPERTY(BlueprintReadOnly, Category = "Control")
    AVehicleSolver* ControlledVehicle = nullptr;

    //------------------------------------------------------------------------------
    //                                input handlers
    //------------------------------------------------------------------------------
    UFUNCTION()
    void ThrottleTriggered(const FInputActionValue& Value);

    UFUNCTION()
    void ThrottleCompleted(const FInputActionValue& Value);

    UFUNCTION()
    void BrakeTriggered(const FInputActionValue& Value);

    UFUNCTION()
    void BrakeCompleted(const FInputActionValue& Value);

    UFUNCTION()
    void SteerTriggered(const FInputActionValue& Value);

    UFUNCTION()
    void SteerCompleted(const FInputActionValue& Value);

    UFUNCTION()
    void HandbrakeTriggered(const FInputActionValue& Value);

    UFUNCTION()
    void HandbrakeCompleted(const FInputActionValue& Value);

    UFUNCTION()
    void GearUpStarted(const FInputActionValue& Value);

    UFUNCTION()
    void GearUpCompleted(const FInputActionValue& Value);

    UFUNCTION()
    void GearDownStarted(const FInputActionValue& Value);

    UFUNCTION()
    void GearDownCompleted(const FInputActionValue& Value);

    UFUNCTION()
    void ClutchTriggered(const FInputActionValue& Value);

    UFUNCTION()
    void ClutchCompleted(const FInputActionValue& Value);

    UFUNCTION()
    void OverDriveStarted(const FInputActionValue& Value);

    UFUNCTION()
    void OverDriveCompleted(const FInputActionValue& Value);

    
    //------------------------------------------------------------------------------
    //                              signal shaping
    //------------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Smoothing")
    float ThrottleRate = 3.0f;           // [s⁻¹] - Throttle response rate

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Smoothing")
    float BrakeRate = 4.0f;              // [s⁻¹] - Brake response rate

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Smoothing")
    float SteeringRate = 2.5f;           // [s⁻¹] - Steering response rate

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Smoothing")
    float HandbrakeRate = 6.0f;          // [s⁻¹] - Handbrake response rate

    //------------------------------------------------------------------------------
    //                                   debug flags
    //------------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowInputDebug = false;

private:
    bool bConnectionEstablished = false;

    float TargetThrottle = 0.0f;         // [0..1]
    float TargetBrake = 0.0f;            // [0..1]
    float TargetSteering = 0.0f;         // [-1..1]
    float TargetHandbrake = 0.0f;        // [0..1]

    /** Minimum pedal input to count as a tap for mode switching */
    UPROPERTY(EditAnywhere, Category = "Input|Transmission")
    float DriveModeTapThreshold = 0.1f;   // [-] - Minimum pedal input to count as tap (responsive)

    /** Max interval between taps to register as a double-tap */
    UPROPERTY(EditAnywhere, Category = "Input|Transmission")
    float DriveModeDoubleTapWindow = 0.35f; // [s] - Max interval between taps

    // --- Driving Mode State ---

    /** True if the controller has latched reverse pedal mapping */
    bool bIsInReverseMode = false;
    /** True if aerodynamic braking is active while the brake pedal is held */
    bool bIsAeroBraking = false;

    float LastBrakeTapTime = -1.0f;       // [s] - Timestamp of previous brake tap
    bool bBrakeTapHoldArmed = false;      // True when brake input is currently held

    float LastThrottleTapTime = -1.0f;    // [s] - Timestamp of previous throttle tap
    bool bThrottleTapHoldArmed = false;   // True when throttle input is currently held

    void EstablishVehicleConnection();
    void UpdateAnalogInputs(float DeltaTime);
    void DisplayInputDiagnostics() const;
};
