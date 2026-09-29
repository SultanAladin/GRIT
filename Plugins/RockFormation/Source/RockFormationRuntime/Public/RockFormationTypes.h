#pragma once

#include "CoreMinimal.h"
#include "RockFormationTypes.generated.h"

UENUM(BlueprintType)
enum class ERockBaseShape : uint8
{
	Icosahedron,
	Octahedron,
	Cylinder
};

UENUM(BlueprintType)
enum class ERockNoiseType : uint8
{
	Simplex,
	Worley,
	Multifractal
};
