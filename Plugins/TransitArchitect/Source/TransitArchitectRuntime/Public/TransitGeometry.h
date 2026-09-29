// TransitGeometry.h — Procedural mesh generation for transit corridors and junctions
#pragma once

#include "CoreMinimal.h"

struct FTransitMeshSpec;
struct FTransitGraph;
struct FCorridorEdge;
struct FJunctionNode;
struct FResolvedTransitProfile;

// All geometry is generated in world space (centimeters).

struct TRANSITARCHITECTRUNTIME_API FTransitGeometry
{
	// ── Polyline utilities ───────────────────────────────────────────────

	static float PolylineLength(const TArray<FVector>& Points);
	static TArray<float> CumulativeLengths(const TArray<FVector>& Points);
	static FVector PointOnPolyline(const TArray<FVector>& Points, float Distance);
	static TArray<FVector> TrimPolyline(const TArray<FVector>& Points, float StartTrim, float EndTrim);
	static TArray<FVector> ResamplePolyline(const TArray<FVector>& Points, int32 Segments);

	// ── Frame computation ───────────────────────────────────────────────

	// Compute tangent, left, up at a polyline index
	static void ComputeFrame(const TArray<FVector>& Points, int32 Index,
	                         FVector& OutTangent, FVector& OutLeft, FVector& OutUp);

	// ── Cross-section ───────────────────────────────────────────────────

	struct FCrossSection
	{
		float Distance;
		FVector Center;
		FVector RoadLeft, RoadRight;
		FVector CurbLeft, CurbRight;
		FVector PaveLeftOuter, PaveRightOuter;
		FVector PaveLeftOuterBase, PaveRightOuterBase;
	};

	static TArray<FCrossSection> BuildCrossSections(
		const TArray<FVector>& Polyline, const FResolvedTransitProfile& Profile);

	// ── Segment mesh (road between two junctions) ───────────────────────

	static void BuildSegmentMesh(const FTransitGraph& Graph, const FCorridorEdge& Edge,
	                             FTransitMeshSpec& OutRoad, FTransitMeshSpec& OutPavement);

	// ── Junction mesh ───────────────────────────────────────────────────

	static void BuildJunctionMesh(const FTransitGraph& Graph, const FJunctionNode& Node,
	                              FTransitMeshSpec& OutRoad, FTransitMeshSpec& OutPavement);

	// ── Surface patches ─────────────────────────────────────────────────

	// Coons patch: bilinear blending of 4 boundary curves → grid of points
	static TArray<TArray<FVector>> MakeCoonsPatch(
		const TArray<FVector>& Bottom, const TArray<FVector>& Top,
		const TArray<FVector>& Left, const TArray<FVector>& Right);

	// Add a Coons quad patch to the mesh spec
	static void AddQuadPatch(FTransitMeshSpec& Spec,
		const TArray<FVector>& Bottom, const TArray<FVector>& Top,
		const TArray<FVector>& Left, const TArray<FVector>& Right,
		int32 USegments = -1, int32 VSegments = -1);

	// Add a linear strip between two curves
	static void AddCurveStrip(FTransitMeshSpec& Spec,
		const TArray<FVector>& StartCurve, const TArray<FVector>& EndCurve,
		int32 Segments = -1, int32 Rows = 1);

	// ── Fillet arc ──────────────────────────────────────────────────────

	static TArray<FVector> FilletBetweenEdges(
		const FVector& PRightPoint, const FVector& TangentRight,
		const FVector& PLeftPoint, const FVector& TangentLeft,
		float Radius, float Z, int32 Steps = 8);

	// ── Offset curve ────────────────────────────────────────────────────

	static TArray<FVector> OffsetCurveFromTargets(
		const TArray<FVector>& BaseCurve,
		const FVector& StartTarget, const FVector& EndTarget);

	// ── Helpers ─────────────────────────────────────────────────────────

	static bool NodeGeneratesJunction(const FTransitGraph& Graph, const FJunctionNode& Node);

private:
	static FVector SafeNormalized(const FVector& V, const FVector& Fallback);
	static FVector2D PlanarUV(const FVector& Point, float Scale = 0.01f);

	struct FApproachFrame
	{
		FString EdgeId;
		FVector Point;
		FVector Tangent;
		FVector Left, Up;
		FVector RoadLeft, RoadRight;
		FVector CurbLeft, CurbRight;
		FVector PaveLeft, PaveRight;
		float Angle;
	};

	static FApproachFrame ComputeApproachFrame(
		const FTransitGraph& Graph, const FCorridorEdge& Edge,
		const FString& NodeId, float TrimDistance, bool bForceEnd = false);
};
