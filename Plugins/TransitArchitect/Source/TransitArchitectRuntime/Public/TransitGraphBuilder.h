// TransitGraphBuilder.h — Builds a transit graph from spline components
#pragma once

#include "CoreMinimal.h"

struct FTransitGraph;
struct FSampledCorridor;
struct FJunctionNode;
class UTransitSplineComponent;

struct TRANSITARCHITECTRUNTIME_API FTransitGraphBuilder
{
	// Merge distance for node endpoints in XY (cm). Default: 300 cm = 3m
	float NodeXYMergeDistance = 300.0f;

	// Polyline sample step (cm). Default: 200 cm = 2m
	float PolylineSampleStep = 200.0f;

	// Z merge threshold for intersections (cm). Default: 150 cm = 1.5m
	float ZMergeThreshold = 150.0f;

	// Spatial grid cell size for intersection detection (cm)
	float GridCellSize = 1000.0f;

	// Build the full graph from an array of spline components
	FTransitGraph BuildGraph(const TArray<UTransitSplineComponent*>& Splines);

	// Sub-steps exposed for testing
	TArray<FSampledCorridor> SampleSplines(const TArray<UTransitSplineComponent*>& Splines);

private:
	// ── Split map types ─────────────────────────────────────────────────
	struct FSplitMarker
	{
		float T;
		bool bIsNode;
	};

	// SplineKey -> SegmentIndex -> array of markers
	using FSplitMap = TMap<FString, TMap<int32, TArray<FSplitMarker>>>;

	struct FPointRecord
	{
		FVector Co;
		bool bIsNode = false;
	};

	// ── Internal methods ────────────────────────────────────────────────
	void AddSplitMarker(FSplitMap& SplitMap, const FString& SplineKey, int32 SegIndex, float T, bool bIsNode);

	FSplitMap BuildSplitRecords(const TArray<FSampledCorridor>& Samples);

	void EnsureAllSplinesSplitAtNodes(const TArray<FSampledCorridor>& Samples, FSplitMap& SplitMap);

	TArray<FPointRecord> RecordsFromSplits(const FSampledCorridor& Sample, const FSplitMap& SplitMap);

	FTransitGraph BuildEdges(const TArray<FSampledCorridor>& Samples, const FSplitMap& SplitMap);

	FString FindOrCreateNode(TMap<FString, FJunctionNode>& Nodes, const FVector& Co);

	FString StableNodeId(const FVector& Co);

	float DefaultCornerRadius(const FTransitGraph& Graph, const FJunctionNode& Node);

	// 2D segment intersection
	struct FIntersectionResult
	{
		enum EType { None, Point, Colinear };
		EType Type = None;
		float T = 0.0f;
		float U = 0.0f;
		FVector HitPoint = FVector::ZeroVector;
		float ZA = 0.0f;
		float ZB = 0.0f;
	};

	static FIntersectionResult SegmentIntersection2D(const FVector& A0, const FVector& A1,
	                                                  const FVector& B0, const FVector& B1);

	// Cached warnings
	TArray<FString> Warnings;
};
