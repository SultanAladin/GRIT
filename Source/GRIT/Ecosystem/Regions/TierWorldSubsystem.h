#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TierWorldSubsystem.generated.h"

class ATierRegion;

UCLASS()
class GRIT_API UTierWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void Register(ATierRegion* Region);
    void Unregister(ATierRegion* Region);

    /** Linear scan; region count is small (a handful per level). */
    ATierRegion* FindRegionContaining(const FVector& WorldPos) const;

private:
    UPROPERTY()
    TArray<TWeakObjectPtr<ATierRegion>> Regions;
};
