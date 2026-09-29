#include "SpatialGridMember.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

USpatialGridMember::USpatialGridMember()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USpatialGridMember::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();
    UWorld* World = GetWorld();
    if (!Owner || !World)
    {
        return;
    }

    if (USpatialGridSubsystem* Grid = World->GetSubsystem<USpatialGridSubsystem>())
    {
        Grid->RegisterTieredActor(Owner, Tier);
    }
}

void USpatialGridMember::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    AActor* Owner = GetOwner();
    UWorld* World = GetWorld();
    if (Owner && World)
    {
        if (USpatialGridSubsystem* Grid = World->GetSubsystem<USpatialGridSubsystem>())
        {
            Grid->UnregisterTieredActor(Owner, Tier);
        }
    }

    Super::EndPlay(EndPlayReason);
}
