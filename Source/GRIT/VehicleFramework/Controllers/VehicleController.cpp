#include "VehicleController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "VehicleSolver.h"

//------------------------------------------------------------------------------
//                                      logging
//------------------------------------------------------------------------------
DEFINE_LOG_CATEGORY(LogVehicleController);

AVehicleController::AVehicleController()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;
    UE_LOG(LogVehicleController, Log, TEXT("🎮 Controller bootstrapped | %s"), *GetName());
}

void AVehicleController::BeginPlay()
{
    Super::BeginPlay();

    EstablishVehicleConnection();

    if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
        {
            if (VehicleInputMappingContext) // Reason: guard null mapping context
            {
                Subsystem->AddMappingContext(VehicleInputMappingContext, 0);
                UE_LOG(LogVehicleController, Log, TEXT("🕹️ Mapping context injected | %s"), *GetName());
            }
            else
            {
                UE_LOG(LogVehicleController, Warning, TEXT("⚠️ Mapping context null | %s"), *GetName());
            } // End if (mapping context check)
        }
    }
}

void AVehicleController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateAnalogInputs(DeltaTime);

    const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

    if (!bBrakeTapHoldArmed && LastBrakeTapTime > 0.0f && (CurrentTime - LastBrakeTapTime) > DriveModeDoubleTapWindow)
    {
        LastBrakeTapTime = -1.0f;
    }

    if (!bThrottleTapHoldArmed && LastThrottleTapTime > 0.0f && (CurrentTime - LastThrottleTapTime) > DriveModeDoubleTapWindow)
    {
        LastThrottleTapTime = -1.0f;
    }
}

void AVehicleController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EnhancedInput) // Reason: ensure enhanced input availability
    {
        UE_LOG(LogVehicleController, Error, TEXT("⛔ Enhanced input component missing | %s"), *GetName());
        return;
    } // End if (enhanced input guard)

    if (IA_Throttle)
    {
        EnhancedInput->BindAction(IA_Throttle, ETriggerEvent::Triggered, this, &AVehicleController::ThrottleTriggered);
        EnhancedInput->BindAction(IA_Throttle, ETriggerEvent::Completed, this, &AVehicleController::ThrottleCompleted);
    }

    if (IA_Brake)
    {
        EnhancedInput->BindAction(IA_Brake, ETriggerEvent::Triggered, this, &AVehicleController::BrakeTriggered);
        EnhancedInput->BindAction(IA_Brake, ETriggerEvent::Completed, this, &AVehicleController::BrakeCompleted);
    }

    if (IA_Steer)
    {
        EnhancedInput->BindAction(IA_Steer, ETriggerEvent::Triggered, this, &AVehicleController::SteerTriggered);
        EnhancedInput->BindAction(IA_Steer, ETriggerEvent::Completed, this, &AVehicleController::SteerCompleted);
    }

    if (IA_Handbrake)
    {
        EnhancedInput->BindAction(IA_Handbrake, ETriggerEvent::Triggered, this, &AVehicleController::HandbrakeTriggered);
        EnhancedInput->BindAction(IA_Handbrake, ETriggerEvent::Completed, this, &AVehicleController::HandbrakeCompleted);
    }

    if (IA_GearUp)
    {
        EnhancedInput->BindAction(IA_GearUp, ETriggerEvent::Started, this, &AVehicleController::GearUpStarted);
        EnhancedInput->BindAction(IA_GearUp, ETriggerEvent::Completed, this, &AVehicleController::GearUpCompleted);
    }

    if (IA_GearDown)
    {
        EnhancedInput->BindAction(IA_GearDown, ETriggerEvent::Started, this, &AVehicleController::GearDownStarted);
        EnhancedInput->BindAction(IA_GearDown, ETriggerEvent::Completed, this, &AVehicleController::GearDownCompleted);
    }

    if (IA_Clutch)
    {
        EnhancedInput->BindAction(IA_Clutch, ETriggerEvent::Triggered, this, &AVehicleController::ClutchTriggered);
        EnhancedInput->BindAction(IA_Clutch, ETriggerEvent::Completed, this, &AVehicleController::ClutchCompleted);
    }

    if (IA_OverDrive)
    {
        EnhancedInput->BindAction(IA_OverDrive, ETriggerEvent::Started, this, &AVehicleController::OverDriveStarted);
        EnhancedInput->BindAction(IA_OverDrive, ETriggerEvent::Completed, this, &AVehicleController::OverDriveCompleted);
    }
}

void AVehicleController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    ControlledVehicle = Cast<AVehicleSolver>(InPawn);
    if (ControlledVehicle) // Reason: confirm vehicle possession
    {
        bConnectionEstablished = true;
        UE_LOG(LogVehicleController, Log, TEXT("🧩 Vehicle possession secured | Controller: %s | Pawn: %s"), *GetName(), *ControlledVehicle->GetName());
    }
    else
    {
        bConnectionEstablished = false;
        UE_LOG(LogVehicleController, Warning, TEXT("⚠️ Pawn not a VehicleConstruct | Controller: %s"), *GetName());
    } // End if (possession validation)
}

void AVehicleController::OnUnPossess()
{
    bConnectionEstablished = false;
    ControlledVehicle = nullptr;
    Super::OnUnPossess();
    UE_LOG(LogVehicleController, Log, TEXT("🧩 Vehicle released | Controller: %s"), *GetName());
}

void AVehicleController::ThrottleTriggered(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    const float InputValue = FMath::Clamp(Value.Get<float>(), 0.0f, 1.0f);
    TargetThrottle = InputValue;

    const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

    if (InputValue > DriveModeTapThreshold)
    {
        if (!bThrottleTapHoldArmed)
        {
            bThrottleTapHoldArmed = true;

            if (LastThrottleTapTime > 0.0f && (CurrentTime - LastThrottleTapTime) <= DriveModeDoubleTapWindow)
            {
                bIsInReverseMode = false;
                bIsAeroBraking = false;

                if (ControlledVehicle)
                {
                    ControlledVehicle->InputTensor_GameThread.bReverseRequest = false;
                    ControlledVehicle->InputTensor_GameThread.bAerodynamicBraking = false;
                }

                LastThrottleTapTime = -1.0f;
            }
            else
            {
                LastThrottleTapTime = CurrentTime;
            }
        }
    }
}

void AVehicleController::ThrottleCompleted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    TargetThrottle = 0.0f;
    bThrottleTapHoldArmed = false;
}

void AVehicleController::BrakeTriggered(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    const float InputValue = FMath::Clamp(Value.Get<float>(), 0.0f, 1.0f);
    TargetBrake = InputValue;

    const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

    if (InputValue > DriveModeTapThreshold)
    {
        if (!bBrakeTapHoldArmed)
        {
            bBrakeTapHoldArmed = true;

            if (LastBrakeTapTime > 0.0f && (CurrentTime - LastBrakeTapTime) <= DriveModeDoubleTapWindow)
            {
                const float VehicleSpeedCms = ControlledVehicle ? ControlledVehicle->GetVelocity().Size() : 0.0f;
                const float AeroToggleSpeedCms = 10.0f;

                if (VehicleSpeedCms > AeroToggleSpeedCms)
                {
                    bIsAeroBraking = true;
                    bIsInReverseMode = false;

                    if (ControlledVehicle)
                    {
                        ControlledVehicle->InputTensor_GameThread.bAerodynamicBraking = true;
                        ControlledVehicle->InputTensor_GameThread.bReverseRequest = false;
                    }
                }
                else
                {
                    bIsInReverseMode = true;
                    bIsAeroBraking = false;

                    if (ControlledVehicle)
                    {
                        ControlledVehicle->InputTensor_GameThread.bReverseRequest = true;
                        ControlledVehicle->InputTensor_GameThread.bAerodynamicBraking = false;
                    }
                }

                LastBrakeTapTime = -1.0f;
            }
            else
            {
                LastBrakeTapTime = CurrentTime;
            }
        }
    }
}

void AVehicleController::BrakeCompleted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return;

    TargetBrake = 0.0f;
    bBrakeTapHoldArmed = false;

    //==========================================================================
    // 🔥 CRITICAL: Preserve Reverse Mode State
    //==========================================================================
    // When the brake pedal is released, we must NOT exit reverse mode.
    // Reverse mode is a latched state that persists until the user
    // double-taps the throttle to exit.
    //
    // Only the aerodynamic braking flag should be cleared here, as it's
    // a momentary "hold to brake harder" feature that requires sustained input.
    
    if (bIsAeroBraking)
    {
        bIsAeroBraking = false;
        if (ControlledVehicle)
        {
            ControlledVehicle->InputTensor_GameThread.bAerodynamicBraking = false;
        }
    }

    // ✅ bIsInReverseMode is NOT cleared here - it remains latched until
    // the user double-taps throttle to exit reverse mode.
}

void AVehicleController::SteerTriggered(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    TargetSteering = FMath::Clamp(Value.Get<float>(), -1.0f, 1.0f);
}

void AVehicleController::SteerCompleted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    TargetSteering = 0.0f;
}

void AVehicleController::HandbrakeTriggered(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    TargetHandbrake = FMath::Clamp(Value.Get<float>(), 0.0f, 1.0f);
}

void AVehicleController::HandbrakeCompleted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    TargetHandbrake = 0.0f;
}

void AVehicleController::GearUpStarted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    ControlledVehicle->InputTensor_GameThread.ShiftUp = true;
}

void AVehicleController::GearUpCompleted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    ControlledVehicle->InputTensor_GameThread.ShiftUp = false;
}

void AVehicleController::GearDownStarted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    ControlledVehicle->InputTensor_GameThread.ShiftDown = true;
}

void AVehicleController::GearDownCompleted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    ControlledVehicle->InputTensor_GameThread.ShiftDown = false;
}

void AVehicleController::ClutchTriggered(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    ControlledVehicle->InputTensor_GameThread.Clutch = FMath::Clamp(Value.Get<float>(), 0.0f, 1.0f);
}

void AVehicleController::ClutchCompleted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    ControlledVehicle->InputTensor_GameThread.Clutch = 0.0f;
}

void AVehicleController::OverDriveStarted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    ControlledVehicle->InputTensor_GameThread.bOverDrive = true;
}

void AVehicleController::OverDriveCompleted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    ControlledVehicle->InputTensor_GameThread.bOverDrive = false;
}

void AVehicleController::EstablishVehicleConnection()
{
    bConnectionEstablished = false;
}

void AVehicleController::UpdateAnalogInputs(float DeltaTime)
{
    if (!ControlledVehicle) return;

    AVehicleSolver* Vehicle = ControlledVehicle;

    const float CurrentThrottle = Vehicle->InputTensor_GameThread.Throttle;
    const float CurrentBrake = Vehicle->InputTensor_GameThread.Brake;
    const float CurrentSteering = Vehicle->InputTensor_GameThread.Steering;
    const float CurrentHandbrake = Vehicle->InputTensor_GameThread.Handbrake;

    //==========================================================================
    // 🔥 CRITICAL: Input Swap for Reverse Mode
    //==========================================================================
    // When in reverse mode, the brake pedal becomes throttle and vice versa.
    // This mimics real vehicle behavior where you use the "gas" to reverse.
    float DesiredThrottle = TargetThrottle;
    float DesiredBrake = TargetBrake;

    if (bIsInReverseMode)
    {
        // Swap: Brake pedal → Throttle, Throttle pedal → Brake
        DesiredThrottle = TargetBrake;
        DesiredBrake = TargetThrottle;
    }

    //==========================================================================
    // 🚨 CRITICAL: Instant Input Conflict Resolution
    //==========================================================================
    // Problem: FInterpTo causes both inputs to be non-zero during transitions,
    // creating a "fighting" effect where throttle and brake cancel each other.
    // Solution: Detect which input is dominant and INSTANTLY zero the other.
    
    constexpr float ConflictThreshold = 0.05f; // Minimum input to trigger conflict resolution
    
    // Case 1: User wants to accelerate (throttle dominant)
    if (DesiredThrottle > ConflictThreshold && DesiredBrake < ConflictThreshold)
    {
        // ⚡ INSTANT brake release for immediate power delivery
        Vehicle->InputTensor_GameThread.Brake = 0.0f;
        Vehicle->InputTensor_GameThread.Throttle = FMath::FInterpTo(CurrentThrottle, DesiredThrottle, DeltaTime, ThrottleRate);
    }
    // Case 2: User wants to brake (brake dominant)
    else if (DesiredBrake > ConflictThreshold && DesiredThrottle < ConflictThreshold)
    {
        // ⚡ INSTANT throttle cut for immediate braking response
        Vehicle->InputTensor_GameThread.Throttle = 0.0f;
        Vehicle->InputTensor_GameThread.Brake = FMath::FInterpTo(CurrentBrake, DesiredBrake, DeltaTime, BrakeRate);
    }
    // Case 3: Both inputs active (shouldn't happen with proper control, but handle gracefully)
    else if (DesiredThrottle > ConflictThreshold && DesiredBrake > ConflictThreshold)
    {
        // Priority: Braking wins (safety first)
        Vehicle->InputTensor_GameThread.Throttle = 0.0f;
        Vehicle->InputTensor_GameThread.Brake = FMath::FInterpTo(CurrentBrake, DesiredBrake, DeltaTime, BrakeRate);
    }
    // Case 4: Both inputs released (coast)
    else
    {
        Vehicle->InputTensor_GameThread.Throttle = FMath::FInterpTo(CurrentThrottle, 0.0f, DeltaTime, ThrottleRate);
        Vehicle->InputTensor_GameThread.Brake = FMath::FInterpTo(CurrentBrake, 0.0f, DeltaTime, BrakeRate);
    }

    //==========================================================================
    // Steering and Handbrake (No Conflict)
    //==========================================================================
    Vehicle->InputTensor_GameThread.Steering = FMath::FInterpTo(CurrentSteering, TargetSteering, DeltaTime, SteeringRate);
    Vehicle->InputTensor_GameThread.Handbrake = FMath::FInterpTo(CurrentHandbrake, TargetHandbrake, DeltaTime, HandbrakeRate);

    //==========================================================================
    // Mode Flags
    //==========================================================================
    Vehicle->InputTensor_GameThread.bReverseRequest = bIsInReverseMode;
    Vehicle->InputTensor_GameThread.bAerodynamicBraking = bIsAeroBraking;

    //==========================================================================
    // Debug Logging
    //==========================================================================
    if (bShowInputDebug)
    {
        UE_LOG(LogVehicleController, Log, TEXT("🎮 Input | ReverseMode:%s | AeroBrake:%s | T:%.2f B:%.2f S:%.2f H:%.2f | Raw T:%.2f B:%.2f"),
            bIsInReverseMode ? TEXT("ON") : TEXT("OFF"),
            bIsAeroBraking ? TEXT("ON") : TEXT("OFF"),
            Vehicle->InputTensor_GameThread.Throttle,
            Vehicle->InputTensor_GameThread.Brake,
            Vehicle->InputTensor_GameThread.Steering,
            Vehicle->InputTensor_GameThread.Handbrake,
            TargetThrottle,
            TargetBrake);
    }
}