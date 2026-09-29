// RoadTypes.h — Shared enums and data structs for the road network system
#pragma once

#include "CoreMinimal.h"
#include "RoadTypes.generated.h"

class ARoadSegment;

/** Type of road surface feature (damage, decoration, etc.) */
UENUM(BlueprintType)
enum class ERoadFeatureType : uint8
{
	Pothole,
	Hole,
	Crack,
	Custom
};

/**
 * Describes a feature placed at a specific distance along a road spline.
 * Features can override the mesh/material for the segment they occupy.
 */
USTRUCT(BlueprintType)
struct FRoadFeature
{
	GENERATED_BODY()

	/** Distance along the road spline (cm) where this feature is placed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float SplineDistance = 0.0f;

	/** The type of road feature */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERoadFeatureType Type = ERoadFeatureType::Pothole;

	/** If set, replaces the default road tile mesh at this location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* OverrideMesh = nullptr;

	/** If set, overrides the material on this feature's mesh segment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* OverrideMaterial = nullptr;

	/** Scale applied to the feature mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Scale = FVector(1.0f);

	/** How much spline distance (cm) this feature occupies. 0 = one segment only */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float Length = 0.0f;
};

/**
 * Maps a socket on an intersection mesh to the road segment connected through it.
 */
USTRUCT(BlueprintType)
struct FSocketConnection
{
	GENERATED_BODY()

	/** Name of the socket on the intersection mesh (e.g. "Road_A") */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName SocketName;

	/** The road segment connected through this socket (nullptr if unoccupied) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ARoadSegment* ConnectedSegment = nullptr;
};
