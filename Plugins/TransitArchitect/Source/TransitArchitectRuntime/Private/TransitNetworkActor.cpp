// TransitNetworkActor.cpp — Transit network orchestrator implementation
#include "TransitNetworkActor.h"
#include "TransitSplineComponent.h"
#include "TransitGraphBuilder.h"
#include "TransitGeometry.h"
#include "TransitMeshSpec.h"
#include "TransitArchitectRuntime.h"
#include "ProceduralMeshComponent.h"
#include "Components/SceneComponent.h"

ATransitNetworkActor::ATransitNetworkActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

// ═══════════════════════════════════════════════════════════════════════════
// Gather child splines (filters out pending-kill / invalid components)
// ═══════════════════════════════════════════════════════════════════════════

TArray<UTransitSplineComponent*> ATransitNetworkActor::GatherSplines() const
{
	TArray<UTransitSplineComponent*> Splines;
	TArray<UActorComponent*> Components;
	GetComponents(UTransitSplineComponent::StaticClass(), Components);
	for (UActorComponent* Comp : Components)
	{
		if (!IsValid(Comp)) continue;
		if (UTransitSplineComponent* TSC = Cast<UTransitSplineComponent>(Comp))
		{
			if (TSC->GetNumberOfSplinePoints() >= 2)
			{
				Splines.Add(TSC);
			}
		}
	}
	return Splines;
}

// ═══════════════════════════════════════════════════════════════════════════
// Add a new spline
// ═══════════════════════════════════════════════════════════════════════════

UTransitSplineComponent* ATransitNetworkActor::AddTransitSpline()
{
	if (!IsValid(this) || !IsValid(RootComponent)) return nullptr;

	FName SplineName = MakeUniqueObjectName(this, UTransitSplineComponent::StaticClass(), TEXT("TransitSpline"));
	UTransitSplineComponent* NewSpline = NewObject<UTransitSplineComponent>(this, SplineName);
	if (!NewSpline) return nullptr;

	NewSpline->SetupAttachment(RootComponent);
	NewSpline->RegisterComponent();
	NewSpline->SetVisibility(true);
	NewSpline->SetHiddenInGame(false);
	NewSpline->bDrawDebug = true;
	NewSpline->ProfilePreset = DefaultProfile;

	// Initialize with 2 points forming a 1000cm straight segment
	NewSpline->ClearSplinePoints(false);
	int32 SplineCount = GatherSplines().Num();
	float Offset = (SplineCount - 1) * 500.0f; // Offset each new spline so they don't overlap
	NewSpline->AddSplinePoint(FVector(0, Offset, 0), ESplineCoordinateSpace::Local, false);
	NewSpline->AddSplinePoint(FVector(1000, Offset, 0), ESplineCoordinateSpace::Local, true);

	StatusReport = FString::Printf(TEXT("Added spline '%s'. Total: %d"), *SplineName.ToString(), SplineCount);

	return NewSpline;
}

// ═══════════════════════════════════════════════════════════════════════════
// PMC pool — never DestroyComponent, only hide + reuse
// ═══════════════════════════════════════════════════════════════════════════

void ATransitNetworkActor::ClearGeneratedMesh()
{
	for (UProceduralMeshComponent* PMC : GeneratedMeshComponents)
	{
		if (IsValid(PMC))
		{
			PMC->ClearAllMeshSections();
			PMC->SetVisibility(false);
			PMCPool.Add(PMC);
		}
	}
	GeneratedMeshComponents.Reset();
}

void ATransitNetworkActor::PurgeStalePoolEntries()
{
	// Remove any pool entries that became invalid (e.g. after undo)
	PMCPool.RemoveAll([](UProceduralMeshComponent* PMC) { return !IsValid(PMC); });
	GeneratedMeshComponents.RemoveAll([](UProceduralMeshComponent* PMC) { return !IsValid(PMC); });
}

UProceduralMeshComponent* ATransitNetworkActor::AcquirePMC()
{
	// Try to reuse a pooled PMC
	while (PMCPool.Num() > 0)
	{
		UProceduralMeshComponent* PMC = PMCPool.Pop();
		if (!IsValid(PMC)) continue; // Skip GC'd or pending-kill entries

		PMC->ClearAllMeshSections();
		PMC->SetVisibility(true);
		PMC->SetWorldLocation(FVector::ZeroVector);
		PMC->SetWorldRotation(FRotator::ZeroRotator);
		return PMC;
	}

	// No valid pooled PMC — create fresh. Use unique name via counter to avoid collisions.
	FName PMCName = MakeUniqueObjectName(this, UProceduralMeshComponent::StaticClass(), TEXT("TransitMesh"));
	UProceduralMeshComponent* PMC = NewObject<UProceduralMeshComponent>(this, PMCName);
	if (!PMC) return nullptr;

	PMC->SetupAttachment(RootComponent);
	PMC->RegisterComponent();
	PMC->SetVisibility(true);
	PMC->SetWorldLocation(FVector::ZeroVector);
	PMC->SetWorldRotation(FRotator::ZeroRotator);
	return PMC;
}

// ═══════════════════════════════════════════════════════════════════════════
// Apply junction overrides from the TMap
// ═══════════════════════════════════════════════════════════════════════════

void ATransitNetworkActor::ApplyJunctionOverrides()
{
	for (auto& Pair : CachedGraph.Nodes)
	{
		FJunctionNode& Node = Pair.Value;
		if (const FJunctionOverride* Override = JunctionOverrides.Find(Node.NodeId))
		{
			Node.bJunctionEnabled = Override->bEnabled && !Override->bForceIgnore;
			Node.bForceIgnore = Override->bForceIgnore;
			Node.NodeType = Override->NodeType;
			if (Override->CornerRadiusOverride > 0.0f)
			{
				Node.CornerRadius = Override->CornerRadiusOverride;
			}
		}
	}
}

// ═══════════════════════════════════════════════════════════════════════════
// Generate all mesh from cached graph
// ═══════════════════════════════════════════════════════════════════════════

static void ApplyMeshSpecToPMC(UProceduralMeshComponent* PMC,
	const FTransitMeshSpec& RoadSpec, const FTransitMeshSpec& PaveSpec,
	UMaterialInterface* RoadMat, UMaterialInterface* CurbMat, UMaterialInterface* PaveMat)
{
	if (!PMC) return;

	// Section 0: Road surface + curb (both in the road spec)
	if (!RoadSpec.IsEmpty())
	{
		PMC->CreateMeshSection_LinearColor(
			TransitMaterial::RoadSurface,
			RoadSpec.Vertices, RoadSpec.Triangles, RoadSpec.Normals,
			RoadSpec.UVs, RoadSpec.VertexColors, RoadSpec.Tangents, true);
		if (RoadMat)
		{
			PMC->SetMaterial(TransitMaterial::RoadSurface, RoadMat);
		}
	}

	// Section 2: Pavement
	if (!PaveSpec.IsEmpty())
	{
		PMC->CreateMeshSection_LinearColor(
			TransitMaterial::Pavement,
			PaveSpec.Vertices, PaveSpec.Triangles, PaveSpec.Normals,
			PaveSpec.UVs, PaveSpec.VertexColors, PaveSpec.Tangents, true);
		if (PaveMat)
		{
			PMC->SetMaterial(TransitMaterial::Pavement, PaveMat);
		}
	}
}

void ATransitNetworkActor::GenerateAllMesh()
{
	auto PostProcessSpecs = [this](FTransitMeshSpec& Road, FTransitMeshSpec& Pave)
	{
		if (bFlipNormals)
		{
			Road.FlipWinding();
			Pave.FlipWinding();
		}
		if (bDoubleSided)
		{
			Road.MakeDoubleSided();
			Pave.MakeDoubleSided();
		}
	};

	// Generate segment meshes
	for (const auto& EdgePair : CachedGraph.Edges)
	{
		const FCorridorEdge& Edge = EdgePair.Value;

		FTransitMeshSpec RoadSpec, PaveSpec;
		FTransitGeometry::BuildSegmentMesh(CachedGraph, Edge, RoadSpec, PaveSpec);
		PostProcessSpecs(RoadSpec, PaveSpec);

		if (RoadSpec.IsEmpty() && PaveSpec.IsEmpty()) continue;

		UProceduralMeshComponent* PMC = AcquirePMC();
		if (!PMC) continue;
		ApplyMeshSpecToPMC(PMC, RoadSpec, PaveSpec, RoadSurfaceMaterial, CurbMaterial, PavementMaterial);
		GeneratedMeshComponents.Add(PMC);
	}

	// Generate junction meshes
	for (const auto& NodePair : CachedGraph.Nodes)
	{
		const FJunctionNode& Node = NodePair.Value;
		if (!FTransitGeometry::NodeGeneratesJunction(CachedGraph, Node)) continue;

		FTransitMeshSpec RoadSpec, PaveSpec;
		FTransitGeometry::BuildJunctionMesh(CachedGraph, Node, RoadSpec, PaveSpec);
		PostProcessSpecs(RoadSpec, PaveSpec);

		if (RoadSpec.IsEmpty() && PaveSpec.IsEmpty()) continue;

		UProceduralMeshComponent* PMC = AcquirePMC();
		if (!PMC) continue;
		ApplyMeshSpecToPMC(PMC, RoadSpec, PaveSpec, RoadSurfaceMaterial, CurbMaterial, PavementMaterial);
		GeneratedMeshComponents.Add(PMC);
	}
}

// ═══════════════════════════════════════════════════════════════════════════
// Rebuild network (safe entry point)
// ═══════════════════════════════════════════════════════════════════════════

void ATransitNetworkActor::RebuildNetwork()
{
	if (!IsValid(this)) return;
	if (bIsRebuilding) return; // Guard against re-entrancy

	bIsRebuilding = true;

	PurgeStalePoolEntries();
	ClearGeneratedMesh();

	TArray<UTransitSplineComponent*> Splines = GatherSplines();
	if (Splines.Num() == 0)
	{
		StatusReport = TEXT("No splines. Use 'Add Transit Spline' to begin.");
		EdgeCount = 0;
		JunctionCount = 0;
		bIsRebuilding = false;
		return;
	}

	FTransitGraphBuilder Builder;
	Builder.NodeXYMergeDistance = NodeXYMergeDistance;
	Builder.PolylineSampleStep = PolylineSampleStep;
	Builder.ZMergeThreshold = ZMergeThreshold;

	CachedGraph = Builder.BuildGraph(Splines);

	ApplyJunctionOverrides();
	GenerateAllMesh();

	// Count junctions
	int32 JuncCount = 0;
	for (const auto& Pair : CachedGraph.Nodes)
	{
		if (FTransitGeometry::NodeGeneratesJunction(CachedGraph, Pair.Value))
		{
			++JuncCount;
		}
	}

	EdgeCount = CachedGraph.Edges.Num();
	JunctionCount = JuncCount;
	StatusReport = FString::Printf(TEXT("Built %d corridors, %d junctions from %d splines."),
		EdgeCount, JunctionCount, Splines.Num());

	UE_LOG(LogTransitArchitect, Log, TEXT("%s"), *StatusReport);

	bIsRebuilding = false;
}

// ═══════════════════════════════════════════════════════════════════════════
// Auto-rebuild hooks (guarded for safety)
// ═══════════════════════════════════════════════════════════════════════════

void ATransitNetworkActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bAutoRebuild && !bIsRebuilding)
	{
		RebuildNetwork();
	}
}

#if WITH_EDITOR
void ATransitNetworkActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (bAutoRebuild && !bIsRebuilding)
	{
		RebuildNetwork();
	}
}
#endif
