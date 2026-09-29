// TransitGraphBuilder.cpp — Port of network.py core graph-building algorithms
// All distances in centimeters.
#include "TransitGraphBuilder.h"
#include "TransitGraph.h"
#include "TransitSplineComponent.h"
#include "TransitGeometry.h"
#include "TransitArchitectRuntime.h"

static constexpr float INTERSECTION_EPSILON = 0.0001f; // cm

// ═══════════════════════════════════════════════════════════════════════════
// Spline sampling
// ═══════════════════════════════════════════════════════════════════════════

TArray<FSampledCorridor> FTransitGraphBuilder::SampleSplines(const TArray<UTransitSplineComponent*>& Splines)
{
	TArray<FSampledCorridor> Result;

	for (int32 SplineIdx = 0; SplineIdx < Splines.Num(); ++SplineIdx)
	{
		UTransitSplineComponent* Spline = Splines[SplineIdx];
		if (!Spline || Spline->GetNumberOfSplinePoints() < 2) continue;

		float TotalLength = Spline->GetSplineLength();
		if (TotalLength < 1.0f) continue;

		int32 NumSamples = FMath::CeilToInt(TotalLength / FMath::Max(PolylineSampleStep, 1.0f));
		NumSamples = FMath::Max(NumSamples, 2);

		TArray<FVector> Points;
		Points.Reserve(NumSamples + 1);
		for (int32 I = 0; I <= NumSamples; ++I)
		{
			float Dist = FMath::Min((float)I * PolylineSampleStep, TotalLength);
			Points.Add(Spline->GetLocationAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World));
		}
		// Ensure last point is exact end
		if (Points.Num() > 0)
		{
			Points.Last() = Spline->GetLocationAtDistanceAlongSpline(TotalLength, ESplineCoordinateSpace::World);
		}

		// Dedup
		TArray<FVector> Deduped;
		for (const FVector& P : Points)
		{
			if (Deduped.Num() > 0 && FVector::Dist(Deduped.Last(), P) <= 0.01f) continue;
			Deduped.Add(P);
		}
		if (Deduped.Num() < 2) continue;

		bool bCyclic = Spline->IsClosedLoop();
		if (bCyclic && Deduped.Num() > 1 && FVector::Dist(Deduped[0], Deduped.Last()) > 0.01f)
		{
			Deduped.Add(Deduped[0]);
		}

		FSampledCorridor Corridor;
		Corridor.SourceId = FString::Printf(TEXT("S%d_%s"), SplineIdx, *Spline->GetName());
		Corridor.SplineKey = Corridor.SourceId;
		Corridor.Profile = Spline->ResolveProfile();
		Corridor.Family = Spline->Family;
		Corridor.CapMode = Spline->CapMode;
		Corridor.RadiusBias = Spline->JunctionRadiusBias;
		Corridor.Points = MoveTemp(Deduped);
		Corridor.bCyclic = bCyclic;
		Corridor.SourceComponent = Spline;
		Result.Add(MoveTemp(Corridor));
	}

	return Result;
}

// ═══════════════════════════════════════════════════════════════════════════
// 2D segment intersection
// ═══════════════════════════════════════════════════════════════════════════

FTransitGraphBuilder::FIntersectionResult FTransitGraphBuilder::SegmentIntersection2D(
	const FVector& A0, const FVector& A1, const FVector& B0, const FVector& B1)
{
	FIntersectionResult Out;

	FVector2D P(A0.X, A0.Y);
	FVector2D Q(B0.X, B0.Y);
	FVector2D R(A1.X - A0.X, A1.Y - A0.Y);
	FVector2D S(B1.X - B0.X, B1.Y - B0.Y);

	float Denom = R.X * S.Y - R.Y * S.X;
	FVector2D Delta = Q - P;

	if (FMath::Abs(Denom) <= INTERSECTION_EPSILON)
	{
		float CrossDR = Delta.X * R.Y - Delta.Y * R.X;
		if (FMath::Abs(CrossDR) <= INTERSECTION_EPSILON)
		{
			Out.Type = FIntersectionResult::Colinear;
		}
		return Out;
	}

	float T = (Delta.X * S.Y - Delta.Y * S.X) / Denom;
	float U = (Delta.X * R.Y - Delta.Y * R.X) / Denom;

	if (T >= -INTERSECTION_EPSILON && T <= 1.0f + INTERSECTION_EPSILON &&
	    U >= -INTERSECTION_EPSILON && U <= 1.0f + INTERSECTION_EPSILON)
	{
		T = FMath::Clamp(T, 0.0f, 1.0f);
		U = FMath::Clamp(U, 0.0f, 1.0f);
		Out.Type = FIntersectionResult::Point;
		Out.T = T;
		Out.U = U;
		Out.HitPoint = FMath::Lerp(A0, A1, T);
		Out.ZA = A0.Z + (A1.Z - A0.Z) * T;
		Out.ZB = B0.Z + (B1.Z - B0.Z) * U;
	}

	return Out;
}

// ═══════════════════════════════════════════════════════════════════════════
// Split markers
// ═══════════════════════════════════════════════════════════════════════════

void FTransitGraphBuilder::AddSplitMarker(FSplitMap& SplitMap, const FString& SplineKey, int32 SegIndex, float T, bool bIsNode)
{
	TArray<FSplitMarker>& Markers = SplitMap.FindOrAdd(SplineKey).FindOrAdd(SegIndex);
	for (FSplitMarker& M : Markers)
	{
		if (FMath::Abs(M.T - T) <= 0.000001f)
		{
			M.bIsNode = M.bIsNode || bIsNode;
			return;
		}
	}
	Markers.Add({T, bIsNode});
}

FTransitGraphBuilder::FSplitMap FTransitGraphBuilder::BuildSplitRecords(const TArray<FSampledCorridor>& Samples)
{
	FSplitMap SplitMap;

	// Initialize endpoint markers for each spline
	for (const FSampledCorridor& Sample : Samples)
	{
		int32 SegCount = Sample.Points.Num() - 1;
		for (int32 SI = 0; SI < SegCount; ++SI)
		{
			bool bStartEndpoint = (SI == 0 && !Sample.bCyclic);
			bool bEndEndpoint = (SI == SegCount - 1 && !Sample.bCyclic);
			AddSplitMarker(SplitMap, Sample.SplineKey, SI, 0.0f, bStartEndpoint);
			AddSplitMarker(SplitMap, Sample.SplineKey, SI, 1.0f, bEndEndpoint);
		}
	}

	// Build flat segment list for spatial grid
	struct FSegEntry
	{
		int32 SampleIdx;
		int32 SegIdx;
		FVector P0, P1;
	};
	TArray<FSegEntry> Segments;
	for (int32 SI = 0; SI < Samples.Num(); ++SI)
	{
		for (int32 SegI = 0; SegI < Samples[SI].Points.Num() - 1; ++SegI)
		{
			Segments.Add({SI, SegI, Samples[SI].Points[SegI], Samples[SI].Points[SegI + 1]});
		}
	}

	// Spatial grid
	auto CellKey = [this](float X, float Y) -> FIntPoint
	{
		return FIntPoint(FMath::FloorToInt(X / GridCellSize), FMath::FloorToInt(Y / GridCellSize));
	};

	TMap<FIntPoint, TArray<int32>> Grid;
	for (int32 I = 0; I < Segments.Num(); ++I)
	{
		const FSegEntry& S = Segments[I];
		float MinX = FMath::Min(S.P0.X, S.P1.X);
		float MaxX = FMath::Max(S.P0.X, S.P1.X);
		float MinY = FMath::Min(S.P0.Y, S.P1.Y);
		float MaxY = FMath::Max(S.P0.Y, S.P1.Y);
		FIntPoint C0 = CellKey(MinX, MinY);
		FIntPoint C1 = CellKey(MaxX, MaxY);
		for (int32 CX = C0.X; CX <= C1.X; ++CX)
		{
			for (int32 CY = C0.Y; CY <= C1.Y; ++CY)
			{
				Grid.FindOrAdd(FIntPoint(CX, CY)).Add(I);
			}
		}
	}

	// Test pairs in shared cells
	TSet<uint64> Tested;
	for (const auto& Cell : Grid)
	{
		const TArray<int32>& CellSegs = Cell.Value;
		for (int32 A = 0; A < CellSegs.Num(); ++A)
		{
			int32 LI = CellSegs[A];
			for (int32 B = A + 1; B < CellSegs.Num(); ++B)
			{
				int32 RI = CellSegs[B];
				uint64 PairKey = ((uint64)FMath::Min(LI, RI) << 32) | (uint64)FMath::Max(LI, RI);
				if (Tested.Contains(PairKey)) continue;
				Tested.Add(PairKey);

				const FSegEntry& Left = Segments[LI];
				const FSegEntry& Right = Segments[RI];

				// Skip same-spline adjacent segments
				if (Samples[Left.SampleIdx].SplineKey == Samples[Right.SampleIdx].SplineKey)
				{
					if (FMath::Abs(Left.SegIdx - Right.SegIdx) <= 3) continue;
				}

				FIntersectionResult Hit = SegmentIntersection2D(Left.P0, Left.P1, Right.P0, Right.P1);
				if (Hit.Type != FIntersectionResult::Point) continue;
				if (FMath::Abs(Hit.ZA - Hit.ZB) > ZMergeThreshold) continue;

				// For same-spline pairs close in index, require interior hit
				if (Samples[Left.SampleIdx].SplineKey == Samples[Right.SampleIdx].SplineKey)
				{
					int32 Gap = FMath::Abs(Left.SegIdx - Right.SegIdx);
					if (Gap <= 8)
					{
						if (Hit.T < 0.1f || Hit.T > 0.9f || Hit.U < 0.1f || Hit.U > 0.9f)
						{
							continue;
						}
					}
				}

				AddSplitMarker(SplitMap, Samples[Left.SampleIdx].SplineKey, Left.SegIdx, Hit.T, true);
				AddSplitMarker(SplitMap, Samples[Right.SampleIdx].SplineKey, Right.SegIdx, Hit.U, true);
			}
		}
	}

	EnsureAllSplinesSplitAtNodes(Samples, SplitMap);

	return SplitMap;
}

// ═══════════════════════════════════════════════════════════════════════════
// Ensure all splines split at detected nodes
// ═══════════════════════════════════════════════════════════════════════════

void FTransitGraphBuilder::EnsureAllSplinesSplitAtNodes(const TArray<FSampledCorridor>& Samples, FSplitMap& SplitMap)
{
	// Gather all unique node locations
	TArray<FVector> NodeLocations;
	TSet<FIntVector> Seen;

	for (const FSampledCorridor& Sample : Samples)
	{
		int32 SegCount = Sample.Points.Num() - 1;
		const TMap<int32, TArray<FSplitMarker>>* SplineMap = SplitMap.Find(Sample.SplineKey);
		if (!SplineMap) continue;

		for (int32 SegI = 0; SegI < SegCount; ++SegI)
		{
			const TArray<FSplitMarker>* Markers = SplineMap->Find(SegI);
			if (!Markers) continue;
			for (const FSplitMarker& M : *Markers)
			{
				if (!M.bIsNode) continue;
				FVector Pt = FMath::Lerp(Sample.Points[SegI], Sample.Points[SegI + 1], M.T);
				FIntVector QK(FMath::RoundToInt(Pt.X * 0.02f), FMath::RoundToInt(Pt.Y * 0.02f), FMath::RoundToInt(Pt.Z * 0.02f));
				if (Seen.Contains(QK)) continue;
				Seen.Add(QK);
				NodeLocations.Add(Pt);
			}
		}
	}

	if (NodeLocations.Num() == 0) return;

	// For each spline, check every node location
	for (const FSampledCorridor& Sample : Samples)
	{
		int32 SegCount = Sample.Points.Num() - 1;

		for (const FVector& NodeCo : NodeLocations)
		{
			// Check if already has a marker near this node
			bool bAlready = false;
			const TMap<int32, TArray<FSplitMarker>>* SplineMap = SplitMap.Find(Sample.SplineKey);
			if (SplineMap)
			{
				for (int32 SegI = 0; SegI < SegCount && !bAlready; ++SegI)
				{
					const TArray<FSplitMarker>* Markers = SplineMap->Find(SegI);
					if (!Markers) continue;
					for (const FSplitMarker& M : *Markers)
					{
						if (!M.bIsNode) continue;
						FVector Pt = FMath::Lerp(Sample.Points[SegI], Sample.Points[SegI + 1], M.T);
						FVector2D DXY(Pt.X - NodeCo.X, Pt.Y - NodeCo.Y);
						if (DXY.Size() <= NodeXYMergeDistance && FMath::Abs(Pt.Z - NodeCo.Z) <= ZMergeThreshold)
						{
							bAlready = true;
							break;
						}
					}
				}
			}
			if (bAlready) continue;

			// Project node onto polyline segments
			int32 BestSeg = -1;
			float BestT = 0.0f;
			float BestDist = MAX_FLT;

			for (int32 SegI = 0; SegI < SegCount; ++SegI)
			{
				const FVector& P0 = Sample.Points[SegI];
				const FVector& P1 = Sample.Points[SegI + 1];
				FVector D = P1 - P0;
				float LenSq = D.SizeSquared();
				if (LenSq < 0.0001f) continue;
				float T = FMath::Clamp(FVector::DotProduct(NodeCo - P0, D) / LenSq, 0.0f, 1.0f);
				FVector Proj = FMath::Lerp(P0, P1, T);
				float DistXY = FVector2D(Proj.X - NodeCo.X, Proj.Y - NodeCo.Y).Size();
				if (DistXY < BestDist)
				{
					BestDist = DistXY;
					BestSeg = SegI;
					BestT = T;
				}
			}

			if (BestSeg >= 0 && BestDist < NodeXYMergeDistance)
			{
				FVector P0 = Sample.Points[BestSeg];
				FVector P1 = Sample.Points[BestSeg + 1];
				float ProjZ = P0.Z + (P1.Z - P0.Z) * BestT;
				if (FMath::Abs(ProjZ - NodeCo.Z) <= ZMergeThreshold)
				{
					AddSplitMarker(SplitMap, Sample.SplineKey, BestSeg, BestT, true);
				}
			}
		}
	}
}

// ═══════════════════════════════════════════════════════════════════════════
// Records from splits
// ═══════════════════════════════════════════════════════════════════════════

TArray<FTransitGraphBuilder::FPointRecord> FTransitGraphBuilder::RecordsFromSplits(
	const FSampledCorridor& Sample, const FSplitMap& SplitMap)
{
	TArray<FPointRecord> Records;
	int32 SegCount = Sample.Points.Num() - 1;

	const TMap<int32, TArray<FSplitMarker>>* SplineMap = SplitMap.Find(Sample.SplineKey);
	if (!SplineMap) return Records;

	for (int32 SegI = 0; SegI < SegCount; ++SegI)
	{
		const TArray<FSplitMarker>* Markers = SplineMap->Find(SegI);
		if (!Markers) continue;

		// Sort by T
		TArray<FSplitMarker> Sorted = *Markers;
		Sorted.Sort([](const FSplitMarker& A, const FSplitMarker& B) { return A.T < B.T; });

		const FVector& PA = Sample.Points[SegI];
		const FVector& PB = Sample.Points[SegI + 1];

		for (const FSplitMarker& M : Sorted)
		{
			FVector Pt = FMath::Lerp(PA, PB, M.T);
			if (Records.Num() > 0 && FVector::Dist(Records.Last().Co, Pt) <= 0.001f)
			{
				Records.Last().bIsNode = Records.Last().bIsNode || M.bIsNode;
				continue;
			}
			Records.Add({Pt, M.bIsNode});
		}
	}

	// Ensure endpoints are nodes for non-cyclic splines
	if (!Sample.bCyclic && Records.Num() > 0)
	{
		Records[0].bIsNode = true;
		Records.Last().bIsNode = true;
	}
	if (Sample.bCyclic && Records.Num() > 0)
	{
		bool bAnyNode = false;
		for (const FPointRecord& R : Records) if (R.bIsNode) { bAnyNode = true; break; }
		if (!bAnyNode)
		{
			Records[0].bIsNode = true;
			if (FVector::Dist(Records[0].Co, Records.Last().Co) <= 0.01f)
			{
				Records.Last().bIsNode = true;
			}
			else
			{
				Records.Add({Records[0].Co, true});
			}
		}
		else if (FVector::Dist(Records[0].Co, Records.Last().Co) <= 0.01f)
		{
			bool Flag = Records[0].bIsNode || Records.Last().bIsNode;
			Records[0].bIsNode = Flag;
			Records.Last().bIsNode = Flag;
		}
	}

	return Records;
}

// ═══════════════════════════════════════════════════════════════════════════
// Node helpers
// ═══════════════════════════════════════════════════════════════════════════

FString FTransitGraphBuilder::StableNodeId(const FVector& Co)
{
	// Hash the quantized position for stable IDs
	FString Payload = FString::Printf(TEXT("%.1f|%.1f|%.1f"), Co.X, Co.Y, Co.Z);
	uint32 Hash = GetTypeHash(Payload);
	return FString::Printf(TEXT("N_%08X"), Hash);
}

FString FTransitGraphBuilder::FindOrCreateNode(TMap<FString, FJunctionNode>& Nodes, const FVector& Co)
{
	for (auto& Pair : Nodes)
	{
		FJunctionNode& Node = Pair.Value;
		if (FMath::Abs(Node.Co.Z - Co.Z) > ZMergeThreshold) continue;
		FVector2D DXY(Node.Co.X - Co.X, Node.Co.Y - Co.Y);
		if (DXY.Size() <= NodeXYMergeDistance)
		{
			return Node.NodeId;
		}
	}

	FString NodeId = StableNodeId(Co);
	while (Nodes.Contains(NodeId))
	{
		NodeId += TEXT("_X");
	}
	FJunctionNode NewNode;
	NewNode.NodeId = NodeId;
	NewNode.Co = Co;
	Nodes.Add(NodeId, MoveTemp(NewNode));
	return NodeId;
}

// ═══════════════════════════════════════════════════════════════════════════
// Build edges
// ═══════════════════════════════════════════════════════════════════════════

FTransitGraph FTransitGraphBuilder::BuildEdges(const TArray<FSampledCorridor>& Samples, const FSplitMap& SplitMap)
{
	FTransitGraph Graph;
	Graph.Warnings = MoveTemp(Warnings);

	for (const FSampledCorridor& Sample : Samples)
	{
		TArray<FPointRecord> Records = RecordsFromSplits(Sample, SplitMap);
		if (Records.Num() < 2) continue;

		int32 EdgeCounter = 0;
		int32 CurrentIndex = 0;

		while (CurrentIndex < Records.Num())
		{
			if (!Records[CurrentIndex].bIsNode)
			{
				++CurrentIndex;
				continue;
			}

			FString StartNodeId = FindOrCreateNode(Graph.Nodes, Records[CurrentIndex].Co);
			TArray<FVector> Polyline = {Records[CurrentIndex].Co};
			int32 SearchIndex = CurrentIndex + 1;

			while (SearchIndex < Records.Num())
			{
				Polyline.Add(Records[SearchIndex].Co);

				if (Records[SearchIndex].bIsNode)
				{
					FString EndNodeId = FindOrCreateNode(Graph.Nodes, Records[SearchIndex].Co);

					// Dedup polyline
					TArray<FVector> Deduped;
					for (const FVector& P : Polyline)
					{
						if (Deduped.Num() > 0 && FVector::Dist(Deduped.Last(), P) <= 0.001f) continue;
						Deduped.Add(P);
					}

					if (Deduped.Num() >= 2 && FTransitGeometry::PolylineLength(Deduped) > 5.0f)
					{
						FString EdgeId = FString::Printf(TEXT("%s:E%03d"), *Sample.SplineKey, EdgeCounter);

						FCorridorEdge Edge;
						Edge.EdgeId = EdgeId;
						Edge.SourceId = Sample.SourceId;
						Edge.SplineKey = Sample.SplineKey;
						Edge.Points = MoveTemp(Deduped);
						Edge.StartNodeId = StartNodeId;
						Edge.EndNodeId = EndNodeId;
						Edge.Profile = Sample.Profile;
						Edge.Family = Sample.Family;
						Edge.CapMode = Sample.CapMode;
						Edge.RadiusBias = Sample.RadiusBias;

						Graph.Edges.Add(EdgeId, MoveTemp(Edge));
						Graph.SourceToEdgeIds.FindOrAdd(Sample.SourceId).Add(EdgeId);
						Graph.Nodes[StartNodeId].EdgeIds.Add(EdgeId);
						Graph.Nodes[StartNodeId].ConnectedSourceIds.Add(Sample.SourceId);
						Graph.Nodes[EndNodeId].EdgeIds.Add(EdgeId);
						Graph.Nodes[EndNodeId].ConnectedSourceIds.Add(Sample.SourceId);
						++EdgeCounter;
					}

					CurrentIndex = SearchIndex;
					break;
				}
				++SearchIndex;
			}

			if (SearchIndex >= Records.Num()) break;
		}
	}

	// Compute node degrees
	for (auto& Pair : Graph.Nodes)
	{
		Pair.Value.Degree = Pair.Value.EdgeIds.Num();
	}

	return Graph;
}

// ═══════════════════════════════════════════════════════════════════════════
// Corner radius computation (port of default_corner_radius)
// ═══════════════════════════════════════════════════════════════════════════

float FTransitGraphBuilder::DefaultCornerRadius(const FTransitGraph& Graph, const FJunctionNode& Node)
{
	if (Node.Degree < 2) return 200.0f; // 2m

	struct FApproachInfo
	{
		FVector Tangent;
		float Angle;
		float HalfWidth;
		float Bias;
	};

	TArray<FApproachInfo> Approaches;
	TSet<FString> SeenEdgeIds;

	for (const FString& EdgeId : Node.EdgeIds)
	{
		const FCorridorEdge* Edge = Graph.Edges.Find(EdgeId);
		if (!Edge) continue;

		bool bIsLoop = Edge->StartNodeId == Edge->EndNodeId;
		bool bForceEnd = bIsLoop && SeenEdgeIds.Contains(EdgeId);
		SeenEdgeIds.Add(EdgeId);

		FVector Tangent;
		if (Node.NodeId == Edge->StartNodeId && !bForceEnd)
			Tangent = (Edge->Points[1] - Edge->Points[0]).GetSafeNormal();
		else
			Tangent = (Edge->Points[Edge->Points.Num() - 2] - Edge->Points.Last()).GetSafeNormal();

		if (Tangent.IsNearlyZero()) Tangent = FVector(1, 0, 0);

		float HW = Edge->Profile.RoadHalfWidth() + Edge->Profile.CurbWidth +
		           FMath::Max(Edge->Profile.PavementLeft, Edge->Profile.PavementRight);

		FApproachInfo Info;
		Info.Tangent = Tangent;
		Info.Angle = FMath::Atan2(Tangent.Y, Tangent.X);
		Info.HalfWidth = HW;
		Info.Bias = Edge->RadiusBias;
		Approaches.Add(Info);
	}

	if (Approaches.Num() == 0) return 200.0f;

	Approaches.Sort([](const FApproachInfo& A, const FApproachInfo& B) { return A.Angle < B.Angle; });

	float MaxRadius = 200.0f;
	int32 Num = Approaches.Num();
	for (int32 I = 0; I < Num; ++I)
	{
		const FApproachInfo& Cur = Approaches[I];
		const FApproachInfo& Nxt = Approaches[(I + 1) % Num];

		float Delta = Nxt.Angle - Cur.Angle;
		while (Delta < 0) Delta += 2.0f * PI;
		while (Delta >= 2.0f * PI) Delta -= 2.0f * PI;

		if (Delta < 0.1f) continue;

		float WMax = FMath::Max(Cur.HalfWidth, Nxt.HalfWidth);
		float Fillet = FMath::Max(Cur.Bias, Nxt.Bias) + 150.0f; // 1.5m
		float Required = (WMax / FMath::Max(0.1f, FMath::Sin(Delta * 0.5f))) + Fillet;
		MaxRadius = FMath::Max(MaxRadius, Required);
	}

	return FMath::Min(MaxRadius, 2500.0f); // 25m cap
}

// ═══════════════════════════════════════════════════════════════════════════
// Main entry: BuildGraph
// ═══════════════════════════════════════════════════════════════════════════

FTransitGraph FTransitGraphBuilder::BuildGraph(const TArray<UTransitSplineComponent*>& Splines)
{
	Warnings.Reset();

	TArray<FSampledCorridor> Samples = SampleSplines(Splines);
	if (Samples.Num() == 0) return FTransitGraph();

	FSplitMap SplitMap = BuildSplitRecords(Samples);
	FTransitGraph Graph = BuildEdges(Samples, SplitMap);

	// Compute corner radii and sync junction settings
	for (auto& Pair : Graph.Nodes)
	{
		FJunctionNode& Node = Pair.Value;
		Node.CornerRadius = DefaultCornerRadius(Graph, Node);
	}

	UE_LOG(LogTransitArchitect, Log, TEXT("TransitGraph: %d edges, %d nodes"),
		Graph.Edges.Num(), Graph.Nodes.Num());

	return Graph;
}
