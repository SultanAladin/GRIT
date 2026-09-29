// TransitMeshSpec.h — Procedural mesh builder with vertex deduplication
#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"

struct TRANSITARCHITECTRUNTIME_API FTransitMeshSpec
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	// Vertex deduplication cache: quantized position -> index
	TMap<FIntVector, int32> VertexCache;

	// Add a vertex with dedup (returns index). Quantizes to 0.01 cm precision.
	int32 AddVertex(const FVector& Position, const FVector2D& UV, const FVector& Normal = FVector::UpVector);

	// Add a single triangle (3 indices, CCW winding)
	void AddTriangle(int32 A, int32 B, int32 C);

	// Add a quad as 2 triangles. Vertices in order: (A,B,C,D) where ABCD is CCW.
	void AddQuad(int32 A, int32 B, int32 C, int32 D);

	// Add a face from world positions + UVs + material. Handles quads (2 tris) and tris.
	void AddFace(const TArray<FVector>& Coords, const TArray<FVector2D>& FaceUVs);

	// Fan-triangulate a convex polygon from world positions (for end caps)
	void AddTriangulatedPolygon(const TArray<FVector>& Polygon);

	// Recompute normals from triangle faces (area-weighted vertex normals)
	void RecalcNormals();

	// Reverse winding order of all triangles (flips face normals)
	void FlipWinding();

	// Duplicate all triangles with reversed winding for double-sided rendering
	void MakeDoubleSided();

	// Reset all data
	void Reset();

	bool IsEmpty() const { return Vertices.Num() == 0; }
};
