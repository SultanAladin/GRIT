// TransitGraph.h — Graph data structures for transit network topology
#pragma once

#include "CoreMinimal.h"
#include "TransitTypes.h"

// Forward declaration
class UTransitSplineComponent;

// ── Sampled corridor (one spline → polyline) ────────────────────────────────

struct TRANSITARCHITECTRUNTIME_API FSampledCorridor
{
	FString SourceId;
	FString SplineKey;
	FResolvedTransitProfile Profile;
	ETransitFamily Family       = ETransitFamily::Road;
	ETransitCapMode CapMode     = ETransitCapMode::Flat;
	float RadiusBias            = 0.0f;
	TArray<FVector> Points;
	bool bCyclic                = false;

	// Back-reference to the originating component (not serialized)
	TWeakObjectPtr<UTransitSplineComponent> SourceComponent;
};

// ── Corridor edge (segment between two junction nodes) ──────────────────────

struct TRANSITARCHITECTRUNTIME_API FCorridorEdge
{
	FString EdgeId;
	FString SourceId;
	FString SplineKey;
	TArray<FVector> Points;
	FString StartNodeId;
	FString EndNodeId;
	FResolvedTransitProfile Profile;
	ETransitFamily Family       = ETransitFamily::Road;
	ETransitCapMode CapMode     = ETransitCapMode::Flat;
	float RadiusBias            = 0.0f;
};

// ── Junction node ───────────────────────────────────────────────────────────

struct TRANSITARCHITECTRUNTIME_API FJunctionNode
{
	FString NodeId;
	FVector Co = FVector::ZeroVector;
	TArray<FString> EdgeIds;
	TSet<FString> ConnectedSourceIds;
	int32 Degree                     = 0;
	float CornerRadius               = 200.0f;  // cm
	bool bJunctionEnabled            = true;
	bool bForceIgnore                = false;
	ETransitJunctionType NodeType    = ETransitJunctionType::Auto;
};

// ── Complete graph ──────────────────────────────────────────────────────────

struct TRANSITARCHITECTRUNTIME_API FTransitGraph
{
	TMap<FString, FJunctionNode> Nodes;
	TMap<FString, FCorridorEdge> Edges;
	TMap<FString, TArray<FString>> SourceToEdgeIds;
	TArray<FString> Warnings;

	void Reset()
	{
		Nodes.Reset();
		Edges.Reset();
		SourceToEdgeIds.Reset();
		Warnings.Reset();
	}
};
