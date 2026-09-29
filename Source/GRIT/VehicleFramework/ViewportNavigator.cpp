#include "ViewportNavigator.h"
#include "VehicleConstruct.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Input/InputTensor.h"

//------------------------------------------------------------------------------
//                          CONSTRUCTOR
//------------------------------------------------------------------------------
AViewportNavigator::AViewportNavigator()
{
    PrimaryActorTick.bCanEverTick = true;
}

//------------------------------------------------------------------------------
//                          BEGIN PLAY
//------------------------------------------------------------------------------
void AViewportNavigator::BeginPlay()
{
    Super::BeginPlay();

    CurrentYaw = TargetYaw;
    CurrentPitch = TargetPitch;

    if (!Vehicle) // Reason: cache vehicle reference on begin play
    {
        Vehicle = Cast<AVehicleConstruct>(GetPawn());
    } // End if (vehicle null)

    if (!CacheVehicleComponents())
        return;

    if (Arm) // Reason: initialize arm with starting values
    {
        Arm->TargetArmLength = TargetArmLength;
        Arm->bDoCollisionTest = false;
    } // End if (arm valid)

    if (ULocalPlayer* LP = GetLocalPlayer()) // Reason: bind input mapping context
    {
        if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            Sub->AddMappingContext(CameraMappingContext, 0);
        } // End if (subsystem valid)
    } // End if (local player)

    // Lock brakes by default in configurator mode
    SetBrakeLock(true);
} // End BeginPlay()

//------------------------------------------------------------------------------
//                          ON POSSESS
//------------------------------------------------------------------------------
void AViewportNavigator::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    Vehicle = Cast<AVehicleConstruct>(InPawn);
    if (!CacheVehicleComponents())
        return;

    if (EnsureRequiredComponentsExist()) // Reason: initialize camera on possess
    {
        InitializeCamera();
    } // End if (components valid)

    // Lock brakes when possessing vehicle
    SetBrakeLock(true);
} // End OnPossess()

//------------------------------------------------------------------------------
//                          ON UNPOSSESS
//------------------------------------------------------------------------------
void AViewportNavigator::OnUnPossess()
{
    // Release brakes when unpossessing
    SetBrakeLock(false);
    Super::OnUnPossess();
} // End OnUnPossess()

//------------------------------------------------------------------------------
//                          CACHE VEHICLE COMPONENTS
//------------------------------------------------------------------------------
bool AViewportNavigator::CacheVehicleComponents()
{
    Vehicle = Cast<AVehicleConstruct>(GetPawn());
    if (!Vehicle) return false;

    Arm = Vehicle->GetSpringArm();
    Cam = Vehicle->GetCamera();

    if (!Arm || !Cam) return false;

    return true;
} // End CacheVehicleComponents()

//------------------------------------------------------------------------------
//                          SETUP INPUT COMPONENT
//------------------------------------------------------------------------------
void AViewportNavigator::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(InputComponent)) // Reason: bind all input actions
    {
        EIC->BindAction(OrbitalAxisControl, ETriggerEvent::Triggered, this, &AViewportNavigator::ProcessCameraInput);
        EIC->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AViewportNavigator::ProcessZoomInput);
        EIC->BindAction(MiddleMouseAction, ETriggerEvent::Triggered, this, &AViewportNavigator::ProcessMiddleMousePressed);
        EIC->BindAction(MiddleMouseAction, ETriggerEvent::Completed, this, &AViewportNavigator::ProcessMiddleMouseReleased);
        EIC->BindAction(PanEnableAction, ETriggerEvent::Triggered, this, &AViewportNavigator::ProcessShiftPressed);
        EIC->BindAction(PanEnableAction, ETriggerEvent::Completed, this, &AViewportNavigator::ProcessShiftReleased);
        EIC->BindAction(CycleModeAction, ETriggerEvent::Triggered, this, &AViewportNavigator::ProcessCycleMode);
        EIC->BindAction(ToggleMouseCursorAction, ETriggerEvent::Triggered, this, &AViewportNavigator::ProcessToggleMouse);
    } // End if (enhanced input)
} // End SetupInputComponent()

//------------------------------------------------------------------------------
//                          TICK
//------------------------------------------------------------------------------
void AViewportNavigator::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!EnsureRequiredComponentsExist())
        return;

    if (bIsTransitioningToAnchor) // Reason: anchor transition takes priority
    {
        UpdateAnchorTransition(DeltaTime);
        return;
    } // End if (transitioning)

    UpdateCameraOrbiting(DeltaTime);
    UpdateCameraZooming(DeltaTime);

    if (bIsMiddleMousePressed) // Reason: only pan when MMB is held
    {
        UpdateCameraPanning(DeltaTime);
    } // End if (panning)
} // End Tick()

//------------------------------------------------------------------------------
//                          INITIALIZE CAMERA
//------------------------------------------------------------------------------
void AViewportNavigator::InitializeCamera()
{
    CurrentYaw = TargetYaw;
    CurrentPitch = TargetPitch;

    TargetArmLength = FMath::Clamp(DefaultArmLength, MinZoomDistance, MaxZoomDistance);

    if (Arm) // Reason: set initial transform - keep attachment, only set rotation/length
    {
        Arm->bDoCollisionTest = false;
        Arm->SetRelativeRotation(FRotator(CurrentPitch, CurrentYaw, 0.0f));
        Arm->TargetArmLength = TargetArmLength;

        // Cache initial offsets from VehicleConstruct setup
        InitialSocketOffset = Arm->SocketOffset;
        TargetSocketOffset = InitialSocketOffset;
        TargetTargetOffset = Arm->TargetOffset;

        bFirstRun = false;
    } // End if (arm valid)

} // End InitializeCamera()

//------------------------------------------------------------------------------
//                          SET BRAKE LOCK
//------------------------------------------------------------------------------
void AViewportNavigator::SetBrakeLock(bool bLocked)
{
    bBrakesLocked = bLocked;

    if (Vehicle) // Reason: apply brake state to vehicle input tensor
    {
        Vehicle->InputTensor_GameThread.Handbrake = bLocked ? 1.0f : 0.0f;
        Vehicle->InputTensor_GameThread.Brake = bLocked ? 1.0f : 0.0f;
    } // End if (vehicle valid)
} // End SetBrakeLock()

//------------------------------------------------------------------------------
//                          PROCESS CAMERA INPUT
//------------------------------------------------------------------------------
void AViewportNavigator::ProcessCameraInput(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (bIsMiddleMousePressed) // Reason: MMB = pan mode
    {
        ProcessPanInput(MovementVector);
    } // End if (pan mode)
    else // Reason: default = orbital rotation mode
    {
        ProcessOrbitalAxisInput(MovementVector);
    } // End else (orbital mode)
} // End ProcessCameraInput()

//------------------------------------------------------------------------------
//                          PROCESS ZOOM INPUT
//------------------------------------------------------------------------------
void AViewportNavigator::ProcessZoomInput(const FInputActionValue& Value)
{
    TargetArmLength -= Value.Get<float>() * ZoomSpeed;
    TargetArmLength = FMath::Clamp(TargetArmLength, MinZoomDistance, MaxZoomDistance);
} // End ProcessZoomInput()

//------------------------------------------------------------------------------
//                          MOUSE/MODIFIER INPUT
//------------------------------------------------------------------------------
void AViewportNavigator::ProcessMiddleMousePressed(const FInputActionValue& Value)
{
    bIsMiddleMousePressed = true;
}

void AViewportNavigator::ProcessMiddleMouseReleased(const FInputActionValue& Value)
{
    bIsMiddleMousePressed = false;
}

void AViewportNavigator::ProcessShiftPressed(const FInputActionValue& Value)
{
    // Reserved for future use (e.g., speed modifier)
}

void AViewportNavigator::ProcessShiftReleased(const FInputActionValue& Value)
{
    // Reserved for future use
}

//------------------------------------------------------------------------------
//                          ORBITAL AXIS INPUT
//------------------------------------------------------------------------------
void AViewportNavigator::ProcessOrbitalAxisInput(const FVector2D& MovementVector)
{
    TargetYaw += MovementVector.X * OrbitalSensitivity;
    TargetPitch = FMath::Clamp(TargetPitch + MovementVector.Y * OrbitalSensitivity, MinPitchAngle, MaxPitchAngle);
} // End ProcessOrbitalAxisInput()

//------------------------------------------------------------------------------
//                          PAN INPUT
//------------------------------------------------------------------------------
void AViewportNavigator::ProcessPanInput(const FVector2D& MovementVector)
{
    CurrentPanInput = MovementVector;
} // End ProcessPanInput()

//------------------------------------------------------------------------------
//                          UPDATE CAMERA ORBITING
//------------------------------------------------------------------------------
void AViewportNavigator::UpdateCameraOrbiting(float DeltaTime)
{
    if (!Arm || !Cam || !GetWorld())
        return;

    const float OriginalTargetPitch = TargetPitch;
    const float OriginalTargetYaw = TargetYaw;

    //--------------------------------------------------------------------------
    // Clamp max movement per frame
    //--------------------------------------------------------------------------
    constexpr float MaxPitchDeltaPerFrame = 8.0f;                             // [deg]
    constexpr float MaxYawDeltaPerFrame = 15.0f;                              // [deg]

    float PitchDelta = FMath::Clamp(TargetPitch - CurrentPitch, -MaxPitchDeltaPerFrame, MaxPitchDeltaPerFrame);
    float YawDelta = FMath::Clamp(TargetYaw - CurrentYaw, -MaxYawDeltaPerFrame, MaxYawDeltaPerFrame);

    const FVector Origin = Arm->GetComponentLocation() + Arm->TargetOffset;
    const float ArmLen = Arm->TargetArmLength;

    //--------------------------------------------------------------------------
    // Recovery: if current position overlaps geometry, snap to safe default
    //--------------------------------------------------------------------------
    if (IsCameraOverlapping(Origin, CurrentPitch, CurrentYaw, ArmLen))
    {
        CurrentPitch = -10.0f;
        CurrentYaw = TargetYaw;
        TargetPitch = CurrentPitch;
        Arm->SetRelativeRotation(FRotator(CurrentPitch, CurrentYaw, 0.0f));
        return;
    }

    //--------------------------------------------------------------------------
    // Independent axis validation (sphere sweep)
    //--------------------------------------------------------------------------
    if (FMath::Abs(PitchDelta) > 0.01f)
    {
        if (!IsCameraPositionClear(Origin, CurrentPitch + PitchDelta, CurrentYaw, ArmLen))
            PitchDelta = 0.0f;
    }

    if (FMath::Abs(YawDelta) > 0.01f)
    {
        if (!IsCameraPositionClear(Origin, CurrentPitch, CurrentYaw + YawDelta, ArmLen))
            YawDelta = 0.0f;
    }

    // Combined validation (catches diagonal into concave corners)
    if (FMath::Abs(PitchDelta) > 0.01f && FMath::Abs(YawDelta) > 0.01f)
    {
        if (!IsCameraPositionClear(Origin, CurrentPitch + PitchDelta, CurrentYaw + YawDelta, ArmLen))
        {
            PitchDelta = 0.0f;
            YawDelta = 0.0f;
        }
    }

    //--------------------------------------------------------------------------
    // Apply validated deltas with smooth interpolation
    //--------------------------------------------------------------------------
    const float DampedTargetPitch = CurrentPitch + PitchDelta;
    const float DampedTargetYaw = CurrentYaw + YawDelta;

    CurrentYaw = FMath::FInterpTo(CurrentYaw, DampedTargetYaw, DeltaTime, CameraLagSpeed);
    CurrentPitch = FMath::FInterpTo(CurrentPitch, DampedTargetPitch, DeltaTime, CameraLagSpeed);

    Arm->SetRelativeRotation(FRotator(CurrentPitch, CurrentYaw, 0.0f));

    // Restore original targets for next frame input accumulation
    TargetPitch = OriginalTargetPitch;
    TargetYaw = OriginalTargetYaw;
} // End UpdateCameraOrbiting()




//------------------------------------------------------------------------------
//                          UPDATE CAMERA ZOOMING
//------------------------------------------------------------------------------
void AViewportNavigator::UpdateCameraZooming(float DeltaTime)
{
    if (!Arm || !Cam || !GetWorld())
        return;

    const float CurrentArmLength = Arm->TargetArmLength;
    float DesiredArmLength = TargetArmLength;

    //--------------------------------------------------------------------------
    // Clamp max zoom delta per frame
    //--------------------------------------------------------------------------
    constexpr float MaxZoomDeltaPerFrame = 100.0f;                            // [cm]
    float ZoomDelta = FMath::Clamp(DesiredArmLength - CurrentArmLength, -MaxZoomDeltaPerFrame, MaxZoomDeltaPerFrame);
    DesiredArmLength = CurrentArmLength + ZoomDelta;

    //--------------------------------------------------------------------------
    // Validate proposed zoom position with sphere sweep
    //--------------------------------------------------------------------------
    const FVector Origin = Arm->GetComponentLocation() + Arm->TargetOffset;

    if (!IsCameraPositionClear(Origin, CurrentPitch, CurrentYaw, DesiredArmLength))
        DesiredArmLength = CurrentArmLength;

    TargetArmLength = DesiredArmLength;
    Arm->TargetArmLength = FMath::FInterpTo(CurrentArmLength, TargetArmLength, DeltaTime, CameraLagSpeed);
} // End UpdateCameraZooming()

//------------------------------------------------------------------------------
//                          UPDATE CAMERA PANNING
//------------------------------------------------------------------------------
void AViewportNavigator::UpdateCameraPanning(float DeltaTime)
{
    if (!Arm || !Cam || !GetWorld())
        return;

    //--------------------------------------------------------------------------
    // Anti-tunneling: Clamp max pan input per frame
    //--------------------------------------------------------------------------
    constexpr float MaxPanInputPerFrame = 5.0f;
    CurrentPanInput.X = FMath::Clamp(CurrentPanInput.X, -MaxPanInputPerFrame, MaxPanInputPerFrame);
    CurrentPanInput.Y = FMath::Clamp(CurrentPanInput.Y, -MaxPanInputPerFrame, MaxPanInputPerFrame);

    // Get camera-relative directions for intuitive panning
    FVector CamRight = Cam->GetRightVector();
    CamRight.Z = 0.0f;
    CamRight.Normalize();

    const float PanSpeed = PanSensitivity * 50.0f;                            // [cm/s]

    //--------------------------------------------------------------------------
    // Compute proposed offsets
    //--------------------------------------------------------------------------
    FVector ProposedTargetOffset = TargetTargetOffset;
    FVector ProposedSocketOffset = TargetSocketOffset;

    if (FMath::Abs(CurrentPanInput.X) > 0.01f)
        ProposedTargetOffset += CamRight * CurrentPanInput.X * DeltaTime * PanSpeed;

    if (FMath::Abs(CurrentPanInput.Y) > 0.01f)
        ProposedSocketOffset.Z += CurrentPanInput.Y * DeltaTime * PanSpeed;

    //--------------------------------------------------------------------------
    // Clamp offsets to reasonable bounds
    //--------------------------------------------------------------------------
    const float MinZ = InitialSocketOffset.Z + MinPanZ;
    const float MaxZ = InitialSocketOffset.Z + MaxPanZ;
    ProposedSocketOffset.Z = FMath::Clamp(ProposedSocketOffset.Z, MinZ, MaxZ);

    constexpr float MaxHorizontalPan = 300.0f;                                // [cm]
    ProposedTargetOffset.X = FMath::Clamp(ProposedTargetOffset.X, -MaxHorizontalPan, MaxHorizontalPan);
    ProposedTargetOffset.Y = FMath::Clamp(ProposedTargetOffset.Y, -MaxHorizontalPan, MaxHorizontalPan);

    //--------------------------------------------------------------------------
    // Validate proposed pan position with sphere sweep
    //--------------------------------------------------------------------------
    // Temporarily apply proposed offsets to compute where the camera would end up
    const FVector OriginalTargetOffset = Arm->TargetOffset;
    const FVector OriginalSocketOffset = Arm->SocketOffset;

    // Test with proposed offsets: sweep from arm origin (with new TargetOffset) to camera
    const FVector ProposedOrigin = Arm->GetComponentLocation() + ProposedTargetOffset;
    const FRotator CamRot(CurrentPitch, CurrentYaw, 0.0f);
    FVector ProposedCamPos = ProposedOrigin - CamRot.Vector() * Arm->TargetArmLength;
    ProposedCamPos += FRotationMatrix(CamRot).TransformVector(ProposedSocketOffset);

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CameraPan), false);
    if (Vehicle) Params.AddIgnoredActor(Vehicle);

    const bool bBlocked = GetWorld()->SweepSingleByChannel(
        Hit, ProposedOrigin, ProposedCamPos, FQuat::Identity,
        ECC_Camera, FCollisionShape::MakeSphere(CameraProbeRadius), Params);

    if (!bBlocked)
    {
        TargetTargetOffset = ProposedTargetOffset;
        TargetSocketOffset = ProposedSocketOffset;
    }
    // If blocked, offsets stay at current values (hard reject)

    //--------------------------------------------------------------------------
    // Smooth interpolation to target offsets
    //--------------------------------------------------------------------------
    Arm->SocketOffset = FMath::VInterpTo(Arm->SocketOffset, TargetSocketOffset, DeltaTime, CameraLagSpeed);
    Arm->TargetOffset = FMath::VInterpTo(Arm->TargetOffset, TargetTargetOffset, DeltaTime, CameraLagSpeed);

    // Clear input after processing
    CurrentPanInput = FVector2D::ZeroVector;
} // End UpdateCameraPanning()

//------------------------------------------------------------------------------
//                          ENSURE REQUIRED COMPONENTS
//------------------------------------------------------------------------------
bool AViewportNavigator::EnsureRequiredComponentsExist()
{
    if (!Arm && !CacheVehicleComponents())
        return false;

    if (!Cam && !CacheVehicleComponents())
        return false;

    if (!Vehicle) // Reason: reacquire vehicle reference
    {
        Vehicle = Cast<AVehicleConstruct>(GetPawn());
        if (!Vehicle)
            return false;
    } // End if (no vehicle)

    return true;
} // End EnsureRequiredComponentsExist()

/*====================================================================================================================================
                                                     COLLISION (Predictive Sphere Sweep)
======================================================================================================================================*/

FVector AViewportNavigator::ComputeCameraWorldPosition(float Pitch, float Yaw, float ArmLen) const
{
    const FVector Origin = Arm->GetComponentLocation() + Arm->TargetOffset;
    const FRotator Rot(Pitch, Yaw, 0.0f);
    FVector Pos = Origin - Rot.Vector() * ArmLen;
    Pos += FRotationMatrix(Rot).TransformVector(Arm->SocketOffset);
    return Pos;
}

bool AViewportNavigator::IsCameraPositionClear(const FVector& Origin, float Pitch, float Yaw, float ArmLen) const
{
    const FVector ProposedPos = ComputeCameraWorldPosition(Pitch, Yaw, ArmLen);

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CameraOrbit), false);
    if (Vehicle) Params.AddIgnoredActor(Vehicle);

    const bool bHit = GetWorld()->SweepSingleByChannel(
        Hit, Origin, ProposedPos, FQuat::Identity,
        ECC_Camera, FCollisionShape::MakeSphere(CameraProbeRadius), Params);

    return !bHit;
}

bool AViewportNavigator::IsCameraOverlapping(const FVector& Origin, float Pitch, float Yaw, float ArmLen) const
{
    const FVector CamPos = ComputeCameraWorldPosition(Pitch, Yaw, ArmLen);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(CameraOverlap), false);
    if (Vehicle) Params.AddIgnoredActor(Vehicle);

    return GetWorld()->OverlapBlockingTestByChannel(
        CamPos, FQuat::Identity,
        ECC_Camera, FCollisionShape::MakeSphere(CameraProbeRadius), Params);
}

//------------------------------------------------------------------------------
//                          PROCESS CYCLE MODE
//------------------------------------------------------------------------------
void AViewportNavigator::ProcessCycleMode(const FInputActionValue& Value)
{
    uint8 Next = static_cast<uint8>(CurrentMode) + 1;
    if (Next >= static_cast<uint8>(EViewportMode::Static) + 1)
        Next = 0;

    CurrentMode = static_cast<EViewportMode>(Next);
} // End ProcessCycleMode()

//------------------------------------------------------------------------------
//                          PROCESS TOGGLE MOUSE
//------------------------------------------------------------------------------
void AViewportNavigator::ProcessToggleMouse(const FInputActionValue& Value)
{
    if (ULocalPlayer* LP = GetLocalPlayer()) // Reason: toggle cursor visibility
    {
        if (LP->ViewportClient)
        {
            const bool bShow = !LP->ViewportClient->Viewport->IsCursorVisible();
            LP->ViewportClient->Viewport->ShowCursor(bShow);
            SetShowMouseCursor(bShow);
        } // End if (viewport client)
    } // End if (local player)
} // End ProcessToggleMouse()

//------------------------------------------------------------------------------
//                          TRANSITION TO ANCHOR
//------------------------------------------------------------------------------
void AViewportNavigator::TransitionToAnchor(const FTransform& TargetTransform)
{
    if (!Arm || !Vehicle)
        return;

    AnchorStartLocation = Arm->GetComponentLocation();
    AnchorStartRotation = Arm->GetRelativeRotation();
    AnchorStartArmLength = Arm->TargetArmLength;

    TargetAnchorLocation = TargetTransform.GetLocation();
    TargetAnchorRotation = TargetTransform.GetRotation().Rotator();

    AnchorTransitionAlpha = 0.0f;
    bIsTransitioningToAnchor = true;
} // End TransitionToAnchor()

//------------------------------------------------------------------------------
//                          UPDATE ANCHOR TRANSITION
//------------------------------------------------------------------------------
void AViewportNavigator::UpdateAnchorTransition(float DeltaTime)
{
    if (!bIsTransitioningToAnchor || !Arm)
        return;

    AnchorTransitionAlpha += DeltaTime * AnchorTransitionSpeed;

    if (AnchorTransitionAlpha >= 1.0f) // Reason: transition complete
    {
        AnchorTransitionAlpha = 1.0f;
        bIsTransitioningToAnchor = false;

        const FRotator FinalRotation = TargetAnchorRotation;

        Arm->SetRelativeRotation(FinalRotation);

        CurrentYaw = FinalRotation.Yaw;
        CurrentPitch = FinalRotation.Pitch;
        TargetYaw = CurrentYaw;
        TargetPitch = CurrentPitch;

        // Reset pan offsets to initial state after anchor transition
        TargetSocketOffset = InitialSocketOffset;
        TargetTargetOffset = FVector::ZeroVector;
        Arm->SocketOffset = InitialSocketOffset;
        Arm->TargetOffset = FVector::ZeroVector;
        return;
    } // End if (complete)

    float SmoothAlpha = FMath::SmoothStep(0.0f, 1.0f, AnchorTransitionAlpha);

    // Interpolate rotation
    float RotationAlpha = FMath::SmoothStep(0.0f, 1.0f, AnchorTransitionAlpha * AnchorRotationSpeed);
    FRotator CurrentRotation = FMath::Lerp(AnchorStartRotation, TargetAnchorRotation, RotationAlpha);
    Arm->SetRelativeRotation(CurrentRotation);

    // Interpolate arm length
    float CurrentArmLength = FMath::Lerp(AnchorStartArmLength, TargetArmLength, SmoothAlpha);
    Arm->TargetArmLength = CurrentArmLength;
} // End UpdateAnchorTransition()
