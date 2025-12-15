#pragma once

#include "CoreMinimal.h"
#include "AntiRollbarSpecifications.generated.h"

//------------------------------------------------------------------------------
//                          ANTI-ROLLBAR CONFIGURATION STRUCT
//------------------------------------------------------------------------------

/** FAntiRollbar - Defines the properties for a single anti-roll bar, connecting a pair of wheels on an axle */
USTRUCT(BlueprintType)
struct FAntiRollbar
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension|AntiRollbar")
    bool bEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension|AntiRollbar")
    float Stiffness = 80000.0f;                          // [N/m] - Roll stiffness coefficient

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension|AntiRollbar")
    float Damping = 1500.0f;                             // [N·s/m] - Damping coefficient

    int32 WheelIndices[2] = {-1, -1};

    //------------------------------------------------------------------------------
    // Constructors
    //------------------------------------------------------------------------------

    FAntiRollbar() = default;
};