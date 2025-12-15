#pragma once

//------------------------------------------------------------------------------
//                               deployment point actor
//------------------------------------------------------------------------------
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "DeploymentPoint.generated.h"

//------------------------------------------------------------------------------
//                                    log channel
//------------------------------------------------------------------------------
DECLARE_LOG_CATEGORY_EXTERN(LogDeploymentPoint, Log, All);

//------------------------------------------------------------------------------
//                                    actor class
//------------------------------------------------------------------------------
UCLASS()
class GRIT_API ADeploymentPoint : public AActor
{
    GENERATED_BODY()

public:
    /** Default constructor */
    ADeploymentPoint();

    /** Default destructor */
    virtual ~ADeploymentPoint();

protected:
    virtual void BeginPlay() override;
    virtual void PostLoad() override;
    virtual void BeginDestroy() override;
    virtual void PostActorCreated() override;
    virtual void OnConstruction(const FTransform& Transform) override;

    //------------------------------------------------------------------------------
    //                              visualization components
    //------------------------------------------------------------------------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visuals")
    class USceneComponent* SceneComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visuals")
    class UArrowComponent* ArrowComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visuals")
    class UBillboardComponent* BillboardComponent;

    //------------------------------------------------------------------------------
    //                                 deployment metadata
    //------------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deployment")
    float Offset = 150.0f; // [cm] - Billboard vertical offset

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deployment")
    int32 DeploymentIndex; // [-] - Sequential index within current world

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SelfDestruction")
    bool bCanSelfDestruct = false; // [-] - Optional auto-destroy toggle

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deployment")
    FName DeploymentTag; // [-] - Deployment grouping tag

private:
    static int32 GlobalDeploymentIndex;           // [-] - Accumulated index counter per world
    static TWeakObjectPtr<UWorld> CachedWorld;     // [-] - Tracks current world owning the counter

    bool bHasAssignedDeploymentIndex = false;     // [-] - Indicates if this actor owns an index
};
