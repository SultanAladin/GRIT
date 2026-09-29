#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TierTypes.h"
#include "TierRegion.generated.h"

class UBoxComponent;
class ATierClusterBase;

/**
 * 1km × 1km root. Designer-placed. Owns a 5×1 grid of Tier1 strips.
 * Holds RegionOrigin (cached at BeginPlay) and the deferred-tenant-spec map.
 * bReplicates = false; each client drives its own activation.
 */
UCLASS()
class GRIT_API ATierRegion : public AActor
{
    GENERATED_BODY()

public:
    ATierRegion();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tier")
    TObjectPtr<UBoxComponent> Bounds;

    /** Per-strip density choice. Index 0..4 maps to the 5 Tier1 strips along X. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier")
    TArray<ETierDensity> Tier1Density;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier|Debug")
    bool bDrawDebug = false;

    FVector GetRegionOrigin() const { return RegionOrigin; }

    bool ContainsWorldPos(const FVector& WorldPos) const;

    /** Append a deferred tenant spec; will spawn into the matching leaf when it activates. */
    void RecordDeferredTenant(const FTierTenantSpec& Spec);

    /** Called by leaf cells in BeginPlay to drain any deferred specs whose XY lies inside their bounds. */
    void ConsumeDeferredTenantsInBox(const FBox2D& BoxXY, TArray<FTierTenantSpec>& Out);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

private:
    FVector RegionOrigin = FVector::ZeroVector;
    FTierCoord CurrentChildCoord{ INT32_MIN, INT32_MIN };

    UPROPERTY()
    TArray<TObjectPtr<ATierClusterBase>> Tier1Children;

    TArray<FTierTenantSpec> DeferredTenants;

    UFUNCTION()
    void OnPlayerEnter(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                       int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    void EvaluateForPlayer(const FVector& PlayerWorld);
    void DestroyAllChildren();
    ATierClusterBase* SpawnTier1At(const FTierCoord& C);
    TSubclassOf<ATierClusterBase> Tier1ClassForDensity(ETierDensity InDensity) const;
};
