#include "TierTenantBinder.h"
#include "TierWorldSubsystem.h"
#include "TierRegion.h"
#include "TierClusterBase.h"
#include "Tier3Base.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

UTierTenantBinder::UTierTenantBinder()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTierTenantBinder::BeginPlay()
{
    Super::BeginPlay();
    if (!bAutoBind) return;

    AActor* Owner = GetOwner();
    UWorld* World = GetWorld();
    if (!Owner || !World) return;

    UTierWorldSubsystem* Sub = World->GetSubsystem<UTierWorldSubsystem>();
    if (!Sub) return;

    const FVector WorldPos = Owner->GetActorLocation();
    ATierRegion* RegionPtr = Sub->FindRegionContaining(WorldPos);
    if (!RegionPtr)
    {
        UE_LOG(LogTemp, Warning, TEXT("TierTenantBinder: no ATierRegion contains %s at %s"), *Owner->GetName(), *WorldPos.ToCompactString());
        return;
    }

    // Iterate live cluster actors in this region; pick the deepest tier whose bounds contain the owner's XY.
    ATierClusterBase* Deepest = nullptr;
    for (TActorIterator<ATierClusterBase> It(World); It; ++It)
    {
        ATierClusterBase* C = *It;
        if (!C || C->Region.Get() != RegionPtr) continue;
        if (!C->Bounds || !C->Bounds->Bounds.GetBox().IsInsideXY(WorldPos)) continue;
        if (!Deepest || (uint8)C->Level > (uint8)Deepest->Level)
        {
            Deepest = C;
        }
    }

    if (Deepest && Deepest->Level == ETierLevel::Tier3)
    {
        // Leaf is alive — owner stays where it is, no respawn needed.
        return;
    }

    // Leaf is not alive. Record a deferred spawn and destroy the owner.
    FTierTenantSpec Spec;
    Spec.Class = Owner->GetClass();
    Spec.LocalXform = Owner->GetActorTransform();
    RegionPtr->RecordDeferredTenant(Spec);
    Owner->Destroy();
}
