// RoadSegment.h — Spline-based road mesh between two intersection sockets
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoadTypes.h"
#include "RoadSegment.generated.h"

class ARoadIntersection;
class USplineComponent;
class USplineMeshComponent;

/**
 * A road segment that tiles a mesh along a spline between two intersection sockets.
 * Spawned by ARoadIntersection::ConnectTo(). Supports feature overrides (potholes, etc.).
 */
UCLASS(BlueprintType, Blueprintable)
class GRIT_API ARoadSegment : public AActor
{
	GENERATED_BODY()

public:
	ARoadSegment();

	//--- Components -------------------------------------------------------

	/** Spline defining the road path between two intersections */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Road Network")
	USplineComponent* RoadSpline;

	//--- Mesh configuration -----------------------------------------------

	/** Base road tile mesh to deform along the spline */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road Network")
	UStaticMesh* RoadMesh = nullptr;

	/** Material applied to road mesh segments */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road Network")
	UMaterialInterface* RoadMaterial = nullptr;

	/** Length (cm) of each tiled mesh segment along the spline */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road Network", meta = (ClampMin = "10.0"))
	float SegmentLength = 500.0f;

	//--- Features ---------------------------------------------------------

	/** Road surface features (potholes, cracks, etc.) placed at specific spline distances */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road Network|Features")
	TArray<FRoadFeature> Features;

	//--- Connection references (set by Initialize) ------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Road Network|Connection")
	ARoadIntersection* StartIntersection = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Road Network|Connection")
	FName StartSocket;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Road Network|Connection")
	ARoadIntersection* EndIntersection = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Road Network|Connection")
	FName EndSocket;

	//--- Methods ----------------------------------------------------------

	/** Called by ARoadIntersection::ConnectTo to wire up this segment */
	void Initialize(ARoadIntersection* Start, FName StartSock, ARoadIntersection* End, FName EndSock);

	/** Destroys existing mesh segments and fully rebuilds spline + mesh */
	UFUNCTION(BlueprintCallable, Category = "Road Network")
	void RebuildRoad();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	/** Active spline mesh components (rebuilt each time) */
	UPROPERTY()
	TArray<USplineMeshComponent*> MeshSegments;

	/** Configures spline points from start/end socket world transforms */
	void BuildSpline();

	/** Tiles mesh segments along the spline, applying feature overrides */
	void BuildMesh();
};
