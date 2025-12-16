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
//                          PAN PIVOT LOCATION
//------------------------------------------------------------------------------
FVector AViewportNavigator::GetPanPivotLocation() const
{
    if (!Vehicle || !Vehicle->VehicleHull) // Reason: validate vehicle state
        return FVector::ZeroVector;

    if (Vehicle->VehicleHull->DoesSocketExist(TEXT("PanPivot"))) // Reason: use named socket if available
        return Vehicle->VehicleHull->GetSocketLocation(TEXT("PanPivot"));

    return Vehicle->VehicleHull->GetCenterOfMass();
} // End GetPanPivotLocation()

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

    LastPanLoc = Arm->GetComponentLocation();
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

    UpdateDistanceMeasurements();

    if (bIsTransitioningToAnchor) // Reason: anchor transition takes priority
    {
        UpdateAnchorTransition(DeltaTime);
        return;
    } // End if (transitioning)

    UpdateCameraOrbiting(DeltaTime);
    UpdateCameraZooming(DeltaTime);
    UpdateCameraPanning(DeltaTime);
} // End Tick()

//------------------------------------------------------------------------------
//                          INITIALIZE CAMERA
//------------------------------------------------------------------------------
void AViewportNavigator::InitializeCamera()
{
    const FVector PivotLocation = GetPanPivotLocation();

    CurrentYaw = TargetYaw;
    CurrentPitch = TargetPitch;

    TargetPanLocation = FVector2D(PivotLocation.X, PivotLocation.Y);
    TargetZPosition = PivotLocation.Z;

    TargetArmLength = FMath::Clamp(DefaultArmLength, MinZoomDistance, MaxZoomDistance);

    if (Arm) // Reason: set initial transform
    {
        Arm->bDoCollisionTest = false;
        Arm->SetWorldLocation(PivotLocation);
        Arm->SetRelativeRotation(FRotator(CurrentPitch, CurrentYaw, 0.0f));
        Arm->TargetArmLength = TargetArmLength;
        bFirstRun = false;
    } // End if (arm valid)

    UpdateDistanceMeasurements();
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

    if (bIsMiddleMousePressed && !bIsShiftPressed) // Reason: orbital rotation mode
    {
        ProcessOrbitalAxisInput(MovementVector);
    } // End if (orbital mode)
    else if (bIsShiftPressed) // Reason: pan mode
    {
        ProcessPanInput(MovementVector);
    } // End else if (pan mode)
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
    bIsShiftPressed = true;
}

void AViewportNavigator::ProcessShiftReleased(const FInputActionValue& Value)
{
    bIsShiftPressed = false;
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
    CurrentPanInput = FVector2D(
        MovementVector.X * PanSensitivity,
        MovementVector.Y * PanSensitivity
    );
    bIsPanning = (MovementVector.X != 0.0f || MovementVector.Y != 0.0f);
} // End ProcessPanInput()

//------------------------------------------------------------------------------
//                          UPDATE CAMERA ORBITING
//------------------------------------------------------------------------------
void AViewportNavigator::UpdateCameraOrbiting(float DeltaTime)
{
    if (!Arm)
        return;

    const float OriginalTargetPitch = TargetPitch;

    // Ground avoidance
    if (TargetPitch > CurrentPitch) // Reason: looking down toward ground
    {
        const float GroundProximityThreshold = OrbitalSafetyDistance * 3.0f;
        if (DownDistance < GroundProximityThreshold) // Reason: near ground
        {
            if (DownDistance < OrbitalSafetyDistance * 1.2f) // Reason: very close to ground
            {
                TargetPitch = CurrentPitch;
            } // End if (very close)
            else
            {
                const float ProximityFactor = 1.0f - (DownDistance / GroundProximityThreshold);
                const float RestrictionStrength = FMath::Pow(ProximityFactor, 2.0f) * 0.95f + 0.05f;
                const float DesiredPitchDelta = TargetPitch - CurrentPitch;
                const float AllowedPitchDelta = DesiredPitchDelta * (1.0f - RestrictionStrength);
                TargetPitch = CurrentPitch + AllowedPitchDelta;
            } // End else (partial restriction)
        } // End if (near ground)
    } // End if (looking down)

    CurrentYaw = FMath::FInterpTo(CurrentYaw, TargetYaw, DeltaTime, CameraLagSpeed);
    CurrentPitch = FMath::FInterpTo(CurrentPitch, TargetPitch, DeltaTime, CameraLagSpeed);

    Arm->SetRelativeRotation(FRotator(CurrentPitch, CurrentYaw, 0.0f));

    TargetPitch = OriginalTargetPitch;
} // End UpdateCameraOrbiting()

//------------------------------------------------------------------------------
//                          RESTRICT ZOOM BY DISTANCE
//------------------------------------------------------------------------------
float AViewportNavigator::RestrictZoomByDistance(float CurrentLength, float DesiredLength, float Distance)
{
    float ProximityThreshold = CollisionSafetyThreshold * ProximityMultiplier + ProximityBuffer;

    if (Distance >= ProximityThreshold)
        return DesiredLength;

    float ProximityFactor = 1.0f - (Distance / ProximityThreshold);
    float RestrictionStrength = FMath::Pow(ProximityFactor, 2.0f) * 0.95f + 0.05f;

    if (Distance < CollisionSafetyThreshold * 1.2f) // Reason: block zoom when very close
        return CurrentLength;

    float ZoomDelta = DesiredLength - CurrentLength;
    return CurrentLength + ZoomDelta * (1.0f - RestrictionStrength);
} // End RestrictZoomByDistance()

//------------------------------------------------------------------------------
//                          UPDATE CAMERA ZOOMING
//------------------------------------------------------------------------------
void AViewportNavigator::UpdateCameraZooming(float DeltaTime)
{
    const float CurrentArmLength = Arm->TargetArmLength;
    float DesiredArmLength = TargetArmLength;
    const int32 ZoomDirection = FMath::Sign(DesiredArmLength - CurrentArmLength);

    if (ZoomDirection < 0) // Reason: zooming in
        DesiredArmLength = RestrictZoomByDistance(CurrentArmLength, DesiredArmLength, ForwardDistance);
    else if (ZoomDirection > 0) // Reason: zooming out
        DesiredArmLength = RestrictZoomByDistance(CurrentArmLength, DesiredArmLength, InverseForwardDistance);

    TargetArmLength = DesiredArmLength;
    Arm->TargetArmLength = FMath::FInterpTo(CurrentArmLength, TargetArmLength, DeltaTime, CameraLagSpeed);
} // End UpdateCameraZooming()

//------------------------------------------------------------------------------
//                          UPDATE CAMERA PANNING
//------------------------------------------------------------------------------
void AViewportNavigator::UpdateCameraPanning(float DeltaTime)
{
    if (!Arm || !Vehicle || !Vehicle->VehicleHull)
        return;

    const FVector PivotWS = GetPanPivotLocation();
    const FVector CurrentWS = Arm->GetComponentLocation();

    if (bIsShiftPressed && bIsPanning) // Reason: active panning
    {
        FVector PanDeltaWS = CurrentPanInput.X * DeltaTime * PanSensitivity * Cam->GetRightVector();
        FVector DesiredWS = CurrentWS + PanDeltaWS;

        FVector ToPivot = DesiredWS - PivotWS;
        ToPivot.Z = 0.0f;
        const float Distance2D = ToPivot.Size();
        const float Limit = 20.0f;

        if (Distance2D > Limit) // Reason: clamp pan radius
            DesiredWS = PivotWS + ToPivot.GetSafeNormal() * Limit;

        DesiredWS.Z += CurrentPanInput.Y * DeltaTime * PanSensitivity;
        DesiredWS.Z = FMath::Clamp(DesiredWS.Z, MinPanZ, MaxPanZ);

        TargetPanLocation = FVector2D(DesiredWS.X, DesiredWS.Y);
        TargetZPosition = DesiredWS.Z;
    } // End if (panning)
    else if (!bIsPanning)
    {
        CurrentPanInput = FVector2D::ZeroVector;
    } // End else (not panning)

    const FVector TargetWS(TargetPanLocation.X, TargetPanLocation.Y, TargetZPosition);
    const FVector NewWS = FMath::VInterpTo(CurrentWS, TargetWS, DeltaTime, CameraLagSpeed);
    Arm->SetWorldLocation(NewWS);
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

//------------------------------------------------------------------------------
//                          GET TRACE DISTANCE
//------------------------------------------------------------------------------
float AViewportNavigator::GetTraceDistance(UCameraComponent* Camera, FVector Direction, float MaxDistance)
{
    if (!Camera || Direction.IsZero() || MaxDistance <= 0.0f || !GetWorld())
        return MaxDistance;

    Direction.Normalize();

    FVector StartLocation = Camera->GetComponentLocation();
    AActor* IgnoreActor = Camera->GetOwner();

    FHitResult HitResult;
    FCollisionQueryParams CollisionParams;
    CollisionParams.bTraceComplex = true;

    if (IgnoreActor) // Reason: ignore self
    {
        CollisionParams.AddIgnoredActor(IgnoreActor);
        if (Vehicle && Vehicle != IgnoreActor)
            CollisionParams.AddIgnoredActor(Vehicle);
    } // End if (ignore actor)

    FVector EndLocation = StartLocation + (Direction * MaxDistance);

    bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, CollisionParams);

    if (bHit) // Reason: return actual distance on hit
        return (HitResult.ImpactPoint - StartLocation).Size();

    return MaxDistance;
} // End GetTraceDistance()

//------------------------------------------------------------------------------
//                          UPDATE DISTANCE MEASUREMENTS
//------------------------------------------------------------------------------
void AViewportNavigator::UpdateDistanceMeasurements()
{
    if (!Cam)
        return;

    ForwardDistance = GetTraceDistance(Cam, Cam->GetForwardVector(), 5000.0f);
    RightDistance = GetTraceDistance(Cam, Cam->GetRightVector(), 5000.0f);
    UpDistance = GetTraceDistance(Cam, FVector(0.0f, 0.0f, 1.0f), 5000.0f);
    LeftDistance = GetTraceDistance(Cam, -Cam->GetRightVector(), 5000.0f);
    InverseForwardDistance = GetTraceDistance(Cam, -Cam->GetForwardVector(), 5000.0f);
    DownDistance = GetTraceDistance(Cam, FVector(0.0f, 0.0f, -1.0f), 5000.0f);
} // End UpdateDistanceMeasurements()

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

        const FVector FinalLocation = TargetAnchorLocation;
        const FRotator FinalRotation = TargetAnchorRotation;

        Arm->SetWorldLocation(FinalLocation);
        Arm->SetRelativeRotation(FinalRotation);

        CurrentYaw = FinalRotation.Yaw;
        CurrentPitch = FinalRotation.Pitch;
        TargetYaw = CurrentYaw;
        TargetPitch = CurrentPitch;
        TargetPanLocation = FVector2D(FinalLocation.X, FinalLocation.Y);
        TargetZPosition = FinalLocation.Z;
        return;
    } // End if (complete)

    float SmoothAlpha = FMath::SmoothStep(0.0f, 1.0f, AnchorTransitionAlpha);

    FVector CurrentLocation = FMath::Lerp(AnchorStartLocation, TargetAnchorLocation, SmoothAlpha);
    Arm->SetWorldLocation(CurrentLocation);

    float RotationAlpha = FMath::SmoothStep(0.0f, 1.0f, AnchorTransitionAlpha * AnchorRotationSpeed);
    FRotator CurrentRotation = FMath::Lerp(AnchorStartRotation, TargetAnchorRotation, RotationAlpha);
    Arm->SetRelativeRotation(CurrentRotation);

    float CurrentArmLength = FMath::Lerp(AnchorStartArmLength, TargetArmLength, SmoothAlpha);
    Arm->TargetArmLength = CurrentArmLength;
} // End UpdateAnchorTransition()
