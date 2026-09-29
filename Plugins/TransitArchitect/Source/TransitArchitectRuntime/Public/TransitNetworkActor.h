// TransitNetworkActor.h — Top-level actor that orchestrates the transit network pipeline
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TransitTypes.h"
#include "TransitGraph.h"
#include "TransitNetworkActor.generated.h"

class UProceduralMeshComponent;
class UTransitSplineComponent;
class USceneComponent;

UCLASS(BlueprintType, Blueprintable)
class TRANSITARCHITECTRUNTIME_API ATransitNetworkActor : public AActor
{
	GENERATED_BODY()

public:
	ATransitNetworkActor();

	// ── Network settings ────────────────────────────────────────────────

	/** XY merge distance for detecting junction nodes (cm). Default: 300 cm = 3m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Settings", meta = (ClampMin = "10"))
	float NodeXYMergeDistance = 300.0f;

	/** Polyline sample step along splines (cm). Default: 200 cm = 2m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Settings", meta = (ClampMin = "10"))
	float PolylineSampleStep = 200.0f;

	/** Z threshold for merging projected XY crossings (cm). Default: 150 cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Settings", meta = (ClampMin = "1"))
	float ZMergeThreshold = 150.0f;

	/** Auto-rebuild when splines are edited. Disable for large networks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Settings")
	bool bAutoRebuild = true;

	/** Render both sides of all faces (duplicates geometry with reversed winding) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Settings")
	bool bDoubleSided = false;

	/** Flip all face normals (reverses triangle winding order) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Settings")
	bool bFlipNormals = false;

	// ── Default profile ─────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Profile")
	ETransitPreset DefaultProfile = ETransitPreset::Street;

	// ── Materials ────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Materials")
	UMaterialInterface* RoadSurfaceMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Materials")
	UMaterialInterface* CurbMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Materials")
	UMaterialInterface* PavementMaterial = nullptr;

	// ── Junction overrides ──────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Junctions")
	TMap<FString, FJunctionOverride> JunctionOverrides;

	// ── Status ──────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transit Architect|Status")
	FString StatusReport;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transit Architect|Status")
	int32 EdgeCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transit Architect|Status")
	int32 JunctionCount = 0;

	// ── Actions ─────────────────────────────────────────────────────────

	/** Rebuild the entire network from spline data */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Transit Architect")
	void RebuildNetwork();

	/** Add a new transit spline component to this network */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Transit Architect")
	UTransitSplineComponent* AddTransitSpline();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY()
	USceneComponent* Root = nullptr;

	UPROPERTY()
	TArray<UProceduralMeshComponent*> GeneratedMeshComponents;

	// Pool of cleared PMCs for reuse (avoids DestroyComponent crashes in UE 5.7)
	UPROPERTY()
	TArray<UProceduralMeshComponent*> PMCPool;

	// Re-entrancy guard — prevents recursive rebuild from OnConstruction/PostEditChange
	bool bIsRebuilding = false;

	// Cached graph (transient)
	FTransitGraph CachedGraph;

	void ClearGeneratedMesh();
	void PurgeStalePoolEntries();
	void GenerateAllMesh();
	UProceduralMeshComponent* AcquirePMC();
	TArray<UTransitSplineComponent*> GatherSplines() const;
	void ApplyJunctionOverrides();
};
