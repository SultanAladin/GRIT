// TireConstruct.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TireSpecifications.h"
#include "TireConstruct.generated.h"

UCLASS()
class GRIT_API ATireConstruct : public AActor
{
    GENERATED_BODY()

public:
    ATireConstruct();

    /** Static tire geometry mesh */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* TireGeometry;

    /** Skeletal tire mesh (deformable) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USkeletalMeshComponent* SkeletalMesh;

    /** Tire specification data - replicated on creation */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Tire")
    FTireSpecSheet SpecSheet;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    virtual void Tick(float DeltaTime) override;
};  