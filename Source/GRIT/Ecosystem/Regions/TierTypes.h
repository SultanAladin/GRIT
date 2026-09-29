#pragma once

#include "CoreMinimal.h"
#include "Math/Vector2D.h"
#include "Templates/SubclassOf.h"
#include "TierTypes.generated.h"

UENUM(BlueprintType)
enum class ETierLevel : uint8
{
    Region,
    Tier1,
    Tier2,
    Tier3
};

UENUM(BlueprintType)
enum class ETierDensity : uint8
{
    Sparse,    // A
    Standard,  // B
    Dense      // C
};

USTRUCT(BlueprintType)
struct GRIT_API FTierCoord
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 X = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Y = 0;

    bool operator==(const FTierCoord& O) const { return X == O.X && Y == O.Y; }
    bool operator!=(const FTierCoord& O) const { return !(*this == O); }
};

FORCEINLINE uint32 GetTypeHash(const FTierCoord& C)
{
    return HashCombine(::GetTypeHash(C.X), ::GetTypeHash(C.Y));
}

USTRUCT(BlueprintType)
struct GRIT_API FTierTenantSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<AActor> Class;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FTransform LocalXform;
};

namespace TierLayout
{
    constexpr float Tier1CellX_Cm = 20000.f;
    constexpr float Tier1CellY_Cm = 100000.f;
    constexpr int32 Tier1NX = 5;
    constexpr int32 Tier1NY = 1;

    constexpr float Tier2CellX_Cm = 5000.f;
    constexpr float Tier2CellY_Cm = 25000.f;
    constexpr int32 Tier2NX = 4;
    constexpr int32 Tier2NY = 4;

    constexpr float Tier3CellX_Cm = 1250.f;
    constexpr float Tier3CellY_Cm = 6250.f;
    constexpr int32 Tier3NX = 4;
    constexpr int32 Tier3NY = 4;

    constexpr float RegionExtentX_Cm = Tier1CellX_Cm * Tier1NX; // 100000 cm = 1km
    constexpr float RegionExtentY_Cm = Tier1CellY_Cm * Tier1NY; // 100000 cm = 1km
    constexpr float ClusterHeight_Cm = 50000.f;                 // tall Z for overlap volume
}
