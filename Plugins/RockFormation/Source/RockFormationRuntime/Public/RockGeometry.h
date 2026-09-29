#pragma once

#include "CoreMinimal.h"
#include "RockFormationTypes.h"

struct FRockMeshData
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
};

// Seeded LCG PRNG matching the Blender plugin
struct FRockRNG
{
	uint32 State;

	explicit FRockRNG(int32 Seed) : State(static_cast<uint32>(Seed) % 4294967296u) {}

	float Next()
	{
		State = (State * 1664525u + 1013904223u);
		return static_cast<float>(State) / 4294967296.0f;
	}
};

namespace RockGeometry
{
	// Create a single deformed rock mesh at origin, radius 1
	FRockMeshData CreateRock(int32 Detail, float Roughness, int32 SeedOffset,
	                         ERockBaseShape Shape, ERockNoiseType NoiseType);

	// Create an icosphere (subdivided icosahedron)
	void MakeIcosphere(TArray<FVector>& OutVerts, TArray<int32>& OutTris,
	                   float Radius, int32 Subdivisions);

	// Sample noise for displacement
	float SampleNoise(const FVector& Pos, ERockNoiseType Type, FRockRNG& RNG);

	// Box-project UVs onto mesh
	void BoxProjectUVs(FRockMeshData& Mesh);

	// Clamp scale stretch
	FVector ClampStretch(const FVector& Scale, float MaxRatio);
}
