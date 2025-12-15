#include "SpatialBridge.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PawnMovementComponent.h"

/*====================================================================================================================================
                                                         SPATIAL BRIDGE SYSTEM
======================================================================================================================================*/

ASpatialBridge::ASpatialBridge()
{
	PrimaryActorTick.bCanEverTick = true;

	BridgeGeometry = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BridgeGeometry"));
	RootComponent = BridgeGeometry;
	
	PortalSurface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalSurface"));
	PortalSurface->SetupAttachment(RootComponent);
	
	PortalCamera = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PortalCamera"));
	PortalCamera->SetupAttachment(RootComponent);

	PortalDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("PortalDirection"));
	PortalDirection->SetupAttachment(RootComponent);

	PlaneIntersectionTest = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneIntersectionTest"));
	PlaneIntersectionTest->SetupAttachment(RootComponent);

	TeleportDetector = CreateDefaultSubobject<USphereComponent>(TEXT("TeleportDetector"));
	TeleportDetector->SetupAttachment(RootComponent);

	TeleportEntryDetector = CreateDefaultSubobject<UBoxComponent>(TEXT("TeleportEntryDetector"));
	TeleportEntryDetector->SetupAttachment(RootComponent);
}

void ASpatialBridge::BeginPlay()
{
	Super::BeginPlay();
	initializeBridgeRendering();
}

void ASpatialBridge::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	syncCaptureTransform();
	applyClipBounds();
	validateRenderResolution();
	CheckForTeleport();
}

//------------------------------------------------------------------------------
//                                   INITIALIZATION
//------------------------------------------------------------------------------

void ASpatialBridge::initializeBridgeRendering()
{
	UMaterial* ParentMat = LoadObject<UMaterial>(nullptr, TEXT("/Game/RIFT/Sceneccapture"));
	if (ParentMat)
	{
		DynamicMatInstance = UMaterialInstanceDynamic::Create(ParentMat, this);
		if (DynamicMatInstance && PortalSurface)
		{
			PortalSurface->SetMaterial(0, DynamicMatInstance);
		}
	}
	constructRenderTarget();
}

void ASpatialBridge::constructRenderTarget()
{
	FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(GetWorld());
	// Clamp minimum size to avoid crash on startup if viewport is 0
	int32 Width = FMath::Max(128, FMath::TruncToInt(ViewportSize.X));
	int32 Height = FMath::Max(128, FMath::TruncToInt(ViewportSize.Y));
	
	ScenecCapture_RT = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), Width, Height, RTF_RGBA16f);
	
	if (DynamicMatInstance && ScenecCapture_RT)
	{
		DynamicMatInstance->SetTextureParameterValue(FName("placeHolder"), ScenecCapture_RT);
	}
	
	if (LinkedPortal && LinkedPortal->PortalCamera && ScenecCapture_RT)
	{
		LinkedPortal->PortalCamera->TextureTarget = ScenecCapture_RT;
	}
}

//------------------------------------------------------------------------------
//                                   TICK LOGIC
//------------------------------------------------------------------------------

void ASpatialBridge::CheckForTeleport()
{
	if (!LinkedPortal || !PlaneIntersectionTest || !TeleportDetector) return;

	const FVector PortalLocation = PlaneIntersectionTest->GetComponentLocation();
	const FVector PortalNormal = PortalDirection->GetForwardVector();
	const double CurrentTime = FPlatformTime::Seconds();

	TArray<AActor*> ActorsInSphere;
	TeleportDetector->GetOverlappingActors(ActorsInSphere);

	// Iterate backwards so we can safely remove from map if needed
	for (int32 i = ActorsInSphere.Num() - 1; i >= 0; --i)
	{
		AActor* Actor = ActorsInSphere[i];
		if (!Actor || Actor->IsPendingKillPending()) continue;

		// Ensure we are tracking this actor
		FPortalTrackingState& State = TrackedActors.FindOrAdd(Actor);

		// COOLDOWN CHECK: prevent loops (0.1s cooldown)
		if (CurrentTime - State.LastTeleportTime < 0.1)
		{
			// Update position to prevent "crossing" detection while cooling down
			State.LastPosition = Actor->GetActorLocation();
			continue;
		}

		if (IsActorCrossingPortal(Actor, PortalLocation, PortalNormal))
		{
			TeleportActor(Actor);
			
			// Reset state after teleport
			State.LastTeleportTime = CurrentTime;
			State.LastPosition = Actor->GetActorLocation(); // Prevent double trigger
			State.bLastInFront = false; // Usually false after exit depending on vector math

			// Also set the cooldown on the LINKED portal for this actor
			if (LinkedPortal)
			{
				FPortalTrackingState& LinkState = LinkedPortal->TrackedActors.FindOrAdd(Actor);
				LinkState.LastTeleportTime = CurrentTime;
			}
		}
	}

	// Cleanup: Remove stale actors from the map that are no longer near the portal
	// (Optional optimization, can be done periodically instead of every tick)
}

bool ASpatialBridge::IsActorCrossingPortal(AActor* Actor, const FVector& PortalLocation, const FVector& PortalNormal)
{
	if (!Actor) return false;

	const FVector CurrentPosition = Actor->GetActorLocation();
	FPortalTrackingState& State = TrackedActors.FindOrAdd(Actor);
	
	const FVector VectorToPoint = CurrentPosition - PortalLocation;
	const float DotProduct = FVector::DotProduct(PortalNormal, VectorToPoint);
	const bool bIsInFront = DotProduct >= 0.0f;
	
	bool bIsIntersecting = false;
	FVector IntersectionPoint;
	float Time = 0.0f;
	
	// Only check intersection if we have a valid previous frame
	if (!State.LastPosition.IsZero())
	{
		bIsIntersecting = UKismetMathLibrary::LinePlaneIntersection(
			State.LastPosition,
			CurrentPosition,
			FPlane(PortalLocation, PortalNormal),
			Time,
			IntersectionPoint
		);	
	}
	
	// Logic: We intersected, we are NOW behind, and LAST frame we were in front
	const bool bIsCrossing = bIsIntersecting && !bIsInFront && State.bLastInFront;
		
	State.bLastInFront = bIsInFront;
	State.LastPosition = CurrentPosition;
	
	return bIsCrossing;
}

void ASpatialBridge::TeleportActor(AActor* ActorToTeleport)
{
	if (!ActorToTeleport || !LinkedPortal) return;

	// 1. Compute New Location
	// We add a small offset based on the linked portal's forward vector to ensure 
	// the physics engine doesn't detect collision *inside* the portal wall.
	FVector NewLocation = computeRelativePosition(ActorToTeleport->GetActorLocation());
	
	// 2. Compute New Rotation
	FRotator NewRotation;
	APawn* Pawn = Cast<APawn>(ActorToTeleport);
	
	if (Pawn && Pawn->GetController())
	{
		NewRotation = computeMirroredRotation(Pawn->GetController()->GetControlRotation());
		// Update Controller immediately
		Pawn->GetController()->SetControlRotation(NewRotation);
	}
	else
	{
		NewRotation = computeMirroredRotation(ActorToTeleport->GetActorRotation());
	}
	
	// 3. Perform Teleport
	// Note: For physics vehicles, we must NOT sweep.
	ActorToTeleport->SetActorLocationAndRotation(NewLocation, NewRotation, false, nullptr, ETeleportType::TeleportPhysics);

	// 4. PHYSICS VELOCITY UPDATE (Essential for Vehicles)
	UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(ActorToTeleport->GetRootComponent());
	
	if (PrimComp && PrimComp->IsSimulatingPhysics())
	{
		// Transform Linear Velocity
		FVector OldLinear = PrimComp->GetPhysicsLinearVelocity();
		FVector NewLinear = computeMirroredVector(OldLinear);
		PrimComp->SetPhysicsLinearVelocity(NewLinear);

		// Transform Angular Velocity
		// Angular velocity is a vector representing the axis of rotation. 
		// We need to rotate this axis into the new space.
		FVector OldAngular = PrimComp->GetPhysicsAngularVelocityInRadians();
		FVector NewAngular = computeMirroredVector(OldAngular);
		PrimComp->SetPhysicsAngularVelocityInRadians(NewAngular);
	}
	// Fallback for non-physics movement components (e.g. FloatingPawnMovement)
	else if (Pawn && Pawn->GetMovementComponent())
	{
		Pawn->GetMovementComponent()->Velocity = computeMirroredVector(Pawn->GetMovementComponent()->Velocity);
	}

	// 5. Camera Cut
	APlayerCameraManager* CamMgr = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	if (CamMgr && Pawn && Pawn->IsPlayerControlled())
	{
		CamMgr->SetGameCameraCutThisFrame();
	}
}

//------------------------------------------------------------------------------
//                                   RENDERING HELPERS
//------------------------------------------------------------------------------

void ASpatialBridge::syncCaptureTransform()
{
	if (!LinkedPortal || !LinkedPortal->PortalCamera) return;
	
	APlayerCameraManager* CamMgr = fetchPlayerCamera();
	if (!CamMgr) return;
	
	// Move the capture component to the "Virtual" position relative to the linked portal
	const FVector PlayerCamLoc = CamMgr->GetTransformComponent()->GetComponentLocation();
	const FRotator PlayerCamRot = CamMgr->GetTransformComponent()->GetComponentRotation();
	
	const FVector RelativeLoc = computeRelativePosition(PlayerCamLoc);
	const FRotator MirroredRot = computeMirroredRotation(PlayerCamRot);
	
	LinkedPortal->PortalCamera->SetWorldLocationAndRotation(RelativeLoc, MirroredRot);
}

void ASpatialBridge::applyClipBounds()
{
	if (!LinkedPortal || !LinkedPortal->PortalCamera) return;
	
	LinkedPortal->PortalCamera->bEnableClipPlane = true;
	const FVector PortalLoc = PlaneIntersectionTest->GetComponentLocation();
	const FVector PortalFwd = PortalDirection->GetForwardVector();
	
	// Push the clip plane slightly forward to avoid artifacts
	LinkedPortal->PortalCamera->ClipPlaneBase = PortalLoc + (PortalFwd * -1.0f);
	LinkedPortal->PortalCamera->ClipPlaneNormal = PortalFwd;
}

void ASpatialBridge::validateRenderResolution()
{
	if (!ScenecCapture_RT) return;
	
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(GetWorld());
	// Ensure we don't resize to 0
	const int32 TargetWidth = FMath::Max(1, FMath::TruncToInt(ViewportSize.X));
	const int32 TargetHeight = FMath::Max(1, FMath::TruncToInt(ViewportSize.Y));
	
	if (ScenecCapture_RT->SizeX != TargetWidth || ScenecCapture_RT->SizeY != TargetHeight)
	{
		UKismetRenderingLibrary::ResizeRenderTarget2D(ScenecCapture_RT, TargetWidth, TargetHeight);
	}
}

//------------------------------------------------------------------------------
//                                   MATH
//------------------------------------------------------------------------------

FVector ASpatialBridge::computeRelativePosition(const FVector& Point) const
{
	if (!LinkedPortal) return Point;

	// Transform Point from World -> ThisPortal Local -> Mirror -> LinkedPortal Local -> World
	const FTransform SelfTransform = GetTransform();
	const FTransform LinkTransform = LinkedPortal->GetTransform();
	
	// Calculate local coordinates relative to this portal
	const FVector LocalPos = UKismetMathLibrary::InverseTransformLocation(SelfTransform, Point);
	
	// Mirror coordinates (Standard portal flipping is -X, -Y, Z or similar depending on setup)
	// Assuming portals face +X, we flip X and Y to turn around.
	// Froyok's method mirrors specifically by axes.
	const FVector MirroredPos = FVector(-LocalPos.X, -LocalPos.Y, LocalPos.Z);
	
	// Transform local mirrored point to world space of linked portal
	return UKismetMathLibrary::TransformLocation(LinkTransform, MirroredPos);
}

FRotator ASpatialBridge::computeMirroredRotation(const FRotator& Rotation) const
{
	if (!LinkedPortal) return Rotation;

	// Froyok Method: Transform basis vectors
	FVector Fwd, Right, Up;
	UKismetMathLibrary::BreakRotIntoAxes(Rotation, Fwd, Right, Up);
	
	// Transform vectors (Direction, not Location)
	Fwd = computeMirroredVector(Fwd);
	Right = computeMirroredVector(Right);
	Up = computeMirroredVector(Up);
	
	return UKismetMathLibrary::MakeRotationFromAxes(Fwd, Right, Up);
}

FVector ASpatialBridge::computeMirroredVector(const FVector& Vector) const
{
	// This function handles Velocity and Direction vectors
	if (!LinkedPortal) return Vector;

	const FTransform SelfTransform = GetTransform();
	const FTransform LinkTransform = LinkedPortal->GetTransform();
	
	// 1. Inverse transform to get local direction
	FVector LocalDir = UKismetMathLibrary::InverseTransformDirection(SelfTransform, Vector);
	
	// 2. Mirror (Flip X and Y for standard back-to-back portals)
	LocalDir.X = -LocalDir.X;
	LocalDir.Y = -LocalDir.Y;
	// Z stays same (up is still up)
	
	// 3. Transform direction to new portal world space
	return UKismetMathLibrary::TransformDirection(LinkTransform, LocalDir);
}

APlayerCameraManager* ASpatialBridge::fetchPlayerCamera() const
{
	return UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
}