#include "RockFormationActor.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "UObject/ConstructorHelpers.h"

ARockFormationActor::ARockFormationActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	GeneratedMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"));
	GeneratedMesh->SetupAttachment(Root);
	GeneratedMesh->bUseAsyncCooking = true;
}

// ---------------------------------------------------------------------------
//  Spline gathering
// ---------------------------------------------------------------------------

TArray<TArray<FVector>> ARockFormationActor::GatherSplinePolylines() const
{
	TArray<TArray<FVector>> Result;

	for (const TSoftObjectPtr<AActor>& Ref : SplineActors)
	{
		AActor* Actor = Ref.Get();
		if (!IsValid(Actor)) continue;

		TArray<UActorComponent*> Comps;
		Actor->GetComponents(USplineComponent::StaticClass(), Comps);

		for (UActorComponent* C : Comps)
		{
			USplineComponent* Spline = Cast<USplineComponent>(C);
			if (!Spline || Spline->GetNumberOfSplinePoints() < 2) continue;

			float SplineLen = Spline->GetSplineLength();
			int32 NumSamples = FMath::Max(2, FMath::CeilToInt32(SplineLen / 10.f));

			TArray<FVector> Pts;
			Pts.Reserve(NumSamples);
			for (int32 S = 0; S < NumSamples; S++)
			{
				float Dist = SplineLen * S / (NumSamples - 1);
				Pts.Add(Spline->GetWorldLocationAtDistanceAlongSpline(Dist));
			}
			if (Pts.Num() >= 2)
			{
				Result.Add(MoveTemp(Pts));
			}
		}
	}

	return Result;
}

// ---------------------------------------------------------------------------
//  Polyline helpers
// ---------------------------------------------------------------------------

float ARockFormationActor::PolylineLength(const TArray<FVector>& Pts)
{
	float Len = 0.f;
	for (int32 i = 0; i + 1 < Pts.Num(); i++)
	{
		Len += FVector::Dist(Pts[i], Pts[i + 1]);
	}
	return Len;
}

void ARockFormationActor::PointAtT(const TArray<FVector>& Pts, float T,
                                    FVector& OutPos, FVector& OutTangent)
{
	if (Pts.Num() < 2)
	{
		OutPos = Pts.Num() > 0 ? Pts[0] : FVector::ZeroVector;
		OutTangent = FVector::UpVector;
		return;
	}

	float Total = PolylineLength(Pts);
	if (Total < SMALL_NUMBER)
	{
		OutPos = Pts[0];
		OutTangent = FVector::UpVector;
		return;
	}

	float Target = T * Total;
	float Accum = 0.f;
	for (int32 i = 0; i + 1 < Pts.Num(); i++)
	{
		FVector Seg = Pts[i + 1] - Pts[i];
		float SegLen = Seg.Size();
		if (Accum + SegLen >= Target || i == Pts.Num() - 2)
		{
			float Frac = SegLen > SMALL_NUMBER ? (Target - Accum) / SegLen : 0.f;
			Frac = FMath::Clamp(Frac, 0.f, 1.f);
			OutPos = FMath::Lerp(Pts[i], Pts[i + 1], Frac);
			OutTangent = SegLen > SMALL_NUMBER ? Seg / SegLen : FVector::UpVector;
			return;
		}
		Accum += SegLen;
	}

	OutPos = Pts.Last();
	OutTangent = (Pts.Last() - Pts[Pts.Num() - 2]).GetSafeNormal();
}

// ---------------------------------------------------------------------------
//  Add transformed rock to combined arrays
// ---------------------------------------------------------------------------

void ARockFormationActor::AddRockToCombined(
	TArray<FVector>& AllVerts, TArray<int32>& AllTris,
	TArray<FVector>& AllNormals, TArray<FVector2D>& AllUVs,
	const FRockMeshData& Rock,
	const FVector& Location, const FRotator& Rotation,
	const FVector& Scale) const
{
	int32 BaseIdx = AllVerts.Num();
	FTransform XForm(Rotation, Location, Scale);

	AllVerts.Reserve(AllVerts.Num() + Rock.Vertices.Num());
	AllNormals.Reserve(AllNormals.Num() + Rock.Normals.Num());
	AllUVs.Reserve(AllUVs.Num() + Rock.UVs.Num());

	for (int32 i = 0; i < Rock.Vertices.Num(); i++)
	{
		AllVerts.Add(XForm.TransformPosition(Rock.Vertices[i]));
		AllNormals.Add(XForm.TransformVectorNoScale(Rock.Normals[i]).GetSafeNormal());
		if (i < Rock.UVs.Num())
			AllUVs.Add(Rock.UVs[i]);
		else
			AllUVs.Add(FVector2D::ZeroVector);
	}

	AllTris.Reserve(AllTris.Num() + Rock.Triangles.Num());
	for (int32 Idx : Rock.Triangles)
	{
		AllTris.Add(BaseIdx + Idx);
	}
}

// ---------------------------------------------------------------------------
//  RebuildFormation
// ---------------------------------------------------------------------------

void ARockFormationActor::RebuildFormation()
{
	if (!IsValid(this)) return;
	if (bIsRebuilding) return;
	bIsRebuilding = true;

	GeneratedMesh->ClearAllMeshSections();
	RockCount = 0;

	TArray<TArray<FVector>> Polylines = GatherSplinePolylines();
	if (Polylines.Num() == 0)
	{
		StatusReport = TEXT("No splines found. Add spline actors to the list.");
		bIsRebuilding = false;
		return;
	}

	TArray<FVector> AllVerts;
	TArray<int32> AllTris;
	TArray<FVector> AllNormals;
	TArray<FVector2D> AllUVs;

	FRockRNG RNG(Seed);
	int32 RockID = 0;

	for (const TArray<FVector>& Pts : Polylines)
	{
		float TotalLen = PolylineLength(Pts);
		if (TotalLen < SMALL_NUMBER) continue;

		// ---- Main spine rocks ----
		int32 Count = FMath::Max(1, FMath::FloorToInt32(TotalLen / Spacing));
		for (int32 i = 0; i <= Count; i++)
		{
			float T = (float)i / Count;
			FVector Pos, Tangent;
			PointAtT(Pts, T, Pos, Tangent);

			// Jitter
			float JitterRange = Spacing * JitterAmount;
			float JX = (RNG.Next() - 0.5f) * JitterRange;
			float JY = (RNG.Next() - 0.5f) * JitterRange;
			float JZ = (RNG.Next() - 0.5f) * JitterRange;

			// Scale with randomness
			float BaseScale = Spacing * 0.8f;
			float SX = BaseScale * (1.f + (RNG.Next() - 0.5f) * ScaleRandomness);
			float SY = BaseScale * (1.f + (RNG.Next() - 0.5f) * ScaleRandomness);
			float SZ = BaseScale * (1.f + (RNG.Next() - 0.5f) * ScaleRandomness);
			FVector ScaleVec = RockGeometry::ClampStretch(FVector(SX, SY, SZ), MaxStretch);

			// Rotation from tangent
			FRotator Rot = Tangent.ToOrientationRotator();

			int32 RockSeed = (int32)(RNG.Next() * 1000000.f);
			FRockMeshData Rock = RockGeometry::CreateRock(Detail, Roughness, RockSeed, BaseShape, NoiseType);

			AddRockToCombined(AllVerts, AllTris, AllNormals, AllUVs,
			                  Rock, Pos + FVector(JX, JY, JZ), Rot, ScaleVec);
			RockID++;
		}

		// ---- Side layer ----
		if (bEnableSideLayer)
		{
			int32 SideCount = FMath::Max(1, FMath::FloorToInt32(TotalLen / SideSpacing));
			for (int32 i = 0; i <= SideCount; i++)
			{
				// Coverage check
				if (RNG.Next() > SideCoverage)
				{
					// Advance RNG to stay deterministic
					for (int32 Skip = 0; Skip < SideRocksPerPoint * 9; Skip++)
						RNG.Next();
					continue;
				}

				float T = (float)i / SideCount;
				FVector Pos, Tangent;
				PointAtT(Pts, T, Pos, Tangent);

				FVector Up(0, 0, 1);
				if (FMath::Abs(Tangent | Up) > 0.999f)
					Up = FVector(0, 1, 0);
				FVector SideDir = FVector::CrossProduct(Tangent, Up).GetSafeNormal();
				FVector Binormal = FVector::CrossProduct(Tangent, SideDir).GetSafeNormal();

				for (int32 R = 0; R < SideRocksPerPoint; R++)
				{
					float Sign = RNG.Next() < 0.5f ? 1.f : -1.f;
					float MainRadius = Spacing * 0.8f * 0.5f;
					float Extra = SideMinDistance + RNG.Next() * (SideMaxDistance - SideMinDistance);
					float Dist = MainRadius + Extra;

					float TJitter = (RNG.Next() - 0.5f) * SideJitter * SideSpacing;
					float VJitter = (RNG.Next() - 0.5f) * SideJitter * SideSpacing * 0.2f;

					float BaseS = Spacing * 0.8f * SideScale;
					float SSX = BaseS * (1.f + (RNG.Next() - 0.5f) * SideScaleRandomness);
					float SSY = BaseS * (1.f + (RNG.Next() - 0.5f) * SideScaleRandomness);
					float SSZ = BaseS * (1.f + (RNG.Next() - 0.5f) * SideScaleRandomness);
					FVector SScaleVec = RockGeometry::ClampStretch(FVector(SSX, SSY, SSZ), MaxStretch);

					float Embed = FMath::Max3(SScaleVec.X, SScaleVec.Y, SScaleVec.Z) * 0.4f;
					float FinalDist = (Dist - Embed) * Sign;

					FVector SidePos = Pos + SideDir * FinalDist + Tangent * TJitter + Binormal * VJitter;

					FRotator SideRot(
						RNG.Next() * 180.f * 0.3f,
						RNG.Next() * 360.f,
						RNG.Next() * 180.f * 0.3f
					);

					int32 RockSeed = (int32)(RNG.Next() * 1000000.f);
					FRockMeshData Rock = RockGeometry::CreateRock(
						FMath::Max(1, Detail - 1), Roughness, RockSeed, BaseShape, NoiseType);

					AddRockToCombined(AllVerts, AllTris, AllNormals, AllUVs,
					                  Rock, SidePos, SideRot, SScaleVec);
					RockID++;
				}
			}
		}

		// ---- Growth layer ----
		if (bEnableGrowthLayer)
		{
			int32 GrowthCount = FMath::Max(1, FMath::FloorToInt32(TotalLen / GrowthSpacing));
			for (int32 i = 0; i <= GrowthCount; i++)
			{
				if (RNG.Next() > GrowthCoverage)
				{
					for (int32 Skip = 0; Skip < GrowthPerPoint * 10; Skip++)
						RNG.Next();
					continue;
				}

				float T = (float)i / GrowthCount;
				FVector Pos, Tangent;
				PointAtT(Pts, T, Pos, Tangent);

				FVector Up(0, 0, 1);
				if (FMath::Abs(Tangent | Up) > 0.999f)
					Up = FVector(0, 1, 0);
				FVector SideDir = FVector::CrossProduct(Tangent, Up).GetSafeNormal();
				FVector Binormal = FVector::CrossProduct(Tangent, SideDir).GetSafeNormal();

				for (int32 G = 0; G < GrowthPerPoint; G++)
				{
					float Angle = RNG.Next() * 2.f * PI;
					float CA = FMath::Cos(Angle);
					float SA = FMath::Sin(Angle);

					bool bAllow = false;
					if (bGrowthSides && FMath::Abs(CA) > 0.3f) bAllow = true;
					if (bGrowthTop && SA > 0.3f) bAllow = true;
					if (bGrowthBottom && SA < -0.3f) bAllow = true;

					if (!bAllow)
					{
						for (int32 Skip = 0; Skip < 6; Skip++)
							RNG.Next();
						continue;
					}

					FVector ScatterDir = SideDir * CA + Binormal * SA;
					float MainRadius = Spacing * 0.8f * 0.5f;
					float GExtra = GrowthMinDist + RNG.Next() * (GrowthMaxDist - GrowthMinDist);

					float BaseS = Spacing * 0.8f * GrowthScale;
					float GSX = BaseS * (1.f + (RNG.Next() - 0.5f) * GrowthScaleRandomness);
					float GSY = BaseS * (1.f + (RNG.Next() - 0.5f) * GrowthScaleRandomness);
					float GSZ = BaseS * (1.f + (RNG.Next() - 0.5f) * GrowthScaleRandomness);
					FVector GScaleVec = RockGeometry::ClampStretch(FVector(GSX, GSY, GSZ), MaxStretch);

					float Embed = FMath::Max3(GScaleVec.X, GScaleVec.Y, GScaleVec.Z) * 0.4f;
					float FinalDist = MainRadius + GExtra - Embed;
					float GTJitter = (RNG.Next() - 0.5f) * GrowthSpacing * 0.3f;

					FVector GrowthPos = Pos + ScatterDir * FinalDist + Tangent * GTJitter;

					FRotator GrowthRot(
						RNG.Next() * 180.f * 0.4f,
						RNG.Next() * 360.f,
						RNG.Next() * 180.f * 0.4f
					);

					int32 RockSeed = (int32)(RNG.Next() * 1000000.f);
					FRockMeshData Rock = RockGeometry::CreateRock(
						FMath::Max(1, Detail - 1), Roughness, RockSeed, BaseShape, NoiseType);

					AddRockToCombined(AllVerts, AllTris, AllNormals, AllUVs,
					                  Rock, GrowthPos, GrowthRot, GScaleVec);
					RockID++;
				}
			}
		}

		// ---- Base rocks (at low-Z endpoints) ----
		if (bEnableBaseRocks)
		{
			TArray<FVector> Endpoints;
			if (Pts[0].Z < 200.f) // 200 cm threshold (Blender used 2m)
				Endpoints.Add(Pts[0]);
			if (Pts.Last().Z < 200.f)
				Endpoints.Add(Pts.Last());

			if (Endpoints.Num() > 0)
			{
				int32 PerBase = FMath::Max(1, BaseRocksDensity / Endpoints.Num());
				for (const FVector& BasePt : Endpoints)
				{
					for (int32 B = 0; B < PerBase; B++)
					{
						float BAngle = RNG.Next() * 2.f * PI;
						float BDist = FMath::Sqrt(RNG.Next()) * BaseRocksSpread;
						float BX = BasePt.X + FMath::Cos(BAngle) * BDist;
						float BY = BasePt.Y + FMath::Sin(BAngle) * BDist;

						float BaseS = Spacing * 0.8f * BaseRocksScale * (0.5f + RNG.Next() * 0.5f);
						float BSX = BaseS * (1.f + (RNG.Next() - 0.5f) * ScaleRandomness);
						float BSY = BaseS * (1.f + (RNG.Next() - 0.5f) * ScaleRandomness) * 0.5f;
						float BSZ = BaseS * (1.f + (RNG.Next() - 0.5f) * ScaleRandomness);
						FVector BScaleVec = RockGeometry::ClampStretch(FVector(BSX, BSY, BSZ), MaxStretch);

						FRotator BRot(0.f, RNG.Next() * 360.f, 0.f);

						int32 RockSeed = (int32)(RNG.Next() * 1000000.f);
						FRockMeshData Rock = RockGeometry::CreateRock(Detail, Roughness, RockSeed, BaseShape, NoiseType);

						FVector BPos(BX, BY, BasePt.Z + BScaleVec.Z * 0.5f);
						AddRockToCombined(AllVerts, AllTris, AllNormals, AllUVs,
						                  Rock, BPos, BRot, BScaleVec);
						RockID++;
					}
				}
			}
		}
	}

	// Apply to ProceduralMeshComponent
	if (AllVerts.Num() > 0)
	{
		TArray<FLinearColor> EmptyColors;
		TArray<FProcMeshTangent> EmptyTangents;

		GeneratedMesh->CreateMeshSection_LinearColor(
			0, AllVerts, AllTris, AllNormals, AllUVs, EmptyColors, EmptyTangents, true);

		if (RockMaterial)
		{
			GeneratedMesh->SetMaterial(0, RockMaterial);
		}
	}

	RockCount = RockID;
	StatusReport = FString::Printf(TEXT("Generated %d rocks across %d splines"),
	                                RockID, Polylines.Num());

	bIsRebuilding = false;
}

// ---------------------------------------------------------------------------
//  BakeMesh – convert PMC to a StaticMeshActor
// ---------------------------------------------------------------------------

void ARockFormationActor::BakeMesh()
{
	if (!GeneratedMesh || GeneratedMesh->GetNumSections() == 0)
	{
		StatusReport = TEXT("Nothing to bake. Rebuild first.");
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	const FProcMeshSection* Section = GeneratedMesh->GetProcMeshSection(0);
	if (!Section || Section->ProcVertexBuffer.Num() == 0) return;

	// Build FMeshDescription directly
	FMeshDescription MeshDesc;
	FStaticMeshAttributes StaticMeshAttrs(MeshDesc);
	StaticMeshAttrs.Register();

	int32 NumVerts = Section->ProcVertexBuffer.Num();
	int32 NumTris = Section->ProcIndexBuffer.Num() / 3;

	MeshDesc.ReserveNewVertices(NumVerts);
	MeshDesc.ReserveNewVertexInstances(NumVerts);
	MeshDesc.ReserveNewPolygons(NumTris);
	MeshDesc.ReserveNewEdges(NumTris * 3);

	// Get attribute accessors
	TVertexAttributesRef<FVector3f> VertexPositions =
		StaticMeshAttrs.GetVertexPositions();
	TVertexInstanceAttributesRef<FVector3f> VertexNormals =
		StaticMeshAttrs.GetVertexInstanceNormals();
	TVertexInstanceAttributesRef<FVector2f> VertexUVs =
		StaticMeshAttrs.GetVertexInstanceUVs();

	// Create polygon group
	FPolygonGroupID PolyGroup = MeshDesc.CreatePolygonGroup();

	// Add vertices and instances
	TArray<FVertexInstanceID> VtxInstances;
	VtxInstances.Reserve(NumVerts);

	for (int32 V = 0; V < NumVerts; V++)
	{
		const FProcMeshVertex& PMV = Section->ProcVertexBuffer[V];
		FVertexID VID = MeshDesc.CreateVertex();
		VertexPositions[VID] = FVector3f(PMV.Position);

		FVertexInstanceID VIID = MeshDesc.CreateVertexInstance(VID);
		VertexNormals[VIID] = FVector3f(PMV.Normal);
		VertexUVs.Set(VIID, 0, FVector2f(PMV.UV0));
		VtxInstances.Add(VIID);
	}

	// Add triangles
	for (int32 T = 0; T + 2 < Section->ProcIndexBuffer.Num(); T += 3)
	{
		TArray<FVertexInstanceID> TriVerts;
		TriVerts.Add(VtxInstances[Section->ProcIndexBuffer[T]]);
		TriVerts.Add(VtxInstances[Section->ProcIndexBuffer[T + 1]]);
		TriVerts.Add(VtxInstances[Section->ProcIndexBuffer[T + 2]]);

		TArray<FEdgeID> NewEdges;
		MeshDesc.CreatePolygon(PolyGroup, TriVerts, &NewEdges);
	}

	// Create StaticMesh
	FString MeshName = FString::Printf(TEXT("RockFormation_Baked_%d"), FMath::Rand());
	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(GetTransientPackage(), *MeshName, RF_Transient);

	StaticMesh->InitResources();
	StaticMesh->SetLightingGuid();

	// Commit mesh description as LOD 0
	TArray<const FMeshDescription*> MeshDescs;
	MeshDescs.Add(&MeshDesc);
	UStaticMesh::FBuildMeshDescriptionsParams Params;
	Params.bFastBuild = true;
	StaticMesh->BuildFromMeshDescriptions(MeshDescs, Params);

	if (RockMaterial && StaticMesh->GetStaticMaterials().Num() > 0)
	{
		StaticMesh->GetStaticMaterials()[0].MaterialInterface = RockMaterial;
	}

	// Spawn StaticMeshActor
	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = FName(*FString::Printf(TEXT("RockFormation_Baked")));
	AStaticMeshActor* BakedActor = World->SpawnActor<AStaticMeshActor>(
		GetActorLocation(), GetActorRotation(), SpawnParams);

	if (BakedActor)
	{
		BakedActor->GetStaticMeshComponent()->SetStaticMesh(StaticMesh);
		BakedActor->SetActorLabel(TEXT("RockFormation_Baked"));
		StatusReport = FString::Printf(TEXT("Baked %d verts to StaticMesh"), NumVerts);
	}
}

// ---------------------------------------------------------------------------
//  ClearFormation
// ---------------------------------------------------------------------------

void ARockFormationActor::ClearFormation()
{
	if (GeneratedMesh)
	{
		GeneratedMesh->ClearAllMeshSections();
	}
	RockCount = 0;
	StatusReport = TEXT("Cleared.");
}

// ---------------------------------------------------------------------------
//  Editor hooks
// ---------------------------------------------------------------------------

void ARockFormationActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bAutoRebuild && !bIsRebuilding)
	{
		RebuildFormation();
	}
}

#if WITH_EDITOR
void ARockFormationActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	if (bAutoRebuild && !bIsRebuilding)
	{
		RebuildFormation();
	}
}
#endif
