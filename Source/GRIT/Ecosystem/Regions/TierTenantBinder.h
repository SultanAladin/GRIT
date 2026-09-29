#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TierTenantBinder.generated.h"

/**
 * Drop on any actor that should live inside the tier system.
 *
 * On BeginPlay: walks the tier hierarchy from the owning region down to the leaf containing
 * the owner's world position. If the leaf is currently alive, the binder is a no-op (the
 * level-placed actor stays where it is). If the leaf is NOT alive, the binder records a
 * deferred tenant spec on the region and destroys the owner — the leaf will respawn an
 * equivalent actor when it activates. This keeps the destroy+respawn invariant uniform.
 */
UCLASS(ClassGroup = (Regions), meta = (BlueprintSpawnableComponent, DisplayName = "Tier Tenant Binder"))
class GRIT_API UTierTenantBinder : public UActorComponent
{
    GENERATED_BODY()

public:
    UTierTenantBinder();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tier")
    bool bAutoBind = true;

protected:
    virtual void BeginPlay() override;
};
