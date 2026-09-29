// TireConstruct.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TireSpecifications.h"
#include "TireConstruct.generated.h"

class AVehicleSolver;
class UStaticMeshComponent;

/** Convention: TireGeometry mesh has a socket named "InnerRadius" placed on the rim seat
 *  (anywhere on the inner-circle of the tire) so its X/Z-plane distance from origin gives
 *  the inner radius. Wheel rotates around local Y. Width = bounds.Y * 2. Outer diameter is
 *  taken from the larger of bounds.X / bounds.Z. */
static const FName TireConstruct_InnerRadiusSocket(TEXT("InnerRadius"));

UCLASS()
class GRIT_API ATireConstruct : public AActor
{
    GENERATED_BODY()

public:
    ATireConstruct();

    /** Static tire geometry mesh (root). Width along local Y, diameter in X/Z. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* TireGeometry;

    /** Optional rim/mags mesh, attached to TireGeometry. Hidden if no mesh assigned. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MagsMesh;

    /** Tire specification data - replicated on creation */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Tire")
    FTireSpecSheet SpecSheet;

    /** Derive Width / OuterRadius / InnerRadius from TireGeometry bounds + InnerRadius socket.
     *  Outputs are in metres (matches SpecSheet conventions). */
    UFUNCTION(BlueprintCallable, Category = "Tire")
    void DeriveGeometryFromMesh(float& OutOuterRadius_m, float& OutInnerRadius_m, float& OutWidth_m) const;

    /** Sync TireGeometry, MagsMesh, BaseRubberMaterial assignments and derived radii into SpecSheet. */
    UFUNCTION(BlueprintCallable, Category = "Tire")
    void RefreshSpecFromMesh();

    /** Convenience: equip this construct onto a vehicle solver wheel slot. Server-authoritative. */
    UFUNCTION(BlueprintCallable, Category = "Tire")
    bool TryEquipTo(AVehicleSolver* Solver, int32 WheelSlot);

protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
