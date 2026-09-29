// RoadIntersection.h — Graph node actor for road network intersections
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoadTypes.h"
#include "RoadIntersection.generated.h"

class ARoadSegment;
class UStaticMeshComponent;

/**
 * A road intersection node placed in the editor.
 * The user assigns a static mesh (2/3/4-way) with named sockets (Road_A, Road_B, etc.).
 * Road segments are spawned between socket pairs on different intersections.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIT_API ARoadIntersection : public AActor
{
	GENERATED_BODY()

public:
	ARoadIntersection();

	//--- Components -------------------------------------------------------

	/** Intersection mesh — user assigns a 2/3/4-way mesh with sockets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road Network")
	UStaticMeshComponent* IntersectionMesh;

	//--- Connections ------------------------------------------------------

	/** Auto-populated from mesh sockets. Shows which sockets are connected. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Road Network")
	TArray<FSocketConnection> Connections;

	//--- Defaults for spawned segments ------------------------------------

	/** Default road tile mesh for segments spawned from this intersection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road Network|Defaults")
	UStaticMesh* DefaultRoadMesh = nullptr;

	/** Default material applied to road segments */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road Network|Defaults")
	UMaterialInterface* DefaultRoadMaterial = nullptr;

	/** Default tile length (cm) for road mesh segments */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road Network|Defaults", meta = (ClampMin = "10.0"))
	float DefaultSegmentLength = 500.0f;

	//--- Methods ----------------------------------------------------------

	/** Scans the mesh for sockets and rebuilds the Connections array (preserves existing links) */
	UFUNCTION(BlueprintCallable, Category = "Road Network")
	void RefreshSockets();

	/** Returns socket names that have no connected segment */
	UFUNCTION(BlueprintCallable, Category = "Road Network")
	TArray<FName> GetAvailableSockets() const;

	/**
	 * Spawns a road segment between LocalSocket on this intersection and OtherSocket on Other.
	 * Returns the new segment, or nullptr if sockets are invalid / already occupied.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Road Network")
	ARoadSegment* ConnectTo(FName LocalSocket, ARoadIntersection* Other, FName OtherSocket);

	/** Destroys the road segment on SocketName and clears connection on both ends */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Road Network")
	void Disconnect(FName SocketName);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
