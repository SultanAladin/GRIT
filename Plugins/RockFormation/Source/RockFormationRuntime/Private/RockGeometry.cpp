#include "RockGeometry.h"

// ---- Icosphere generation ----

static void SubdivideTriangle(
	TArray<FVector>& Verts, TArray<int32>& Tris,
	TMap<TPair<int32,int32>, int32>& MidCache,
	int32 A, int32 B, int32 C, int32 Depth, float Radius)
{
	if (Depth == 0)
	{
		Tris.Add(A); Tris.Add(B); Tris.Add(C);
		return;
	}

	auto GetMid = [&](int32 V0, int32 V1) -> int32
	{
		auto Key = TPair<int32,int32>(FMath::Min(V0,V1), FMath::Max(V0,V1));
		if (int32* Found = MidCache.Find(Key))
			return *Found;
		FVector Mid = ((Verts[V0] + Verts[V1]) * 0.5f).GetSafeNormal() * Radius;
		int32 Idx = Verts.Num();
		Verts.Add(Mid);
		MidCache.Add(Key, Idx);
		return Idx;
	};

	int32 AB = GetMid(A, B);
	int32 BC = GetMid(B, C);
	int32 CA = GetMid(C, A);

	SubdivideTriangle(Verts, Tris, MidCache, A, AB, CA, Depth-1, Radius);
	SubdivideTriangle(Verts, Tris, MidCache, B, BC, AB, Depth-1, Radius);
	SubdivideTriangle(Verts, Tris, MidCache, C, CA, BC, Depth-1, Radius);
	SubdivideTriangle(Verts, Tris, MidCache, AB, BC, CA, Depth-1, Radius);
}

void RockGeometry::MakeIcosphere(TArray<FVector>& OutVerts, TArray<int32>& OutTris,
                                  float Radius, int32 Subdivisions)
{
	const float T = (1.0f + FMath::Sqrt(5.0f)) * 0.5f;

	OutVerts.Empty();
	OutTris.Empty();

	// 12 vertices of an icosahedron
	auto AddV = [&](float X, float Y, float Z)
	{
		OutVerts.Add(FVector(X, Y, Z).GetSafeNormal() * Radius);
	};

	AddV(-1, T, 0); AddV(1, T, 0); AddV(-1,-T, 0); AddV(1,-T, 0);
	AddV(0,-1, T); AddV(0, 1, T); AddV(0,-1,-T); AddV(0, 1,-T);
	AddV(T, 0,-1); AddV(T, 0, 1); AddV(-T, 0,-1); AddV(-T, 0, 1);

	// 20 faces
	static const int32 Faces[60] = {
		0,11,5, 0,5,1, 0,1,7, 0,7,10, 0,10,11,
		1,5,9, 5,11,4, 11,10,2, 10,7,6, 7,1,8,
		3,9,4, 3,4,2, 3,2,6, 3,6,8, 3,8,9,
		4,9,5, 2,4,11, 6,2,10, 8,6,7, 9,8,1
	};

	TMap<TPair<int32,int32>, int32> MidCache;

	for (int32 i = 0; i < 60; i += 3)
	{
		SubdivideTriangle(OutVerts, OutTris, MidCache,
		                  Faces[i], Faces[i+1], Faces[i+2],
		                  Subdivisions, Radius);
	}
}

// ---- Noise ----

// Simple hash-based 3D noise (no external dependency)
static float Hash3D(float X, float Y, float Z)
{
	int32 IX = FMath::FloorToInt32(X);
	int32 IY = FMath::FloorToInt32(Y);
	int32 IZ = FMath::FloorToInt32(Z);
	uint32 H = static_cast<uint32>(IX * 374761393 + IY * 668265263 + IZ * 1274126177);
	H = (H ^ (H >> 13)) * 1274126177u;
	H = H ^ (H >> 16);
	return static_cast<float>(H & 0x7FFFFFFF) / static_cast<float>(0x7FFFFFFF);
}

static float ValueNoise3D(const FVector& P)
{
	float FX = FMath::FloorToFloat(P.X), FY = FMath::FloorToFloat(P.Y), FZ = FMath::FloorToFloat(P.Z);
	float DX = P.X - FX, DY = P.Y - FY, DZ = P.Z - FZ;
	// Smoothstep
	DX = DX*DX*(3.f-2.f*DX);
	DY = DY*DY*(3.f-2.f*DY);
	DZ = DZ*DZ*(3.f-2.f*DZ);

	float C000 = Hash3D(FX,   FY,   FZ);
	float C100 = Hash3D(FX+1, FY,   FZ);
	float C010 = Hash3D(FX,   FY+1, FZ);
	float C110 = Hash3D(FX+1, FY+1, FZ);
	float C001 = Hash3D(FX,   FY,   FZ+1);
	float C101 = Hash3D(FX+1, FY,   FZ+1);
	float C011 = Hash3D(FX,   FY+1, FZ+1);
	float C111 = Hash3D(FX+1, FY+1, FZ+1);

	float X0 = FMath::Lerp(C000, C100, DX);
	float X1 = FMath::Lerp(C010, C110, DX);
	float X2 = FMath::Lerp(C001, C101, DX);
	float X3 = FMath::Lerp(C011, C111, DX);
	float Y0 = FMath::Lerp(X0, X1, DY);
	float Y1 = FMath::Lerp(X2, X3, DY);
	return FMath::Lerp(Y0, Y1, DZ) * 2.f - 1.f; // [-1, 1]
}

float RockGeometry::SampleNoise(const FVector& Pos, ERockNoiseType Type, FRockRNG& RNG)
{
	if (Type == ERockNoiseType::Simplex)
	{
		float Val = 0.f, Amp = 1.f, Freq = 1.f, MaxVal = 0.f;
		for (int32 O = 0; O < 3; O++)
		{
			Val += ValueNoise3D(Pos * Freq) * Amp;
			MaxVal += Amp;
			Amp *= 0.5f;
			Freq *= 2.f;
		}
		return Val / MaxVal;
	}
	else if (Type == ERockNoiseType::Worley)
	{
		float Val = 0.f, Amp = 1.f, Freq = 1.5f;
		for (int32 O = 0; O < 2; O++)
		{
			float N = ValueNoise3D(Pos * Freq);
			Val -= FMath::Abs(N) * Amp;
			Amp *= 0.5f;
			Freq *= 2.f;
		}
		return Val;
	}
	else // Multifractal
	{
		float Val = 0.f, Amp = 1.f, Freq = 1.f, Weight = 1.f;
		for (int32 O = 0; O < 3; O++)
		{
			float N = ValueNoise3D(Pos * Freq);
			N = 1.f - FMath::Abs(N);
			N *= N;
			N *= Weight;
			Weight = FMath::Clamp(N * 2.f, 0.f, 1.f);
			Val += N * Amp;
			Amp *= 0.5f;
			Freq *= 2.f;
		}
		return Val;
	}
}

// ---- Rock creation ----

FRockMeshData RockGeometry::CreateRock(int32 Detail, float Roughness,
                                        int32 SeedOffset, ERockBaseShape Shape,
                                        ERockNoiseType NoiseType)
{
	FRockMeshData Mesh;
	FRockRNG RNG(SeedOffset);

	int32 Subdivs = FMath::Max(1, Detail - 1);
	if (Shape == ERockBaseShape::Octahedron)
		Subdivs = FMath::Max(1, FMath::Min(Detail - 2, 2));

	if (Shape == ERockBaseShape::Cylinder)
	{
		// Simple cylinder
		int32 Segs = 8 + Detail * 4;
		int32 Rings = 2 + Detail;
		float Height = 2.f;
		for (int32 R = 0; R <= Rings; R++)
		{
			float Z = -1.f + (Height * R / Rings);
			for (int32 S = 0; S < Segs; S++)
			{
				float Angle = 2.f * PI * S / Segs;
				Mesh.Vertices.Add(FVector(FMath::Cos(Angle), FMath::Sin(Angle), Z));
			}
		}
		for (int32 R = 0; R < Rings; R++)
		{
			for (int32 S = 0; S < Segs; S++)
			{
				int32 A = R * Segs + S;
				int32 B = R * Segs + (S + 1) % Segs;
				int32 C = (R + 1) * Segs + S;
				int32 D = (R + 1) * Segs + (S + 1) % Segs;
				Mesh.Triangles.Append({A, B, C, B, D, C});
			}
		}
	}
	else
	{
		MakeIcosphere(Mesh.Vertices, Mesh.Triangles, 1.f, Subdivs);
	}

	// Noise displacement
	FVector SeedOffset3D(SeedOffset * 0.01f);
	for (FVector& V : Mesh.Vertices)
	{
		FVector Dir = V.GetSafeNormal();
		float N = SampleNoise(Dir + SeedOffset3D, NoiseType, RNG);
		V *= (1.f + N * Roughness);
	}

	// Compute normals
	Mesh.Normals.SetNumZeroed(Mesh.Vertices.Num());
	for (int32 i = 0; i + 2 < Mesh.Triangles.Num(); i += 3)
	{
		int32 A = Mesh.Triangles[i], B = Mesh.Triangles[i+1], C = Mesh.Triangles[i+2];
		FVector FaceN = FVector::CrossProduct(
			Mesh.Vertices[B] - Mesh.Vertices[A],
			Mesh.Vertices[C] - Mesh.Vertices[A]);
		Mesh.Normals[A] += FaceN;
		Mesh.Normals[B] += FaceN;
		Mesh.Normals[C] += FaceN;
	}
	for (FVector& N : Mesh.Normals)
		N = N.GetSafeNormal();

	BoxProjectUVs(Mesh);
	return Mesh;
}

// ---- UV projection ----

void RockGeometry::BoxProjectUVs(FRockMeshData& Mesh)
{
	Mesh.UVs.SetNum(Mesh.Vertices.Num());
	for (int32 i = 0; i + 2 < Mesh.Triangles.Num(); i += 3)
	{
		int32 A = Mesh.Triangles[i], B = Mesh.Triangles[i+1], C = Mesh.Triangles[i+2];
		FVector FaceN = FVector::CrossProduct(
			Mesh.Vertices[B] - Mesh.Vertices[A],
			Mesh.Vertices[C] - Mesh.Vertices[A]).GetSafeNormal();

		float AX = FMath::Abs(FaceN.X), AY = FMath::Abs(FaceN.Y), AZ = FMath::Abs(FaceN.Z);

		for (int32 Idx : {A, B, C})
		{
			const FVector& Co = Mesh.Vertices[Idx];
			if (AZ >= AX && AZ >= AY)
				Mesh.UVs[Idx] = FVector2D(Co.X, Co.Y);
			else if (AX >= AY)
				Mesh.UVs[Idx] = FVector2D(Co.Y, Co.Z);
			else
				Mesh.UVs[Idx] = FVector2D(Co.X, Co.Z);
		}
	}
}

// ---- Stretch clamp ----

FVector RockGeometry::ClampStretch(const FVector& Scale, float MaxRatio)
{
	if (MaxRatio <= 0.f) return Scale;
	float Lo = FMath::Min3(Scale.X, Scale.Y, Scale.Z);
	if (Lo < SMALL_NUMBER) return Scale;
	float Cap = Lo * MaxRatio;
	return FVector(
		FMath::Min(Scale.X, Cap),
		FMath::Min(Scale.Y, Cap),
		FMath::Min(Scale.Z, Cap));
}
