// TransitGeometry.cpp — Port of geometry.py to C++
// All distances in centimeters. Blender meters × 100.
#include "TransitGeometry.h"
#include "TransitMeshSpec.h"
#include "TransitGraph.h"
#include "TransitTypes.h"
#include "TransitArchitectRuntime.h"

// ═══════════════════════════════════════════════════════════════════════════
// Polyline utilities
// ═══════════════════════════════════════════════════════════════════════════

float FTransitGeometry::PolylineLength(const TArray<FVector>& Points)
{
	float Total = 0.0f;
	for (int32 i = 0; i < Points.Num() - 1; ++i)
	{
		Total += FVector::Dist(Points[i], Points[i + 1]);
	}
	return Total;
}

TArray<float> FTransitGeometry::CumulativeLengths(const TArray<FVector>& Points)
{
	TArray<float> Cumulative;
	Cumulative.Reserve(Points.Num());
	Cumulative.Add(0.0f);
	for (int32 i = 0; i < Points.Num() - 1; ++i)
	{
		Cumulative.Add(Cumulative.Last() + FVector::Dist(Points[i], Points[i + 1]));
	}
	return Cumulative;
}

FVector FTransitGeometry::PointOnPolyline(const TArray<FVector>& Points, float Distance)
{
	if (Points.Num() == 0) return FVector::ZeroVector;
	if (Points.Num() == 1) return Points[0];

	TArray<float> Cum = CumulativeLengths(Points);
	float Total = Cum.Last();
	Distance = FMath::Clamp(Distance, 0.0f, Total);

	for (int32 i = 0; i < Points.Num() - 1; ++i)
	{
		float StartDist = Cum[i];
		float EndDist = Cum[i + 1];
		if (EndDist - StartDist <= 0.001f) continue;
		if (Distance <= EndDist || i == Points.Num() - 2)
		{
			float Factor = (Distance - StartDist) / (EndDist - StartDist);
			return FMath::Lerp(Points[i], Points[i + 1], Factor);
		}
	}
	return Points.Last();
}

TArray<FVector> FTransitGeometry::TrimPolyline(const TArray<FVector>& Points, float StartTrim, float EndTrim)
{
	float TotalLength = PolylineLength(Points);
	if (TotalLength <= 0.01f) return TArray<FVector>();

	StartTrim = FMath::Clamp(StartTrim, 0.0f, TotalLength * 0.49f);
	EndTrim = FMath::Clamp(EndTrim, 0.0f, TotalLength * 0.49f);
	if (TotalLength <= StartTrim + EndTrim + 0.01f) return TArray<FVector>();

	TArray<float> Cum = CumulativeLengths(Points);
	float StartDist = StartTrim;
	float EndDist = TotalLength - EndTrim;

	TArray<FVector> Trimmed;
	Trimmed.Add(PointOnPolyline(Points, StartDist));
	for (int32 i = 1; i < Points.Num() - 1; ++i)
	{
		if (Cum[i] > StartDist && Cum[i] < EndDist)
		{
			Trimmed.Add(Points[i]);
		}
	}
	Trimmed.Add(PointOnPolyline(Points, EndDist));

	// Dedup
	TArray<FVector> Deduped;
	for (const FVector& P : Trimmed)
	{
		if (Deduped.Num() > 0 && FVector::Dist(Deduped.Last(), P) <= 0.01f) continue;
		Deduped.Add(P);
	}
	if (Deduped.Num() < 2) return TArray<FVector>();
	return Deduped;
}

TArray<FVector> FTransitGeometry::ResamplePolyline(const TArray<FVector>& Points, int32 Segments)
{
	if (Points.Num() == 0) return TArray<FVector>();
	Segments = FMath::Max(1, Segments);
	if (Points.Num() == 1)
	{
		TArray<FVector> Result;
		for (int32 i = 0; i <= Segments; ++i) Result.Add(Points[0]);
		return Result;
	}

	float Total = PolylineLength(Points);
	if (Total <= 0.01f)
	{
		TArray<FVector> Result;
		for (int32 i = 0; i <= Segments; ++i) Result.Add(Points[0]);
		return Result;
	}

	TArray<FVector> Result;
	Result.Reserve(Segments + 1);
	for (int32 i = 0; i <= Segments; ++i)
	{
		Result.Add(PointOnPolyline(Points, (Total * i) / Segments));
	}
	return Result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Frame computation
// ═══════════════════════════════════════════════════════════════════════════

FVector FTransitGeometry::SafeNormalized(const FVector& V, const FVector& Fallback)
{
	if (V.Size() <= 0.001f) return Fallback;
	return V.GetSafeNormal();
}

FVector2D FTransitGeometry::PlanarUV(const FVector& Point, float Scale)
{
	return FVector2D(Point.X * Scale, Point.Y * Scale);
}

void FTransitGeometry::ComputeFrame(const TArray<FVector>& Points, int32 Index,
                                     FVector& OutTangent, FVector& OutLeft, FVector& OutUp)
{
	if (Index <= 0)
	{
		OutTangent = SafeNormalized(Points[1] - Points[0], FVector(1, 0, 0));
	}
	else if (Index >= Points.Num() - 1)
	{
		OutTangent = SafeNormalized(Points.Last() - Points[Points.Num() - 2], FVector(1, 0, 0));
	}
	else
	{
		OutTangent = SafeNormalized(Points[Index + 1] - Points[Index - 1], FVector(1, 0, 0));
	}

	FVector UpRef(0, 0, 1);
	if (FMath::Abs(FVector::DotProduct(OutTangent, UpRef)) > 0.98f)
	{
		UpRef = FVector(0, 1, 0);
	}
	OutLeft = SafeNormalized(FVector::CrossProduct(UpRef, OutTangent), FVector(0, 1, 0));
	OutUp = SafeNormalized(FVector::CrossProduct(OutTangent, OutLeft), FVector(0, 0, 1));
}

// ═══════════════════════════════════════════════════════════════════════════
// Cross-section
// ═══════════════════════════════════════════════════════════════════════════

TArray<FTransitGeometry::FCrossSection> FTransitGeometry::BuildCrossSections(
	const TArray<FVector>& Polyline, const FResolvedTransitProfile& Profile)
{
	TArray<float> Cum = CumulativeLengths(Polyline);
	TArray<FCrossSection> Sections;
	Sections.Reserve(Polyline.Num());

	for (int32 i = 0; i < Polyline.Num(); ++i)
	{
		FVector Tangent, Left, Up;
		ComputeFrame(Polyline, i, Tangent, Left, Up);

		const FVector& Pt = Polyline[i];
		float RHW = Profile.RoadHalfWidth();
		float CW = Profile.CurbWidth;
		float CH = Profile.CurbHeight;
		float PL = Profile.PavementLeft;
		float PR = Profile.PavementRight;

		FCrossSection S;
		S.Distance = Cum[i];
		S.Center = Pt;
		S.RoadLeft  = Pt + Left * RHW;
		S.RoadRight = Pt - Left * RHW;
		S.CurbLeft  = Pt + Left * (RHW + CW) + Up * CH;
		S.CurbRight = Pt - Left * (RHW + CW) + Up * CH;
		S.PaveLeftOuter      = Pt + Left * (RHW + CW + PL) + Up * CH;
		S.PaveRightOuter     = Pt - Left * (RHW + CW + PR) + Up * CH;
		S.PaveLeftOuterBase  = Pt + Left * (RHW + CW + PL);
		S.PaveRightOuterBase = Pt - Left * (RHW + CW + PR);
		Sections.Add(S);
	}
	return Sections;
}

// ═══════════════════════════════════════════════════════════════════════════
// Helper: NodeGeneratesJunction (port of node_generates_junction)
// ═══════════════════════════════════════════════════════════════════════════

static FVector EdgeTangentAwayFromNode(const FCorridorEdge& Edge, const FString& NodeId, bool bForceEnd = false)
{
	FVector Tangent;
	if (NodeId == Edge.StartNodeId && !bForceEnd)
	{
		Tangent = Edge.Points[1] - Edge.Points[0];
	}
	else
	{
		Tangent = Edge.Points[Edge.Points.Num() - 2] - Edge.Points.Last();
	}
	if (Tangent.Size() <= 0.001f) return FVector(1, 0, 0);
	return Tangent.GetSafeNormal();
}

bool FTransitGeometry::NodeGeneratesJunction(const FTransitGraph& Graph, const FJunctionNode& Node)
{
	if (Node.bForceIgnore || !Node.bJunctionEnabled) return false;
	if (Node.Degree < 2) return false;
	if (Node.NodeType != ETransitJunctionType::Auto) return true;
	if (Node.Degree >= 3) return true;

	// Degree == 2: check angle between edges
	const FCorridorEdge* EdgeA = Graph.Edges.Find(Node.EdgeIds[0]);
	const FCorridorEdge* EdgeB = Graph.Edges.Find(Node.EdgeIds[1]);
	if (!EdgeA || !EdgeB) return false;

	FVector TangentA = EdgeTangentAwayFromNode(*EdgeA, Node.NodeId);
	bool bForceEndB = (EdgeB->StartNodeId == EdgeB->EndNodeId && Node.EdgeIds[0] == Node.EdgeIds[1]);
	FVector TangentB = EdgeTangentAwayFromNode(*EdgeB, Node.NodeId, bForceEndB);

	float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(
		FVector::DotProduct(TangentA, TangentB), -1.0f, 1.0f)));
	return AngleDeg < 165.0f;
}

// ═══════════════════════════════════════════════════════════════════════════
// Segment mesh
// ═══════════════════════════════════════════════════════════════════════════

void FTransitGeometry::BuildSegmentMesh(const FTransitGraph& Graph, const FCorridorEdge& Edge,
                                         FTransitMeshSpec& OutRoad, FTransitMeshSpec& OutPavement)
{
	OutRoad.Reset();
	OutPavement.Reset();

	float TotalLength = PolylineLength(Edge.Points);
	if (TotalLength <= 0.01f) return;

	const FJunctionNode* StartNode = Graph.Nodes.Find(Edge.StartNodeId);
	const FJunctionNode* EndNode = Graph.Nodes.Find(Edge.EndNodeId);

	float StartTrim = (StartNode && NodeGeneratesJunction(Graph, *StartNode)) ? StartNode->CornerRadius : 0.0f;
	float EndTrim = (EndNode && NodeGeneratesJunction(Graph, *EndNode)) ? EndNode->CornerRadius : 0.0f;

	TArray<FVector> Trimmed = TrimPolyline(Edge.Points, StartTrim, EndTrim);
	if (Trimmed.Num() < 2) return;

	TArray<FCrossSection> Sections = BuildCrossSections(Trimmed, Edge.Profile);
	if (Sections.Num() < 2) return;

	// Pre-compute continuous V coordinates in meters across the full cross-section.
	// Layout (right to left): PaveRightSide | PaveRight | CurbRight | RoadRight | RoadLeft | CurbLeft | PaveLeft | PaveLeftSide
	// Each piece's V starts where the previous one ended — no seams with tileable textures.
	const float RHW_m = Edge.Profile.RoadHalfWidth() * 0.01f;
	const float CW_m  = Edge.Profile.CurbWidth * 0.01f;
	const float CH_m  = Edge.Profile.CurbHeight * 0.01f;
	const float PL_m  = Edge.Profile.PavementLeft * 0.01f;
	const float PR_m  = Edge.Profile.PavementRight * 0.01f;
	const float CurbDiag_m = FMath::Sqrt(CW_m * CW_m + CH_m * CH_m);

	// Road surface: continuous V from 0 (RoadRight) → RHW_m (Center) → 2*RHW_m (RoadLeft)
	const float VRoadRight  = 0.0f;
	const float VRoadCenter = RHW_m;
	const float VRoadLeft   = RHW_m * 2.0f;

	// Curb profile: continues from road edges outward
	const float VCurbRightOuter = VRoadRight - CurbDiag_m;  // negative = further right
	const float VCurbLeftOuter  = VRoadLeft + CurbDiag_m;

	// Pavement: continues from curb outer edges
	const float VPaveRightOuter = VCurbRightOuter - PR_m;
	const float VPaveLeftOuter  = VCurbLeftOuter + PL_m;

	// Pavement side drops: separate V strip for vertical face
	// (these are vertical faces so they get their own 0→CH_m range)

	// Sweep quads along the polyline
	for (int32 i = 0; i < Sections.Num() - 1; ++i)
	{
		const FCrossSection& Cur = Sections[i];
		const FCrossSection& Nxt = Sections[i + 1];
		float U0 = Cur.Distance * 0.01f; // UV U-axis in meters along spline
		float U1 = Nxt.Distance * 0.01f;

		// Road surface: 2 quads — V continuous across full road width
		OutRoad.AddFace(
			{Cur.Center, Nxt.Center, Nxt.RoadRight, Cur.RoadRight},
			{FVector2D(U0, VRoadCenter), FVector2D(U1, VRoadCenter), FVector2D(U1, VRoadRight), FVector2D(U0, VRoadRight)});
		OutRoad.AddFace(
			{Cur.RoadLeft, Nxt.RoadLeft, Nxt.Center, Cur.Center},
			{FVector2D(U0, VRoadLeft), FVector2D(U1, VRoadLeft), FVector2D(U1, VRoadCenter), FVector2D(U0, VRoadCenter)});

		// Curb (pavement profile): 2 quads — V continues from road edges
		OutRoad.AddFace(
			{Cur.CurbLeft, Nxt.CurbLeft, Nxt.RoadLeft, Cur.RoadLeft},
			{FVector2D(U0, VCurbLeftOuter), FVector2D(U1, VCurbLeftOuter), FVector2D(U1, VRoadLeft), FVector2D(U0, VRoadLeft)});
		OutRoad.AddFace(
			{Cur.RoadRight, Nxt.RoadRight, Nxt.CurbRight, Cur.CurbRight},
			{FVector2D(U0, VRoadRight), FVector2D(U1, VRoadRight), FVector2D(U1, VCurbRightOuter), FVector2D(U0, VCurbRightOuter)});

		// Pavement top: 2 quads — V continues from curb edges
		OutPavement.AddFace(
			{Cur.PaveLeftOuter, Nxt.PaveLeftOuter, Nxt.CurbLeft, Cur.CurbLeft},
			{FVector2D(U0, VPaveLeftOuter), FVector2D(U1, VPaveLeftOuter), FVector2D(U1, VCurbLeftOuter), FVector2D(U0, VCurbLeftOuter)});
		OutPavement.AddFace(
			{Cur.CurbRight, Nxt.CurbRight, Nxt.PaveRightOuter, Cur.PaveRightOuter},
			{FVector2D(U0, VCurbRightOuter), FVector2D(U1, VCurbRightOuter), FVector2D(U1, VPaveRightOuter), FVector2D(U0, VPaveRightOuter)});

		// Pavement side drops (vertical faces — own V range 0→CH_m)
		OutPavement.AddFace(
			{Cur.PaveLeftOuter, Nxt.PaveLeftOuter, Nxt.PaveLeftOuterBase, Cur.PaveLeftOuterBase},
			{FVector2D(U0, CH_m), FVector2D(U1, CH_m), FVector2D(U1, 0), FVector2D(U0, 0)});
		OutPavement.AddFace(
			{Cur.PaveRightOuterBase, Nxt.PaveRightOuterBase, Nxt.PaveRightOuter, Cur.PaveRightOuter},
			{FVector2D(U0, 0), FVector2D(U1, 0), FVector2D(U1, CH_m), FVector2D(U0, CH_m)});
	}

	// End caps
	bool bLoopEdge = Edge.StartNodeId == Edge.EndNodeId;
	bool bCapStart = StartNode && StartNode->Degree <= 1 &&
	                 !NodeGeneratesJunction(Graph, *StartNode) &&
	                 Edge.CapMode != ETransitCapMode::None && !bLoopEdge;
	bool bCapEnd = EndNode && EndNode->Degree <= 1 &&
	               !NodeGeneratesJunction(Graph, *EndNode) &&
	               Edge.CapMode != ETransitCapMode::None && !bLoopEdge;

	if (bCapStart)
	{
		const FCrossSection& S = Sections[0];
		OutRoad.AddTriangulatedPolygon({S.CurbLeft, S.RoadLeft, S.Center, S.RoadRight, S.CurbRight});
		OutPavement.AddTriangulatedPolygon({
			S.PaveLeftOuterBase, S.PaveLeftOuter, S.CurbLeft,
			S.CurbRight, S.PaveRightOuter, S.PaveRightOuterBase});
	}
	if (bCapEnd)
	{
		const FCrossSection& S = Sections.Last();
		OutRoad.AddTriangulatedPolygon({S.CurbLeft, S.RoadLeft, S.Center, S.RoadRight, S.CurbRight});
		OutPavement.AddTriangulatedPolygon({
			S.PaveLeftOuterBase, S.PaveLeftOuter, S.CurbLeft,
			S.CurbRight, S.PaveRightOuter, S.PaveRightOuterBase});
	}

	OutRoad.RecalcNormals();
	OutPavement.RecalcNormals();
}

// ═══════════════════════════════════════════════════════════════════════════
// Coons patch
// ═══════════════════════════════════════════════════════════════════════════

TArray<TArray<FVector>> FTransitGeometry::MakeCoonsPatch(
	const TArray<FVector>& Bottom, const TArray<FVector>& Top,
	const TArray<FVector>& Left, const TArray<FVector>& Right)
{
	TArray<TArray<FVector>> Grid;
	if (Bottom.Num() < 2 || Top.Num() < 2 || Left.Num() < 2 || Right.Num() < 2) return Grid;
	if (Bottom.Num() != Top.Num() || Left.Num() != Right.Num()) return Grid;

	int32 USteps = Bottom.Num() - 1;
	int32 VSteps = Left.Num() - 1;
	FVector P00 = Bottom[0], P10 = Bottom.Last();
	FVector P01 = Top[0], P11 = Top.Last();

	Grid.Reserve(USteps + 1);
	for (int32 UI = 0; UI <= USteps; ++UI)
	{
		float U = (float)UI / FMath::Max(USteps, 1);
		TArray<FVector> Row;
		Row.Reserve(VSteps + 1);
		for (int32 VJ = 0; VJ <= VSteps; ++VJ)
		{
			float V = (float)VJ / FMath::Max(VSteps, 1);
			FVector Bilinear = (1.0f - U) * (1.0f - V) * P00
			                 + U * (1.0f - V) * P10
			                 + (1.0f - U) * V * P01
			                 + U * V * P11;
			FVector Point = (1.0f - V) * Bottom[UI]
			              + V * Top[UI]
			              + (1.0f - U) * Left[VJ]
			              + U * Right[VJ]
			              - Bilinear;
			Row.Add(Point);
		}
		Grid.Add(MoveTemp(Row));
	}
	return Grid;
}

void FTransitGeometry::AddQuadPatch(FTransitMeshSpec& Spec,
	const TArray<FVector>& Bottom, const TArray<FVector>& Top,
	const TArray<FVector>& Left, const TArray<FVector>& Right,
	int32 USegments, int32 VSegments)
{
	if (Bottom.Num() < 2 || Top.Num() < 2 || Left.Num() < 2 || Right.Num() < 2) return;

	int32 USteps = FMath::Max(1, USegments >= 0 ? USegments : FMath::Max(Bottom.Num(), Top.Num()) - 1);
	int32 VSteps = FMath::Max(1, VSegments >= 0 ? VSegments : FMath::Max(Left.Num(), Right.Num()) - 1);

	TArray<FVector> ResBottom = ResamplePolyline(Bottom, USteps);
	TArray<FVector> ResTop = ResamplePolyline(Top, USteps);
	TArray<FVector> ResLeft = ResamplePolyline(Left, VSteps);
	TArray<FVector> ResRight = ResamplePolyline(Right, VSteps);

	TArray<TArray<FVector>> Grid = MakeCoonsPatch(ResBottom, ResTop, ResLeft, ResRight);
	if (Grid.Num() == 0) return;

	for (int32 UI = 0; UI < Grid.Num() - 1; ++UI)
	{
		for (int32 VJ = 0; VJ < Grid[0].Num() - 1; ++VJ)
		{
			FVector Pt1 = Grid[UI][VJ];
			FVector Pt2 = Grid[UI + 1][VJ];
			FVector Pt3 = Grid[UI + 1][VJ + 1];
			FVector Pt4 = Grid[UI][VJ + 1];

			// Skip degenerate
			if (FVector::Dist(Pt1, Pt2) <= 0.01f &&
			    FVector::Dist(Pt2, Pt3) <= 0.01f &&
			    FVector::Dist(Pt3, Pt4) <= 0.01f)
			{
				continue;
			}

			Spec.AddFace(
				{Pt1, Pt2, Pt3, Pt4},
				{PlanarUV(Pt1), PlanarUV(Pt2), PlanarUV(Pt3), PlanarUV(Pt4)});
		}
	}
}

void FTransitGeometry::AddCurveStrip(FTransitMeshSpec& Spec,
	const TArray<FVector>& StartCurve, const TArray<FVector>& EndCurve,
	int32 Segments, int32 Rows)
{
	if (StartCurve.Num() < 2 || EndCurve.Num() < 2) return;

	int32 USteps = FMath::Max(1, Segments >= 0 ? Segments : FMath::Max(StartCurve.Num(), EndCurve.Num()) - 1);
	Rows = FMath::Max(1, Rows);

	TArray<FVector> StartSamples = ResamplePolyline(StartCurve, USteps);
	TArray<FVector> EndSamples = ResamplePolyline(EndCurve, USteps);

	// Build grid via linear interpolation
	TArray<TArray<FVector>> Grid;
	Grid.Reserve(USteps + 1);
	for (int32 i = 0; i <= USteps; ++i)
	{
		TArray<FVector> Col;
		Col.Reserve(Rows + 1);
		for (int32 R = 0; R <= Rows; ++R)
		{
			Col.Add(FMath::Lerp(StartSamples[i], EndSamples[i], (float)R / Rows));
		}
		Grid.Add(MoveTemp(Col));
	}

	for (int32 UI = 0; UI < USteps; ++UI)
	{
		for (int32 R = 0; R < Rows; ++R)
		{
			FVector Pt1 = Grid[UI][R];
			FVector Pt2 = Grid[UI + 1][R];
			FVector Pt3 = Grid[UI + 1][R + 1];
			FVector Pt4 = Grid[UI][R + 1];

			if (FVector::Dist(Pt1, Pt2) <= 0.01f &&
			    FVector::Dist(Pt2, Pt3) <= 0.01f &&
			    FVector::Dist(Pt3, Pt4) <= 0.01f)
			{
				continue;
			}

			Spec.AddFace(
				{Pt1, Pt2, Pt3, Pt4},
				{PlanarUV(Pt1), PlanarUV(Pt2), PlanarUV(Pt3), PlanarUV(Pt4)});
		}
	}
}

// ═══════════════════════════════════════════════════════════════════════════
// Fillet arc between edges (port of _fillet_between_edges)
// ═══════════════════════════════════════════════════════════════════════════

TArray<FVector> FTransitGeometry::FilletBetweenEdges(
	const FVector& PRightPoint, const FVector& TangentRight,
	const FVector& PLeftPoint, const FVector& TangentLeft,
	float Radius, float Z, int32 Steps)
{
	float ZStart = PRightPoint.Z;
	float ZEnd = PLeftPoint.Z;

	FVector2D PA(PRightPoint.X, PRightPoint.Y);
	FVector2D PB(PLeftPoint.X, PLeftPoint.Y);
	FVector2D DA(TangentRight.X, TangentRight.Y);
	FVector2D DB(TangentLeft.X, TangentLeft.Y);

	if (DA.Size() < 0.001f || DB.Size() < 0.001f)
	{
		return {FVector(PRightPoint.X, PRightPoint.Y, ZStart), FVector(PLeftPoint.X, PLeftPoint.Y, ZEnd)};
	}

	DA.Normalize();
	DB.Normalize();

	float Denom = DA.X * DB.Y - DA.Y * DB.X;
	if (FMath::Abs(Denom) < 0.001f)
	{
		return {FVector(PRightPoint.X, PRightPoint.Y, ZStart), FVector(PLeftPoint.X, PLeftPoint.Y, ZEnd)};
	}

	float DX = PB.X - PA.X;
	float DY = PB.Y - PA.Y;
	float TVal = (DX * DB.Y - DY * DB.X) / Denom;

	FVector2D V(PA.X + DA.X * TVal, PA.Y + DA.Y * TVal);

	if (FVector2D::DotProduct(V - PA, DA) < -0.05f || FVector2D::DotProduct(V - PB, DB) < -0.05f)
	{
		return {FVector(PRightPoint.X, PRightPoint.Y, ZStart), FVector(PLeftPoint.X, PLeftPoint.Y, ZEnd)};
	}

	FVector2D VA = -DA;
	FVector2D VB = -DB;

	float DADist = (PA - V).Size();
	float DBDist = (PB - V).Size();

	float CosTheta = FMath::Clamp(FVector2D::DotProduct(VA, VB), -1.0f, 1.0f);
	float Theta = FMath::Acos(CosTheta);

	if (Theta < 0.02f || Theta > PI - 0.02f)
	{
		return {FVector(PRightPoint.X, PRightPoint.Y, ZStart), FVector(PLeftPoint.X, PLeftPoint.Y, ZEnd)};
	}

	float Alpha = Theta * 0.5f;
	float TanAlpha = FMath::Tan(Alpha);
	float SinAlpha = FMath::Sin(Alpha);

	if (TanAlpha < 0.001f || SinAlpha < 0.001f)
	{
		return {FVector(PRightPoint.X, PRightPoint.Y, ZStart), FVector(PLeftPoint.X, PLeftPoint.Y, ZEnd)};
	}

	float T = Radius / TanAlpha;
	float TMax = FMath::Min(FMath::Max(DADist, 0.0f), FMath::Max(DBDist, 0.0f)) * 0.95f;
	if (T > TMax)
	{
		T = TMax;
		Radius = T * TanAlpha;
	}

	FVector2D TPA = V + VA * T;
	FVector2D TPB = V + VB * T;

	FVector2D BDir = VA + VB;
	if (BDir.Size() < 0.001f)
	{
		return {FVector(PRightPoint.X, PRightPoint.Y, ZStart), FVector(PLeftPoint.X, PLeftPoint.Y, ZEnd)};
	}
	BDir.Normalize();

	FVector2D C = V + BDir * (Radius / SinAlpha);

	FVector2D RA2D = TPA - C;
	float AStart = FMath::Atan2(RA2D.Y, RA2D.X);
	FVector2D RB2D = TPB - C;
	float AEnd = FMath::Atan2(RB2D.Y, RB2D.X);

	float Delta = AEnd - AStart;
	while (Delta > PI) Delta -= 2.0f * PI;
	while (Delta < -PI) Delta += 2.0f * PI;

	TArray<FVector> Pts;
	Pts.Reserve(Steps + 3);
	Pts.Add(FVector(PRightPoint.X, PRightPoint.Y, ZStart));
	for (int32 I = 0; I <= Steps; ++I)
	{
		float Frac = (float)I / FMath::Max(Steps, 1);
		float Angle = AStart + Delta * Frac;
		float ArcZ = ZStart + (ZEnd - ZStart) * Frac;
		Pts.Add(FVector(C.X + FMath::Cos(Angle) * Radius, C.Y + FMath::Sin(Angle) * Radius, ArcZ));
	}
	Pts.Add(FVector(PLeftPoint.X, PLeftPoint.Y, ZEnd));
	return Pts;
}

// ═══════════════════════════════════════════════════════════════════════════
// Offset curve from targets (port of _offset_curve_from_targets)
// ═══════════════════════════════════════════════════════════════════════════

static FVector2D CurveTangentXY(const TArray<FVector>& Points, int32 Index)
{
	if (Points.Num() < 2) return FVector2D(1, 0);
	FVector Delta;
	if (Index <= 0)
		Delta = Points[1] - Points[0];
	else if (Index >= Points.Num() - 1)
		Delta = Points.Last() - Points[Points.Num() - 2];
	else
		Delta = Points[Index + 1] - Points[Index - 1];
	FVector2D T(Delta.X, Delta.Y);
	if (T.Size() <= 0.001f) return FVector2D(1, 0);
	return T.GetSafeNormal();
}

TArray<FVector> FTransitGeometry::OffsetCurveFromTargets(
	const TArray<FVector>& BaseCurve,
	const FVector& StartTarget, const FVector& EndTarget)
{
	if (BaseCurve.Num() == 0) return TArray<FVector>();
	if (BaseCurve.Num() == 1) return {StartTarget};

	FVector StartDelta = StartTarget - BaseCurve[0];
	FVector EndDelta = EndTarget - BaseCurve.Last();
	FVector2D StartXY(StartDelta.X, StartDelta.Y);
	FVector2D EndXY(EndDelta.X, EndDelta.Y);

	FVector2D StartNorm = CurveTangentXY(BaseCurve, 0);
	StartNorm = FVector2D(-StartNorm.Y, StartNorm.X);
	FVector2D EndNorm = CurveTangentXY(BaseCurve, BaseCurve.Num() - 1);
	EndNorm = FVector2D(-EndNorm.Y, EndNorm.X);

	float PosScore = FVector2D::DotProduct(StartNorm, StartXY) + FVector2D::DotProduct(EndNorm, EndXY);
	float NegScore = FVector2D::DotProduct(-StartNorm, StartXY) + FVector2D::DotProduct(-EndNorm, EndXY);
	float NormalSign = (PosScore >= NegScore) ? 1.0f : -1.0f;

	int32 Total = BaseCurve.Num() - 1;
	TArray<FVector> Result;
	Result.Reserve(BaseCurve.Num());

	for (int32 i = 0; i < BaseCurve.Num(); ++i)
	{
		float Frac = (float)i / FMath::Max(Total, 1);
		FVector2D Tangent = CurveTangentXY(BaseCurve, i);
		FVector2D Normal(-Tangent.Y, Tangent.X);
		if (Normal.Size() <= 0.001f)
		{
			Normal = StartXY.Size() > 0.001f ? StartXY.GetSafeNormal() : FVector2D(0, 1);
		}
		else
		{
			Normal.Normalize();
		}
		Normal *= NormalSign;

		float Width = (1.0f - Frac) * StartXY.Size() + Frac * EndXY.Size();
		float ZOff = (1.0f - Frac) * StartDelta.Z + Frac * EndDelta.Z;
		const FVector& Pt = BaseCurve[i];
		Result.Add(FVector(Pt.X + Normal.X * Width, Pt.Y + Normal.Y * Width, Pt.Z + ZOff));
	}

	Result[0] = StartTarget;
	Result.Last() = EndTarget;
	return Result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Approach frame (port of _approach_frame)
// ═══════════════════════════════════════════════════════════════════════════

FTransitGeometry::FApproachFrame FTransitGeometry::ComputeApproachFrame(
	const FTransitGraph& Graph, const FCorridorEdge& Edge,
	const FString& NodeId, float TrimDistance, bool bForceEnd)
{
	FApproachFrame Frame;
	Frame.EdgeId = Edge.EdgeId;

	float TotalLength = PolylineLength(Edge.Points);
	bool bIsStart = (NodeId == Edge.StartNodeId) && !bForceEnd;
	float Dist = FMath::Min(TrimDistance, TotalLength * 0.49f);

	if (bIsStart)
	{
		Frame.Point = PointOnPolyline(Edge.Points, Dist);
	}
	else
	{
		Frame.Point = PointOnPolyline(Edge.Points, TotalLength - Dist);
	}

	// Compute tangent from trimmed polyline (same as segment mesh boundary)
	FVector Tangent = FVector::ZeroVector;
	{
		const FJunctionNode* StartN = Graph.Nodes.Find(Edge.StartNodeId);
		const FJunctionNode* EndN = Graph.Nodes.Find(Edge.EndNodeId);
		float ST = (StartN && NodeGeneratesJunction(Graph, *StartN)) ? StartN->CornerRadius : 0.0f;
		float ET = (EndN && NodeGeneratesJunction(Graph, *EndN)) ? EndN->CornerRadius : 0.0f;
		TArray<FVector> Trimmed = TrimPolyline(Edge.Points, ST, ET);
		if (Trimmed.Num() >= 2)
		{
			if (bIsStart)
			{
				FVector RoadTangent = SafeNormalized(Trimmed[1] - Trimmed[0], FVector(1, 0, 0));
				Tangent = -RoadTangent;
			}
			else
			{
				FVector RoadTangent = SafeNormalized(Trimmed.Last() - Trimmed[Trimmed.Num() - 2], FVector(1, 0, 0));
				Tangent = RoadTangent;
			}
		}
	}

	// Fallback
	if (Tangent.IsNearlyZero(0.001f))
	{
		TArray<float> Cum = CumulativeLengths(Edge.Points);
		if (bIsStart)
		{
			FVector PNext = Edge.Points.Last();
			for (int32 i = 0; i < Edge.Points.Num(); ++i)
			{
				if (Cum[i] > Dist + 0.001f) { PNext = Edge.Points[i]; break; }
			}
			Tangent = -SafeNormalized(PNext - Frame.Point, FVector(1, 0, 0));
		}
		else
		{
			float TrimDistFromStart = TotalLength - Dist;
			FVector PPrev = Edge.Points[0];
			for (int32 i = Edge.Points.Num() - 1; i >= 0; --i)
			{
				if (Cum[i] < TrimDistFromStart - 0.001f) { PPrev = Edge.Points[i]; break; }
			}
			Tangent = SafeNormalized(Frame.Point - PPrev, FVector(1, 0, 0));
		}
	}

	Frame.Tangent = Tangent;

	FVector UpRef(0, 0, 1);
	if (FMath::Abs(FVector::DotProduct(Tangent, UpRef)) > 0.98f)
	{
		UpRef = FVector(0, 1, 0);
	}
	Frame.Left = SafeNormalized(FVector::CrossProduct(UpRef, Tangent), FVector(0, 1, 0));
	Frame.Up = SafeNormalized(FVector::CrossProduct(Tangent, Frame.Left), FVector(0, 0, 1));

	const FResolvedTransitProfile& P = Edge.Profile;
	Frame.RoadLeft  = Frame.Point + Frame.Left * P.RoadHalfWidth();
	Frame.RoadRight = Frame.Point - Frame.Left * P.RoadHalfWidth();
	Frame.CurbLeft  = Frame.Point + Frame.Left * (P.RoadHalfWidth() + P.CurbWidth) + Frame.Up * P.CurbHeight;
	Frame.CurbRight = Frame.Point - Frame.Left * (P.RoadHalfWidth() + P.CurbWidth) + Frame.Up * P.CurbHeight;
	Frame.PaveLeft  = Frame.Point + Frame.Left * P.LeftTotalHalfWidth() + Frame.Up * P.CurbHeight;
	Frame.PaveRight = Frame.Point - Frame.Left * P.RightTotalHalfWidth() + Frame.Up * P.CurbHeight;

	Frame.Angle = FMath::Atan2(Tangent.Y, Tangent.X);

	return Frame;
}

// ═══════════════════════════════════════════════════════════════════════════
// Junction mesh (port of build_junction_mesh)
// ═══════════════════════════════════════════════════════════════════════════

void FTransitGeometry::BuildJunctionMesh(const FTransitGraph& Graph, const FJunctionNode& Node,
                                          FTransitMeshSpec& OutRoad, FTransitMeshSpec& OutPavement)
{
	OutRoad.Reset();
	OutPavement.Reset();

	if (!NodeGeneratesJunction(Graph, Node)) return;

	// Collect approach frames sorted by angle
	TArray<FApproachFrame> Approaches;
	TSet<FString> SeenEdgeIds;

	for (const FString& EdgeId : Node.EdgeIds)
	{
		const FCorridorEdge* Edge = Graph.Edges.Find(EdgeId);
		if (!Edge) continue;

		bool bIsLoop = Edge->StartNodeId == Edge->EndNodeId;
		bool bForceEnd = bIsLoop && SeenEdgeIds.Contains(EdgeId);
		SeenEdgeIds.Add(EdgeId);

		FApproachFrame Frame = ComputeApproachFrame(Graph, *Edge, Node.NodeId, Node.CornerRadius, bForceEnd);
		Approaches.Add(Frame);
	}

	if (Approaches.Num() < 2) return;

	Approaches.Sort([](const FApproachFrame& A, const FApproachFrame& B) { return A.Angle < B.Angle; });

	int32 N = Approaches.Num();
	float FilletRadius = FMath::Max(Node.CornerRadius * 0.6f, 150.0f); // 1.5m min

	// Compute fillet arcs for each corner
	TArray<TArray<FVector>> RoadFilletArcs;
	TArray<TArray<FVector>> CurbFilletArcs;
	TArray<TArray<FVector>> PaveFilletArcs;
	RoadFilletArcs.SetNum(N);
	CurbFilletArcs.SetNum(N);
	PaveFilletArcs.SetNum(N);

	for (int32 I = 0; I < N; ++I)
	{
		const FApproachFrame& Cur = Approaches[I];
		const FApproachFrame& Nxt = Approaches[(I + 1) % N];

		RoadFilletArcs[I] = FilletBetweenEdges(
			Cur.RoadRight, Cur.Tangent,
			Nxt.RoadLeft, Nxt.Tangent,
			FilletRadius, Node.Co.Z, 8);
	}

	// Resample and compute curb/pave offsets
	constexpr int32 TargetFilletSegments = 10;
	for (int32 I = 0; I < N; ++I)
	{
		int32 Segments = FMath::Max(TargetFilletSegments, RoadFilletArcs[I].Num() - 1);
		RoadFilletArcs[I] = ResamplePolyline(RoadFilletArcs[I], Segments);

		const FApproachFrame& Cur = Approaches[I];
		const FApproachFrame& Nxt = Approaches[(I + 1) % N];

		CurbFilletArcs[I] = OffsetCurveFromTargets(RoadFilletArcs[I], Cur.CurbRight, Nxt.CurbLeft);
		PaveFilletArcs[I] = OffsetCurveFromTargets(CurbFilletArcs[I], Cur.PaveRight, Nxt.PaveLeft);
	}

	// Road surface: Coons patches (2 per corner, split at midpoint)
	FVector C = Node.Co;

	for (int32 I = 0; I < N; ++I)
	{
		const FApproachFrame& Cur = Approaches[I];
		const FApproachFrame& Nxt = Approaches[(I + 1) % N];
		const TArray<FVector>& Fillet = RoadFilletArcs[I];

		int32 TotalPts = Fillet.Num();
		int32 Half1 = (TotalPts - 1) / 2 + 1;

		TArray<FVector> Right1;
		for (int32 J = 0; J < Half1; ++J) Right1.Add(Fillet[J]);

		TArray<FVector> Right2;
		for (int32 J = Half1 - 1; J < TotalPts; ++J) Right2.Add(Fillet[J]);

		// Patch 1: approach I right half to diagonal
		{
			int32 Steps1 = FMath::Max(1, Right1.Num() - 1);
			TArray<FVector> BBottom = {Cur.Point, Cur.RoadRight};
			TArray<FVector> BTop = {C, Right1.Last()};
			TArray<FVector> BLeft;
			BLeft.Reserve(Steps1 + 1);
			for (int32 J = 0; J <= Steps1; ++J)
			{
				float Frac = (float)J / Steps1;
				float EasedFrac = 1.0f - FMath::Pow(1.0f - Frac, 1.5f);
				BLeft.Add(FMath::Lerp(Cur.Point, C, EasedFrac));
			}
			AddQuadPatch(OutRoad, BBottom, BTop, BLeft, Right1);
		}

		// Patch 2: diagonal to approach I+1 left half
		{
			int32 Steps2 = FMath::Max(1, Right2.Num() - 1);
			TArray<FVector> BBottom = {C, Right2[0]};
			TArray<FVector> BTop = {Nxt.Point, Nxt.RoadLeft};
			TArray<FVector> BLeft;
			BLeft.Reserve(Steps2 + 1);
			for (int32 J = 0; J <= Steps2; ++J)
			{
				float Frac = (float)J / Steps2;
				float EasedFrac = FMath::Pow(Frac, 1.5f);
				BLeft.Add(FMath::Lerp(C, Nxt.Point, EasedFrac));
			}
			AddQuadPatch(OutRoad, BBottom, BTop, BLeft, Right2);
		}
	}

	// Curb strips and pavement strips per corner
	for (int32 I = 0; I < N; ++I)
	{
		AddCurveStrip(OutRoad, RoadFilletArcs[I], CurbFilletArcs[I],
			RoadFilletArcs[I].Num() - 1, 1);
		AddCurveStrip(OutPavement, CurbFilletArcs[I], PaveFilletArcs[I],
			CurbFilletArcs[I].Num() - 1, 2);
	}

	OutRoad.RecalcNormals();
	OutPavement.RecalcNormals();
}
