#pragma once

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

/*====================================================================================================================================
                                                     VEHICLE INPUT ORCHESTRATOR
======================================================================================================================================*/
UCLASS(BlueprintType, Blueprintable)
class GRIT_API AVehicleController : public APlayerController
{
    GENERATED_BODY()

public:
    AVehicleController();

protected:
    /** Core lifecycle initialization */
    virtual void BeginPlay() override;
    
    /** Frame update with input processing */
    virtual void Tick(float DeltaTime) override;
    
    /** Enhanced input component binding */
    virtual void SetupInputComponent() override;
    
    /** Vehicle possession with network owner assignment */
    virtual void OnPossess(APawn* InPawn) override;
    
    /** Release vehicle and write telemetry */
    virtual void OnUnPossess() override;
    
    /** Client-side pawn replication callback */
    virtual void OnRep_Pawn() override;

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
    //                           network replication
    //------------------------------------------------------------------------------
    /** Transmit input tensor to server for authoritative processing */
    UFUNCTION(Server, Unreliable, WithValidation)
    void ServerSendInput(FInputTensor NewInput);
    
    /** RPC validation guard */
    bool ServerSendInput_Validate(FInputTensor NewInput);

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
    float ThrottleRate = 3.0f;           // [s⁻¹] - Throttle interpolation rate

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Smoothing")
    float BrakeRate = 4.0f;              // [s⁻¹] - Brake interpolation rate

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Smoothing")
    float SteeringRate = 2.5f;           // [s⁻¹] - Steering interpolation rate

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Smoothing")
    float HandbrakeRate = 6.0f;          // [s⁻¹] - Handbrake interpolation rate

    //------------------------------------------------------------------------------
    //                                   debug flags
    //------------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowInputDebug = false;        // [-] - Enable input visualization

private:
    bool bConnectionEstablished = false; // [-] - Vehicle link status
    
    FInputTensor PendingInput;           // [-] - Local input state awaiting transmission
    FInputTensor LastSentInput;          // [-] - Previous transmitted input for delta compression
    bool bInputDirty = false;            // [-] - Input modification flag

    float TargetThrottle = 0.0f;         // [0..1] - Desired throttle position
    float TargetBrake = 0.0f;            // [0..1] - Desired brake pressure
    float TargetSteering = 0.0f;         // [-1..1] - Desired steering angle
    float TargetHandbrake = 0.0f;        // [0..1] - Desired handbrake pressure

    /** Initialize vehicle reference connection */
    void EstablishVehicleConnection();
    
    /** Smooth analog inputs with conflict resolution */
    void UpdateAnalogInputs(float DeltaTime);
    
    /** Render input state to screen */
    void DisplayInputDiagnostics() const;
};