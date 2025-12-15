#pragma once

#include "CoreMinimal.h"
#include "VehicleAssist.generated.h"

//------------------------------------------------------------------------------
// 🧩 Trajectory Atlas
//------------------------------------------------------------------------------

/** @brief FTrajectoryAtlas - Stores trajectory ray data */
USTRUCT()
struct FTrajectoryAtlas
{
    GENERATED_BODY()

    int32 NumRays = 0;                 // [-] - Number of trajectory rays
    TArray<float> Fractions;           // [-] - Fractional distances per ray
    TArray<FVector> Directions;        // [unitless] - Ray direction vectors
};

