// TransitMeshSpec.cpp
#include "TransitMeshSpec.h"

int32 FTransitMeshSpec::AddVertex(const FVector& Position, const FVector2D& UV, const FVector& Normal)
{
	// Quantize to 0.01 cm for dedup
	FIntVector Key(
		FMath::RoundToInt(Position.X * 100.0f),
		FMath::RoundToInt(Position.Y * 100.0f),
		FMath::RoundToInt(Position.Z * 100.0f));

	if (const int32* Cached = VertexCache.Find(Key))
	{
		return *Cached;
	}

	int32 Index = Vertices.Num();
	Vertices.Add(Position);
	UVs.Add(UV);
	Normals.Add(Normal);
	VertexColors.Add(FLinearColor::White);
	Tangents.Add(FProcMeshTangent(FVector(1, 0, 0), false));
	VertexCache.Add(Key, Index);
	return Index;
}

void FTransitMeshSpec::AddTriangle(int32 A, int32 B, int32 C)
{
	// Skip degenerate
	if (A == B || B == C || A == C)
	{
		return;
	}
	Triangles.Add(A);
	Triangles.Add(B);
	Triangles.Add(C);
}

void FTransitMeshSpec::AddQuad(int32 A, int32 B, int32 C, int32 D)
{
	AddTriangle(A, B, C);
	AddTriangle(A, C, D);
}

void FTransitMeshSpec::AddFace(const TArray<FVector>& Coords, const TArray<FVector2D>& FaceUVs)
{
	if (Coords.Num() < 3 || FaceUVs.Num() < Coords.Num())
	{
		return;
	}

	TArray<int32> Indices;
	Indices.Reserve(Coords.Num());
	for (int32 i = 0; i < Coords.Num(); ++i)
	{
		Indices.Add(AddVertex(Coords[i], FaceUVs[i]));
	}

	// Check for degenerate (all same index)
	bool AllSame = true;
	for (int32 i = 1; i < Indices.Num(); ++i)
	{
		if (Indices[i] != Indices[0])
		{
			AllSame = false;
			break;
		}
	}
	if (AllSame) return;

	if (Coords.Num() == 3)
	{
		AddTriangle(Indices[0], Indices[1], Indices[2]);
	}
	else if (Coords.Num() == 4)
	{
		AddQuad(Indices[0], Indices[1], Indices[2], Indices[3]);
	}
	else
	{
		// Fan triangulation for N-gons
		for (int32 i = 1; i < Indices.Num() - 1; ++i)
		{
			AddTriangle(Indices[0], Indices[i], Indices[i + 1]);
		}
	}
}

void FTransitMeshSpec::AddTriangulatedPolygon(const TArray<FVector>& Polygon)
{
	if (Polygon.Num() < 3) return;

	// Clean: remove near-duplicate consecutive vertices
	TArray<FVector> Cleaned;
	Cleaned.Add(Polygon[0]);
	for (int32 i = 1; i < Polygon.Num(); ++i)
	{
		if (FVector::Dist(Cleaned.Last(), Polygon[i]) > 0.01f)
		{
			Cleaned.Add(Polygon[i]);
		}
	}
	if (Cleaned.Num() > 1 && FVector::Dist(Cleaned.Last(), Cleaned[0]) <= 0.01f)
	{
		Cleaned.Pop();
	}
	if (Cleaned.Num() < 3) return;

	// Compute planar UV
	TArray<FVector2D> PolyUVs;
	PolyUVs.Reserve(Cleaned.Num());
	constexpr float UVScale = 0.01f; // 1 UV unit per 100 cm
	for (const FVector& P : Cleaned)
	{
		PolyUVs.Add(FVector2D(P.X * UVScale, P.Y * UVScale));
	}

	// Fan triangulation from vertex 0
	TArray<int32> Indices;
	Indices.Reserve(Cleaned.Num());
	for (int32 i = 0; i < Cleaned.Num(); ++i)
	{
		Indices.Add(AddVertex(Cleaned[i], PolyUVs[i]));
	}

	for (int32 i = 1; i < Indices.Num() - 1; ++i)
	{
		// Skip degenerate triangles
		FVector E1 = Cleaned[i] - Cleaned[0];
		FVector E2 = Cleaned[i + 1] - Cleaned[0];
		if (FVector::CrossProduct(E1, E2).Size() < 0.01f)
		{
			continue;
		}
		AddTriangle(Indices[0], Indices[i], Indices[i + 1]);
	}
}

void FTransitMeshSpec::RecalcNormals()
{
	// Zero out existing normals
	for (FVector& N : Normals)
	{
		N = FVector::ZeroVector;
	}

	// Accumulate face normals (area-weighted)
	for (int32 i = 0; i < Triangles.Num(); i += 3)
	{
		int32 I0 = Triangles[i];
		int32 I1 = Triangles[i + 1];
		int32 I2 = Triangles[i + 2];

		if (!Vertices.IsValidIndex(I0) || !Vertices.IsValidIndex(I1) || !Vertices.IsValidIndex(I2))
		{
			continue;
		}

		FVector E1 = Vertices[I1] - Vertices[I0];
		FVector E2 = Vertices[I2] - Vertices[I0];
		FVector FaceNormal = FVector::CrossProduct(E1, E2); // not normalized = area weighted

		Normals[I0] += FaceNormal;
		Normals[I1] += FaceNormal;
		Normals[I2] += FaceNormal;
	}

	// Normalize
	for (FVector& N : Normals)
	{
		if (!N.Normalize())
		{
			N = FVector::UpVector;
		}
	}
}

void FTransitMeshSpec::FlipWinding()
{
	for (int32 i = 0; i < Triangles.Num() - 2; i += 3)
	{
		Swap(Triangles[i + 1], Triangles[i + 2]);
	}
	for (FVector& N : Normals)
	{
		N = -N;
	}
}

void FTransitMeshSpec::MakeDoubleSided()
{
	int32 OrigVertCount = Vertices.Num();
	int32 OrigTriCount = Triangles.Num();

	// Duplicate all vertices with flipped normals
	for (int32 i = 0; i < OrigVertCount; ++i)
	{
		Vertices.Add(Vertices[i]);
		UVs.Add(UVs[i]);
		Normals.Add(-Normals[i]);
		VertexColors.Add(VertexColors[i]);
		Tangents.Add(Tangents[i]);
	}

	// Duplicate all triangles with reversed winding, offset by OrigVertCount
	for (int32 i = 0; i < OrigTriCount; i += 3)
	{
		Triangles.Add(Triangles[i] + OrigVertCount);
		Triangles.Add(Triangles[i + 2] + OrigVertCount);
		Triangles.Add(Triangles[i + 1] + OrigVertCount);
	}
}

void FTransitMeshSpec::Reset()
{
	Vertices.Reset();
	Triangles.Reset();
	Normals.Reset();
	UVs.Reset();
	VertexColors.Reset();
	Tangents.Reset();
	VertexCache.Reset();
}
