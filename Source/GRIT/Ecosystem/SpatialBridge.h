#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpatialBridge.generated.h"

// Forward declarations
class UStaticMeshComponent;
class USphereComponent;
class UBoxComponent;
class UArrowComponent;
class USceneCaptureComponent2D;
class UMaterialInstanceDynamic;
class UTextureRenderTarget2D;
class APlayerCameraManager;

/**
 * @struct FPortalTrackingState
 * @brief Stores the state of an actor relative to the portal plane.
 * Now includes cooldowns to allow multiple vehicles to use the portal simultaneously.
 */
USTRUCT()
struct FPortalTrackingState
{
	GENERATED_BODY()

	/** The actor's position in the last frame. */
	UPROPERTY()
	FVector LastPosition = FVector::ZeroVector;

	/** Whether the actor was in front of the portal plane in the last frame. */
	UPROPERTY()
	bool bLastInFront = false;

	/** Timestamp of the last teleport to prevent immediate re-entry loops. */
	UPROPERTY()
	double LastTeleportTime = 0.0;

	FPortalTrackingState() {}
};

UCLASS()
class GRIT_API ASpatialBridge : public AActor
{
	GENERATED_BODY()

public:
	ASpatialBridge();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	//------------------------------------------------------------------------------
	//                                   CORE COMPONENTS
	//------------------------------------------------------------------------------
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bridge|Components")
	UArrowComponent* PortalDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bridge|Components")
	UStaticMeshComponent* BridgeGeometry; 
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bridge|Components")
	UStaticMeshComponent* PortalSurface; 

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bridge|Components")
	UStaticMeshComponent* PlaneIntersectionTest;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bridge|Components")
	USceneCaptureComponent2D* PortalCamera; 
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bridge|Components")
	USphereComponent* TeleportDetector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bridge|Components")
	UBoxComponent* TeleportEntryDetector;

	//------------------------------------------------------------------------------
	//                                   BRIDGE CONFIGURATION
	//------------------------------------------------------------------------------
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bridge|Configuration")
	ASpatialBridge* LinkedPortal; 
	
	UPROPERTY(BlueprintReadOnly, Category = "Bridge|Rendering")
	UMaterialInstanceDynamic* DynamicMatInstance; 
	
	UPROPERTY(BlueprintReadOnly, Category = "Bridge|Rendering")
	UTextureRenderTarget2D* ScenecCapture_RT; 

private:
	//------------------------------------------------------------------------------
	//                                   STATE TRACKING
	//------------------------------------------------------------------------------

	/** * Tracked actors and their states. 
	 * Uses TWeakObjectPtr to safely handle actors being destroyed during play.
	 */
	TMap<TWeakObjectPtr<AActor>, FPortalTrackingState> TrackedActors;

	//------------------------------------------------------------------------------
	//                                   INTERNAL LOGIC
	//------------------------------------------------------------------------------
	
	void initializeBridgeRendering();
	void constructRenderTarget();
	void CheckForTeleport();

	/** Checks crossing and handles the per-actor state updates. */
	bool IsActorCrossingPortal(AActor* Actor, const FVector& PortalLocation, const FVector& PortalNormal);

	/** * Teleports actor and specifically handles UPrimitiveComponent Physics 
	 * for vehicles (Angular/Linear Velocity).
	 */
	void TeleportActor(AActor* ActorToTeleport);

	void syncCaptureTransform();
	void applyClipBounds();
	void validateRenderResolution();

	//------------------------------------------------------------------------------
	//                                   MATH UTILITIES
	//------------------------------------------------------------------------------
	
	FVector computeRelativePosition(const FVector& Point) const;
	FRotator computeMirroredRotation(const FRotator& Rotation) const;
	FVector computeMirroredVector(const FVector& Vector) const;
	
	APlayerCameraManager* fetchPlayerCamera() const;
};