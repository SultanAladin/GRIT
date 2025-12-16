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

    UpdateDistanceMeasurements();

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
    if (!Arm || !Cam)
        return;

    const float OriginalTargetPitch = TargetPitch;
    const float OriginalTargetYaw = TargetYaw;

    //--------------------------------------------------------------------------
    // Anti-tunneling: Clamp max movement per frame
    //--------------------------------------------------------------------------
    constexpr float MaxPitchDeltaPerFrame = 8.0f;                             // [deg]
    constexpr float MaxYawDeltaPerFrame = 15.0f;                              // [deg]

    float PitchDelta = FMath::Clamp(TargetPitch - CurrentPitch, -MaxPitchDeltaPerFrame, MaxPitchDeltaPerFrame);
    float YawDelta = FMath::Clamp(TargetYaw - CurrentYaw, -MaxYawDeltaPerFrame, MaxYawDeltaPerFrame);

    //--------------------------------------------------------------------------
    // Velocity-scaled lookahead: faster movement = further trace
    //--------------------------------------------------------------------------
    const float AngularSpeed = FMath::Abs(PitchDelta) + FMath::Abs(YawDelta); // [deg/frame]
    const float SpeedMultiplier = FMath::Clamp(AngularSpeed * 0.1f, 1.0f, 5.0f);
    const float TraceDistance = OrbitalSafetyDistance * 4.0f * SpeedMultiplier;

    // Precompute inverse for damper calculations (avoid division)
    const float ProximityThreshold = OrbitalSafetyDistance * 3.0f;
    const float InvProximityThreshold = 1.0f / FMath::Max(ProximityThreshold, 1.0f);

    //--------------------------------------------------------------------------
    // Determine movement direction flags for adaptive cone traces
    //--------------------------------------------------------------------------
    const bool bMovingRight = YawDelta > 0.1f;
    const bool bMovingLeft = YawDelta < -0.1f;
    const bool bMovingDown = PitchDelta > 0.1f;
    const bool bMovingUp = PitchDelta < -0.1f;

    //--------------------------------------------------------------------------
    // Ground avoidance when pitching down (positive pitch = looking down)
    //--------------------------------------------------------------------------
    if (bMovingDown) // Reason: only trace when moving toward ground
    {
        const float DownDist = GetAdaptiveConeTrace(FVector(0.0f, 0.0f, -1.0f), TraceDistance, TraceConeAngle,
                                                     bMovingRight, bMovingLeft, false, true);

        if (DownDist < ProximityThreshold)
        {
            // Suspension damping: closer = stronger resistance (using multiplication)
            const float Penetration = FMath::Max(0.0f, ProximityThreshold - DownDist);
            const float DamperStrength = FMath::Square(Penetration * InvProximityThreshold); // [0-1] quadratic
            PitchDelta *= (1.0f - DamperStrength);

            // Hard stop at minimum distance
            if (DownDist < OrbitalSafetyDistance)
            {
                PitchDelta = 0.0f;
            } // End if (hard stop)
        } // End if (within threshold)
    } // End if (pitching down)

    //--------------------------------------------------------------------------
    // Sky/ceiling avoidance when pitching up (negative pitch = looking up)
    //--------------------------------------------------------------------------
    if (bMovingUp)
    {
        const float UpDist = GetAdaptiveConeTrace(FVector(0.0f, 0.0f, 1.0f), TraceDistance, TraceConeAngle,
                                                   bMovingRight, bMovingLeft, true, false);

        if (UpDist < ProximityThreshold)
        {
            const float Penetration = FMath::Max(0.0f, ProximityThreshold - UpDist);
            const float DamperStrength = FMath::Square(Penetration * InvProximityThreshold);
            PitchDelta *= (1.0f - DamperStrength);

            if (UpDist < OrbitalSafetyDistance)
            {
                PitchDelta = 0.0f;
            }
        }
    } // End if (pitching up)

    //--------------------------------------------------------------------------
    // Side collision when yawing
    //--------------------------------------------------------------------------
    if (bMovingRight || bMovingLeft)
    {
        const FVector YawDir = bMovingRight ? Cam->GetRightVector() : -Cam->GetRightVector();
        const float SideProximity = OrbitalSafetyDistance * 2.0f;
        const float InvSideProximity = 1.0f / FMath::Max(SideProximity, 1.0f);

        const float SideDist = GetAdaptiveConeTrace(YawDir, TraceDistance, TraceConeAngle,
                                                     bMovingRight, bMovingLeft, bMovingUp, bMovingDown);

        if (SideDist < SideProximity)
        {
            const float Penetration = FMath::Max(0.0f, SideProximity - SideDist);
            const float DamperStrength = FMath::Square(Penetration * InvSideProximity);
            YawDelta *= (1.0f - DamperStrength);

            if (SideDist < OrbitalSafetyDistance * 0.5f)
            {
                YawDelta = 0.0f;
            }
        }
    } // End if (yawing)

    //--------------------------------------------------------------------------
    // Apply damped deltas and smooth interpolation
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
//                          RESTRICT ZOOM BY DISTANCE (Suspension-style)
//------------------------------------------------------------------------------
float AViewportNavigator::RestrictZoomByDistance(float CurrentLength, float DesiredLength, float Distance)
{
    const float ProximityThreshold = CollisionSafetyThreshold * ProximityMultiplier + ProximityBuffer;

    if (Distance >= ProximityThreshold)
        return DesiredLength;

    // Precompute inverse to avoid division in damper calculation
    const float InvProximityThreshold = 1.0f / FMath::Max(ProximityThreshold, 1.0f);

    // Suspension damping: closer = stronger resistance (using multiplication)
    const float Penetration = FMath::Max(0.0f, ProximityThreshold - Distance);
    const float DamperStrength = FMath::Square(Penetration * InvProximityThreshold); // [0-1] quadratic

    // Hard stop at minimum distance
    if (Distance < CollisionSafetyThreshold)
        return CurrentLength;

    // Damped zoom delta
    const float ZoomDelta = DesiredLength - CurrentLength;
    return CurrentLength + ZoomDelta * (1.0f - DamperStrength);
} // End RestrictZoomByDistance()

//------------------------------------------------------------------------------
//                          UPDATE CAMERA ZOOMING
//------------------------------------------------------------------------------
void AViewportNavigator::UpdateCameraZooming(float DeltaTime)
{
    if (!Arm || !Cam)
        return;

    const float CurrentArmLength = Arm->TargetArmLength;
    float DesiredArmLength = TargetArmLength;

    //--------------------------------------------------------------------------
    // Anti-tunneling: Clamp max zoom delta per frame
    //--------------------------------------------------------------------------
    constexpr float MaxZoomDeltaPerFrame = 100.0f;                            // [cm]
    float ZoomDelta = FMath::Clamp(DesiredArmLength - CurrentArmLength, -MaxZoomDeltaPerFrame, MaxZoomDeltaPerFrame);
    DesiredArmLength = CurrentArmLength + ZoomDelta;

    //--------------------------------------------------------------------------
    // Velocity-scaled lookahead: faster zoom = further trace
    //--------------------------------------------------------------------------
    const float ZoomVelocity = FMath::Abs(ZoomDelta);                         // [cm/frame]
    const float SpeedMultiplier = FMath::Clamp(ZoomVelocity * 0.02f, 1.0f, 5.0f);
    const float TraceDistance = CollisionSafetyThreshold * 5.0f * SpeedMultiplier;

    // Only trace in direction of zoom movement
    if (ZoomDelta < -1.0f) // Reason: zooming IN (toward vehicle)
    {
        // Adaptive cone trace forward (camera looks at vehicle)
        const float ForwardDist = GetAdaptiveConeTrace(Cam->GetForwardVector(), TraceDistance, TraceConeAngle,
                                                        true, true, true, true); // All quadrants when zooming
        DesiredArmLength = RestrictZoomByDistance(CurrentArmLength, DesiredArmLength, ForwardDist);
    }
    else if (ZoomDelta > 1.0f) // Reason: zooming OUT (away from vehicle)
    {
        // Adaptive cone trace backward (away from vehicle)
        const float BackwardDist = GetAdaptiveConeTrace(-Cam->GetForwardVector(), TraceDistance, TraceConeAngle,
                                                         true, true, true, true); // All quadrants when zooming
        DesiredArmLength = RestrictZoomByDistance(CurrentArmLength, DesiredArmLength, BackwardDist);
    }

    TargetArmLength = DesiredArmLength;
    Arm->TargetArmLength = FMath::FInterpTo(CurrentArmLength, TargetArmLength, DeltaTime, CameraLagSpeed);
} // End UpdateCameraZooming()

//------------------------------------------------------------------------------
//                          UPDATE CAMERA PANNING
//------------------------------------------------------------------------------
void AViewportNavigator::UpdateCameraPanning(float DeltaTime)
{
    if (!Arm || !Cam)
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

    const float PanSpeed = PanSensitivity * 50.0f;                            // [cm/s] - Scale for reasonable speed
    const float PanSafetyDist = CollisionSafetyThreshold;                     // [cm] - Collision buffer

    // Precompute inverse for damper calculations (avoid division)
    const float ProximityThreshold = PanSafetyDist * 3.0f;
    const float InvProximityThreshold = 1.0f / FMath::Max(ProximityThreshold, 1.0f);

    //--------------------------------------------------------------------------
    // Determine movement direction flags for adaptive cone traces
    //--------------------------------------------------------------------------
    const bool bMovingRight = CurrentPanInput.X > 0.01f;
    const bool bMovingLeft = CurrentPanInput.X < -0.01f;
    const bool bMovingUp = CurrentPanInput.Y > 0.01f;
    const bool bMovingDown = CurrentPanInput.Y < -0.01f;

    //--------------------------------------------------------------------------
    // Horizontal pan with directional collision (cone trace)
    //--------------------------------------------------------------------------
    if (bMovingRight || bMovingLeft)
    {
        const FVector PanDir = bMovingRight ? CamRight : -CamRight;
        const float SideDist = GetAdaptiveConeTrace(PanDir, PanSafetyDist * 4.0f, TraceConeAngle,
                                                     bMovingRight, bMovingLeft, bMovingUp, bMovingDown);

        float PanMultiplier = 1.0f;
        if (SideDist < ProximityThreshold)
        {
            // Suspension damping (using multiplication)
            const float Penetration = FMath::Max(0.0f, ProximityThreshold - SideDist);
            PanMultiplier = 1.0f - FMath::Square(Penetration * InvProximityThreshold);

            // Hard stop
            if (SideDist < PanSafetyDist)
                PanMultiplier = 0.0f;
        }

        TargetTargetOffset += CamRight * CurrentPanInput.X * DeltaTime * PanSpeed * PanMultiplier;
    }

    //--------------------------------------------------------------------------
    // Vertical pan with directional collision (cone trace)
    //--------------------------------------------------------------------------
    if (bMovingUp || bMovingDown)
    {
        const FVector VertDir = bMovingUp ? FVector::UpVector : FVector::DownVector;
        const float VertDist = GetAdaptiveConeTrace(VertDir, PanSafetyDist * 4.0f, TraceConeAngle,
                                                     bMovingRight, bMovingLeft, bMovingUp, bMovingDown);

        float PanMultiplier = 1.0f;
        if (VertDist < ProximityThreshold)
        {
            const float Penetration = FMath::Max(0.0f, ProximityThreshold - VertDist);
            PanMultiplier = 1.0f - FMath::Square(Penetration * InvProximityThreshold);

            if (VertDist < PanSafetyDist)
                PanMultiplier = 0.0f;
        }

        TargetSocketOffset.Z += CurrentPanInput.Y * DeltaTime * PanSpeed * PanMultiplier;
    }

    //--------------------------------------------------------------------------
    // Clamp offsets to reasonable bounds
    //--------------------------------------------------------------------------
    const float MinZ = InitialSocketOffset.Z + MinPanZ;
    const float MaxZ = InitialSocketOffset.Z + MaxPanZ;
    TargetSocketOffset.Z = FMath::Clamp(TargetSocketOffset.Z, MinZ, MaxZ);

    constexpr float MaxHorizontalPan = 300.0f;                                // [cm]
    TargetTargetOffset.X = FMath::Clamp(TargetTargetOffset.X, -MaxHorizontalPan, MaxHorizontalPan);
    TargetTargetOffset.Y = FMath::Clamp(TargetTargetOffset.Y, -MaxHorizontalPan, MaxHorizontalPan);

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

//------------------------------------------------------------------------------
//                          GET ADAPTIVE CONE TRACE
//------------------------------------------------------------------------------
float AViewportNavigator::GetAdaptiveConeTrace(FVector Direction, float MaxDistance, float ConeAngleDeg,
                                                bool bTraceRight, bool bTraceLeft, bool bTraceUp, bool bTraceDown)
{
    if (!Cam || Direction.IsNearlyZero())
        return MaxDistance;

    // Center ray always fires
    float MinDist = GetTraceDistance(Cam, Direction, MaxDistance);

    // Build orthonormal basis from direction
    FVector Right = FVector::CrossProduct(Direction, FVector::UpVector);
    if (Right.IsNearlyZero()) // Reason: direction is vertical, use forward as reference
    {
        Right = FVector::CrossProduct(Direction, FVector::ForwardVector);
    }
    Right.Normalize();

    FVector Up = FVector::CrossProduct(Right, Direction);
    Up.Normalize();

    const float AngleRad = FMath::DegreesToRadians(ConeAngleDeg);
    const float SinAngle = FMath::Sin(AngleRad);
    const float CosAngle = FMath::Cos(AngleRad);

    // Only fire corner rays in relevant quadrants based on movement direction
    if (bTraceRight && bTraceUp) // Top-Right quadrant
    {
        FVector CornerDir = (Direction * CosAngle + (Right + Up) * SinAngle * 0.707f).GetSafeNormal();
        MinDist = FMath::Min(MinDist, GetTraceDistance(Cam, CornerDir, MaxDistance));
    }

    if (bTraceRight && bTraceDown) // Bottom-Right quadrant
    {
        FVector CornerDir = (Direction * CosAngle + (Right - Up) * SinAngle * 0.707f).GetSafeNormal();
        MinDist = FMath::Min(MinDist, GetTraceDistance(Cam, CornerDir, MaxDistance));
    }

    if (bTraceLeft && bTraceUp) // Top-Left quadrant
    {
        FVector CornerDir = (Direction * CosAngle + (-Right + Up) * SinAngle * 0.707f).GetSafeNormal();
        MinDist = FMath::Min(MinDist, GetTraceDistance(Cam, CornerDir, MaxDistance));
    }

    if (bTraceLeft && bTraceDown) // Bottom-Left quadrant
    {
        FVector CornerDir = (Direction * CosAngle + (-Right - Up) * SinAngle * 0.707f).GetSafeNormal();
        MinDist = FMath::Min(MinDist, GetTraceDistance(Cam, CornerDir, MaxDistance));
    }

    return MinDist;
} // End GetAdaptiveConeTrace()

//------------------------------------------------------------------------------
//                          GET TRACE DISTANCE
//------------------------------------------------------------------------------
float AViewportNavigator::GetTraceDistance(UCameraComponent* Camera, FVector Direction, float MaxDistance)
{
    if (!Camera || Direction.IsZero() || MaxDistance <= 0.0f || !GetWorld())
        return MaxDistance;

    Direction.Normalize();

    FVector StartLocation = Camera->GetComponentLocation();

    FHitResult HitResult;
    FCollisionQueryParams CollisionParams;
    CollisionParams.bTraceComplex = false;
    CollisionParams.AddIgnoredActor(Camera->GetOwner());
    if (Vehicle) CollisionParams.AddIgnoredActor(Vehicle);

    FVector EndLocation = StartLocation + (Direction * MaxDistance);

    bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, CollisionParams);

#if !UE_BUILD_SHIPPING
    // Debug visualization
    FColor TraceColor = bHit ? FColor::Red : FColor::Green;
    DrawDebugLine(GetWorld(), StartLocation, bHit ? HitResult.ImpactPoint : EndLocation, TraceColor, false, -1.0f, 0, 1.0f);
    if (bHit)
    {
        DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 5.0f, 8, FColor::Yellow, false, -1.0f, 0, 1.0f);
    }
#endif

    if (bHit)
        return FVector::Dist(StartLocation, HitResult.ImpactPoint);

    return MaxDistance;
} // End GetTraceDistance()

//------------------------------------------------------------------------------
//                          UPDATE DISTANCE MEASUREMENTS (On-Demand)
//------------------------------------------------------------------------------
void AViewportNavigator::UpdateDistanceMeasurements()
{
    // Now only traces what's needed - called selectively by movement functions
    // This function kept for compatibility but individual traces done on-demand
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
