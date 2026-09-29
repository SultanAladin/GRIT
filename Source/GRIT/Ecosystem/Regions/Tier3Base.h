#pragma once

#include "CoreMinimal.h"
#include "TierClusterBase.h"
#include "Tier3Base.generated.h"

/**
 * Tier3 leaf base. Holds tenants — no children. Spawns from TenantClasses + deferred specs
 * (recorded by UTierTenantBinder for level-placed actors that landed in inactive cells).
 */
UCLASS(Abstract)
class GRIT_API ATier3Base : public ATierClusterBase
{
    GENERATED_BODY()

public:
    /** Tenant prefabs to spawn whenever this leaf activates. Designer-authored. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier|Tenants")
    TArray<FTierTenantSpec> TenantClasses;

protected:
    virtual void BeginPlay() override;

    // No children.
    virtual int32 GetChildNX() const override { return 0; }
    virtual int32 GetChildNY() const override { return 0; }
    virtual FVector2D GetChildCellSizeCm() const override { return FVector2D::ZeroVector; }
    virtual TSubclassOf<ATierClusterBase> ChildClassForDensity(ETierDensity) const override { return nullptr; }

    UPROPERTY()
    TArray<TObjectPtr<AActor>> SpawnedTenants;

    void SpawnTenantSpec(const FTierTenantSpec& Spec);
};
