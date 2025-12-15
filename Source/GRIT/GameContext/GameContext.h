#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameContext.generated.h"

class ADeploymentPoint;

UENUM(BlueprintType)
enum class ESpawnStrategy : uint8
{
    Default,
    Random,
    ArrivalTime
};

UCLASS()
class GRIT_API AGameContext : public AGameModeBase
{
    GENERATED_BODY()

public:
    AGameContext();

    UFUNCTION()
    void DeployAllPlayers(ESpawnStrategy Strategy);

    UFUNCTION()
    void DeployPlayers_Default();

    UFUNCTION()
    void DeployPlayers_Random();

    UFUNCTION()
    void DeployPlayers_ArrivalTime();

protected:
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void BeginPlay() override;
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override; // Purpose: suppress default auto-spawn path (intentionally empty)
    virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;

private:
    TArray<class ADeploymentPoint*> GetAllDeploymentPoints() const;
    ADeploymentPoint* GetFirstDeploymentPoint() const;

    APawn* SpawnPawnForControllerAtPoint(AController* Controller, ADeploymentPoint* Point);

    void EnsureArrivalTimeEntry(APlayerController* Controller);
    bool HasControllerBeenDeployed(APlayerController* Controller) const;
    void MarkControllerDeployed(APlayerController* Controller);
    void PurgeInvalidDeployedControllers();

private:
    UPROPERTY(EditAnywhere, Category = "Spawning")
    ESpawnStrategy SpawnStrategy;

    TMap<APlayerController*, double> ArrivalTimeMap;

    UPROPERTY(EditAnywhere, Category = "Spawning")
    bool bAutoSpawnPlayers = true;

    TSet<TWeakObjectPtr<APlayerController>> DeployedControllers;
};
