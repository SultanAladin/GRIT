#include "Tier3Base.h"
#include "TierRegion.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

void ATier3Base::BeginPlay()
{
    Super::BeginPlay();

    // Spawn designer-authored tenants.
    for (const FTierTenantSpec& Spec : TenantClasses)
    {
        SpawnTenantSpec(Spec);
    }

    // Drain any deferred tenants whose XY lies inside our footprint.
    if (ATierRegion* R = Region.Get())
    {
        if (Bounds)
        {
            const FVector C = Bounds->GetComponentLocation();
            const FVector E = Bounds->GetScaledBoxExtent();
            const FBox2D BoxXY(FVector2D(C.X - E.X, C.Y - E.Y), FVector2D(C.X + E.X, C.Y + E.Y));
            TArray<FTierTenantSpec> Drained;
            R->ConsumeDeferredTenantsInBox(BoxXY, Drained);
            for (const FTierTenantSpec& Spec : Drained)
            {
                SpawnTenantSpec(Spec);
            }
        }
    }
}

void ATier3Base::SpawnTenantSpec(const FTierTenantSpec& Spec)
{
    if (!Spec.Class) return;
    UWorld* World = GetWorld();
    if (!World) return;

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* Spawned = World->SpawnActor<AActor>(Spec.Class.Get(), Spec.LocalXform, Params);
    if (!Spawned) return;

    // Attach so destruction of this leaf cascades to the tenant.
    Spawned->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
    SpawnedTenants.Add(Spawned);
}
