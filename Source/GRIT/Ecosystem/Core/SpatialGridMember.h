#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpatialGridSubsystem.h"
#include "SpatialGridMember.generated.h"

/**
 * Drop on any actor that should be managed by the SpatialGridSubsystem's collision LOD.
 * The component registers its owner into the appropriate tier bucket on BeginPlay
 * and unregisters it on EndPlay. No interface, no tags — just a component with an enum.
 */
UCLASS(ClassGroup=(Spatial), meta=(BlueprintSpawnableComponent, DisplayName="Spatial Grid Member"))
class GRIT_API USpatialGridMember : public UActorComponent
{
    GENERATED_BODY()

public:
    USpatialGridMember();

    /** Importance tier. Controls how aggressively the grid toggles this actor's collision. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spatial")
    ESpatialTier Tier = ESpatialTier::Tier1;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
