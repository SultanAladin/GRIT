#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TierTypes.h"
#include "TierClusterBase.generated.h"

class UBoxComponent;
class ATierRegion;
class ATierClusterBase;

/**
 * Base class for Tier1/Tier2/Tier3 cluster actors.
 *
 * All work is game-thread. bReplicates = false; each client drives its own activation.
 * Activation diff is idempotent — both Tick poll and OnComponentBeginOverlap funnel
 * into EvaluateForPlayer; equal Centers ⇒ no-op.
 */
UCLASS(Abstract)
class GRIT_API ATierClusterBase : public AActor
{
    GENERATED_BODY()

public:
    ATierClusterBase();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tier")
    TObjectPtr<UBoxComponent> Bounds;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tier")
    ETierLevel Level = ETierLevel::Tier1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tier")
    ETierDensity Density = ETierDensity::Standard;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tier")
    FTierCoord Coord;

    UPROPERTY()
    TWeakObjectPtr<ATierRegion> Region;

    UPROPERTY()
    TWeakObjectPtr<ATierClusterBase> ParentCluster;

    UPROPERTY()
    TArray<TObjectPtr<ATierClusterBase>> ChildClusters;

    /** Configure local position + footprint of the bounds box. SizeCm is XY extent in centimeters. */
    void ConfigureBounds(const FVector& LocalCenter, const FVector2D& SizeCm);

    /** Cascading 3×3 active-set diff. Idempotent when Center hasn't changed. */
    virtual void EvaluateForPlayer(const FVector& PlayerWorld);

    /** Floor-and-clamp world pos to a child cell index local to this cluster. */
    FTierCoord WorldToChildCoord(const FVector& WorldPos) const;

    /** Return the 3×3 ring around Center clamped to [0,NX) × [0,NY). */
    static void Compute3x3Ring(const FTierCoord& Center, int32 NX, int32 NY, TArray<FTierCoord, TInlineAllocator<9>>& Out);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

    /** Dimensions of *this* cluster's child grid. Override per concrete tier class. */
    virtual int32 GetChildNX() const { return 0; }
    virtual int32 GetChildNY() const { return 0; }
    virtual FVector2D GetChildCellSizeCm() const { return FVector2D::ZeroVector; }

    /** Concrete child class for the given density. Override per tier; return null for leaf tiers. */
    virtual TSubclassOf<ATierClusterBase> ChildClassForDensity(ETierDensity InDensity) const { return nullptr; }

    /** Hook for leaf cells to instantiate tenants on activation. Default: no-op. */
    virtual void SpawnTenants() {}

    UFUNCTION()
    void OnPlayerEnter(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                       int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    void DestroyAllChildren();

private:
    FTierCoord CurrentChildCoord{ INT32_MIN, INT32_MIN };

    ATierClusterBase* SpawnChildAt(const FTierCoord& C);
};
