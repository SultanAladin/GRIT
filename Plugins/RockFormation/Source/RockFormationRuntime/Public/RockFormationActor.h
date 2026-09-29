#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "ProceduralMeshComponent.h"
#include "RockFormationTypes.h"
#include "RockGeometry.h"
#include "RockFormationActor.generated.h"

UCLASS()
class ROCKFORMATIONRUNTIME_API ARockFormationActor : public AActor
{
	GENERATED_BODY()

public:
	ARockFormationActor();

	// --- Spline references ---
	UPROPERTY(EditAnywhere, Category="Rock Formation|Curves")
	TArray<TSoftObjectPtr<AActor>> SplineActors;

	// --- Main rocks ---
	UPROPERTY(EditAnywhere, Category="Rock Formation|Shape", meta=(ClampMin="0"))
	int32 Seed = 42;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Shape")
	ERockBaseShape BaseShape = ERockBaseShape::Icosahedron;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Shape", meta=(ClampMin="10.0"))
	float Spacing = 150.f;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Shape", meta=(ClampMin="0.0", ClampMax="2.0"))
	float ScaleRandomness = 0.5f;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Shape", meta=(ClampMin="0.0", ClampMax="2.0"))
	float JitterAmount = 0.2f;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Shape", meta=(ClampMin="1.0", ClampMax="10.0"))
	float MaxStretch = 2.f;

	// --- Noise ---
	UPROPERTY(EditAnywhere, Category="Rock Formation|Noise")
	ERockNoiseType NoiseType = ERockNoiseType::Simplex;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Noise", meta=(ClampMin="0.0", ClampMax="2.0"))
	float Roughness = 0.4f;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Noise", meta=(ClampMin="1", ClampMax="5"))
	int32 Detail = 3;

	// --- Appearance ---
	UPROPERTY(EditAnywhere, Category="Rock Formation|Appearance")
	UMaterialInterface* RockMaterial = nullptr;

	// --- Side layer ---
	UPROPERTY(EditAnywhere, Category="Rock Formation|Side Layer")
	bool bEnableSideLayer = false;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Side Layer", meta=(ClampMin="1", ClampMax="10", EditCondition="bEnableSideLayer"))
	int32 SideRocksPerPoint = 2;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Side Layer", meta=(ClampMin="10.0", EditCondition="bEnableSideLayer"))
	float SideSpacing = 100.f;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Side Layer", meta=(ClampMin="0.0", EditCondition="bEnableSideLayer"))
	float SideMinDistance = 50.f;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Side Layer", meta=(ClampMin="1.0", EditCondition="bEnableSideLayer"))
	float SideMaxDistance = 200.f;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Side Layer", meta=(ClampMin="0.01", ClampMax="5.0", EditCondition="bEnableSideLayer"))
	float SideScale = 0.5f;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Side Layer", meta=(ClampMin="0.0", ClampMax="2.0", EditCondition="bEnableSideLayer"))
	float SideScaleRandomness = 0.6f;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Side Layer", meta=(ClampMin="0.0", ClampMax="2.0", EditCondition="bEnableSideLayer"))
	float SideJitter = 0.5f;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Side Layer", meta=(ClampMin="0.0", ClampMax="1.0", EditCondition="bEnableSideLayer"))
	float SideCoverage = 1.f;

	// --- Growth layer ---
	UPROPERTY(EditAnywhere, Category="Rock Formation|Growth Layer")
	bool bEnableGrowthLayer = false;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Growth Layer", meta=(ClampMin="1", ClampMax="15", EditCondition="bEnableGrowthLayer"))
	int32 GrowthPerPoint = 3;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Growth Layer", meta=(ClampMin="10.0", EditCondition="bEnableGrowthLayer"))
	float GrowthSpacing = 100.f;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Growth Layer", meta=(ClampMin="0.0", ClampMax="1.0", EditCondition="bEnableGrowthLayer"))
	float GrowthCoverage = 0.6f;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Growth Layer", meta=(ClampMin="0.0", EditCondition="bEnableGrowthLayer"))
	float GrowthMinDist = 0.f;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Growth Layer", meta=(ClampMin="1.0", EditCondition="bEnableGrowthLayer"))
	float GrowthMaxDist = 100.f;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Growth Layer", meta=(ClampMin="0.01", ClampMax="5.0", EditCondition="bEnableGrowthLayer"))
	float GrowthScale = 0.35f;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Growth Layer", meta=(ClampMin="0.0", ClampMax="2.0", EditCondition="bEnableGrowthLayer"))
	float GrowthScaleRandomness = 0.7f;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Growth Layer", meta=(EditCondition="bEnableGrowthLayer"))
	bool bGrowthSides = true;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Growth Layer", meta=(EditCondition="bEnableGrowthLayer"))
	bool bGrowthTop = true;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Growth Layer", meta=(EditCondition="bEnableGrowthLayer"))
	bool bGrowthBottom = false;

	// --- Base rocks ---
	UPROPERTY(EditAnywhere, Category="Rock Formation|Base Rocks")
	bool bEnableBaseRocks = false;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Base Rocks", meta=(ClampMin="1", ClampMax="100", EditCondition="bEnableBaseRocks"))
	int32 BaseRocksDensity = 10;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Base Rocks", meta=(ClampMin="10.0", EditCondition="bEnableBaseRocks"))
	float BaseRocksSpread = 500.f;

	UPROPERTY(EditAnywhere, Category="Rock Formation|Base Rocks", meta=(ClampMin="0.01", ClampMax="5.0", EditCondition="bEnableBaseRocks"))
	float BaseRocksScale = 0.5f;

	// --- Auto rebuild ---
	UPROPERTY(EditAnywhere, Category="Rock Formation")
	bool bAutoRebuild = true;

	// --- Status ---
	UPROPERTY(VisibleAnywhere, Category="Rock Formation|Status")
	int32 RockCount = 0;

	UPROPERTY(VisibleAnywhere, Category="Rock Formation|Status")
	FString StatusReport;

	// --- Functions ---
	UFUNCTION(CallInEditor, Category="Rock Formation")
	void RebuildFormation();

	UFUNCTION(CallInEditor, Category="Rock Formation")
	void BakeMesh();

	UFUNCTION(CallInEditor, Category="Rock Formation")
	void ClearFormation();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif

private:
	UPROPERTY()
	USceneComponent* Root = nullptr;

	UPROPERTY()
	UProceduralMeshComponent* GeneratedMesh = nullptr;

	bool bIsRebuilding = false;

	// Gather spline points from referenced actors
	TArray<TArray<FVector>> GatherSplinePolylines() const;

	// Spline helpers
	static float PolylineLength(const TArray<FVector>& Pts);
	static void PointAtT(const TArray<FVector>& Pts, float T,
	                      FVector& OutPos, FVector& OutTangent);

	// Add a transformed rock into combined arrays
	void AddRockToCombined(TArray<FVector>& AllVerts, TArray<int32>& AllTris,
	                       TArray<FVector>& AllNormals, TArray<FVector2D>& AllUVs,
	                       const FRockMeshData& Rock,
	                       const FVector& Location, const FRotator& Rotation,
	                       const FVector& Scale) const;
};
