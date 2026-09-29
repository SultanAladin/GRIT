#include "TierWorldSubsystem.h"
#include "TierRegion.h"

void UTierWorldSubsystem::Register(ATierRegion* Region)
{
    if (!Region) return;
    Regions.AddUnique(Region);
}

void UTierWorldSubsystem::Unregister(ATierRegion* Region)
{
    if (!Region) return;
    Regions.RemoveAllSwap([Region](const TWeakObjectPtr<ATierRegion>& W) { return !W.IsValid() || W.Get() == Region; });
}

ATierRegion* UTierWorldSubsystem::FindRegionContaining(const FVector& WorldPos) const
{
    for (const TWeakObjectPtr<ATierRegion>& W : Regions)
    {
        ATierRegion* R = W.Get();
        if (R && R->ContainsWorldPos(WorldPos))
        {
            return R;
        }
    }
    return nullptr;
}
