// RoadSegment.cpp — Spline road mesh implementation
#include "RoadSegment.h"
#include "RoadIntersection.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

ARoadSegment::ARoadSegment()
{
	PrimaryActorTick.bCanEverTick = false;

	RoadSpline = CreateDefaultSubobject<USplineComponent>(TEXT("RoadSpline"));
	RootComponent = RoadSpline;
	RoadSpline->SetMobility(EComponentMobility::Movable);
}

void ARoadSegment::Initialize(ARoadIntersection* Start, FName StartSock, ARoadIntersection* End, FName EndSock)
{
	StartIntersection = Start;
	StartSocket = StartSock;
	EndIntersection = End;
	EndSocket = EndSock;

	// Inherit defaults from the start intersection
	if (Start)
	{
		if (!RoadMesh)
		{
			RoadMesh = Start->DefaultRoadMesh;
		}
		if (!RoadMaterial)
		{
			RoadMaterial = Start->DefaultRoadMaterial;
		}
		if (SegmentLength == 500.0f)
		{
			SegmentLength = Start->DefaultSegmentLength;
		}
	}

	RebuildRoad();
}

void ARoadSegment::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Only rebuild if we have valid connection data (avoids rebuilding on initial spawn before Initialize)
	if (StartIntersection && EndIntersection)
	{
		RebuildRoad();
	}
}

#if WITH_EDITOR
void ARoadSegment::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (StartIntersection && EndIntersection)
	{
		RebuildRoad();
	}
}
#endif

void ARoadSegment::RebuildRoad()
{
	// Destroy existing mesh segments
	for (USplineMeshComponent* SMC : MeshSegments)
	{
		if (SMC)
		{
			SMC->DestroyComponent();
		}
	}
	MeshSegments.Empty();

	BuildSpline();
	BuildMesh();
}

void ARoadSegment::BuildSpline()
{
	if (!StartIntersection || !EndIntersection || !RoadSpline)
	{
		return;
	}

	UStaticMeshComponent* StartMeshComp = StartIntersection->IntersectionMesh;
	UStaticMeshComponent* EndMeshComp = EndIntersection->IntersectionMesh;
	if (!StartMeshComp || !EndMeshComp)
	{
		return;
	}

	// Get world-space socket transforms
	FTransform StartXform = StartMeshComp->GetSocketTransform(StartSocket, RTS_World);
	FTransform EndXform = EndMeshComp->GetSocketTransform(EndSocket, RTS_World);

	FVector StartPos = StartXform.GetLocation();
	FVector EndPos = EndXform.GetLocation();

	// Socket forward vectors define the outgoing road tangent direction
	FVector StartForward = StartXform.GetRotation().GetForwardVector();
	FVector EndForward = EndXform.GetRotation().GetForwardVector();

	float Distance = FVector::Dist(StartPos, EndPos);
	float TangentMagnitude = Distance * 0.5f;

	// Clear and rebuild spline with 2 points
	RoadSpline->ClearSplinePoints(false);
	RoadSpline->AddSplinePoint(StartPos, ESplineCoordinateSpace::World, false);
	RoadSpline->AddSplinePoint(EndPos, ESplineCoordinateSpace::World, false);

	// Set tangents: start tangent follows socket forward, end tangent opposes socket forward
	// (end socket points outward from its intersection, so we negate to point toward start)
	RoadSpline->SetTangentAtSplinePoint(0, StartForward * TangentMagnitude, ESplineCoordinateSpace::World, false);
	RoadSpline->SetTangentAtSplinePoint(1, -EndForward * TangentMagnitude, ESplineCoordinateSpace::World, true);
}

void ARoadSegment::BuildMesh()
{
	if (!RoadSpline || !RoadMesh)
	{
		return;
	}

	float TotalLength = RoadSpline->GetSplineLength();
	if (TotalLength <= 0.0f || SegmentLength <= 0.0f)
	{
		return;
	}

	int32 SegCount = FMath::CeilToInt(TotalLength / SegmentLength);

	for (int32 i = 0; i < SegCount; ++i)
	{
		float StartDist = i * SegmentLength;
		float EndDist = FMath::Min((i + 1) * SegmentLength, TotalLength);

		// Sample spline at segment boundaries
		FVector StartPos = RoadSpline->GetLocationAtDistanceAlongSpline(StartDist, ESplineCoordinateSpace::Local);
		FVector StartTangent = RoadSpline->GetTangentAtDistanceAlongSpline(StartDist, ESplineCoordinateSpace::Local);
		FVector EndPos = RoadSpline->GetLocationAtDistanceAlongSpline(EndDist, ESplineCoordinateSpace::Local);
		FVector EndTangent = RoadSpline->GetTangentAtDistanceAlongSpline(EndDist, ESplineCoordinateSpace::Local);

		// Scale tangents to match segment length (USplineMeshComponent expects tangents proportional to segment span)
		float SegFraction = (EndDist - StartDist) / TotalLength;
		StartTangent *= SegFraction;
		EndTangent *= SegFraction;

		// Check for feature overrides at this segment
		UStaticMesh* MeshToUse = RoadMesh;
		UMaterialInterface* MatToUse = RoadMaterial;
		FVector FeatureScale = FVector(1.0f);

		for (const FRoadFeature& Feature : Features)
		{
			float FeatureEnd = Feature.Length > 0.0f ? Feature.SplineDistance + Feature.Length : Feature.SplineDistance + SegmentLength;
			bool bOverlaps = Feature.SplineDistance < EndDist && FeatureEnd > StartDist;

			if (bOverlaps)
			{
				if (Feature.OverrideMesh)
				{
					MeshToUse = Feature.OverrideMesh;
				}
				if (Feature.OverrideMaterial)
				{
					MatToUse = Feature.OverrideMaterial;
				}
				FeatureScale = Feature.Scale;
				break; // First matching feature wins
			}
		}

		// Create spline mesh component
		USplineMeshComponent* SMC = NewObject<USplineMeshComponent>(this);
		SMC->SetMobility(EComponentMobility::Movable);
		SMC->SetStaticMesh(MeshToUse);
		SMC->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
		SMC->SetStartScale(FVector2D(FeatureScale.X, FeatureScale.Y));
		SMC->SetEndScale(FVector2D(FeatureScale.X, FeatureScale.Y));

		if (MatToUse)
		{
			SMC->SetMaterial(0, MatToUse);
		}

		SMC->SetForwardAxis(ESplineMeshAxis::X);
		SMC->AttachToComponent(RoadSpline, FAttachmentTransformRules::KeepRelativeTransform);
		SMC->RegisterComponent();

		MeshSegments.Add(SMC);
	}
}
