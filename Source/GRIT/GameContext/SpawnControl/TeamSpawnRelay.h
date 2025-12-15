#pragma once

//------------------------------------------------------------------------------
//                                team spawn relay
//------------------------------------------------------------------------------
#include "DeploymentPoint.h"
#include "TeamSpawnRelay.generated.h"

//------------------------------------------------------------------------------
//                                   log channel
//------------------------------------------------------------------------------
DECLARE_LOG_CATEGORY_EXTERN(LogTeamSpawnRelay, Log, All);

//------------------------------------------------------------------------------
//                               relay configuration
//------------------------------------------------------------------------------
UCLASS()
class GRIT_API ATeamSpawnRelay : public ADeploymentPoint
{
    GENERATED_BODY()

public:
    /** Default constructor */
    ATeamSpawnRelay();

    //------------------------------------------------------------------------------
    //                              relay operations
    //------------------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Team")
    void StageTeamPoints(int32 RelayCount, float RadialSpacing);

protected:
    virtual void BeginPlay() override;

private:
    //------------------------------------------------------------------------------
    //                                   team data
    //------------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team", meta = (AllowPrivateAccess = "true"))
    int32 TeamID = 0; // [-] - Team identifier

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team", meta = (AllowPrivateAccess = "true"))
    FName TeamName = NAME_None; // [-] - Team label

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team", meta = (AllowPrivateAccess = "true"))
    FLinearColor TeamColour = FLinearColor::White; // [-] - HUD colour indicator

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Team", meta = (AllowPrivateAccess = "true"))
    TArray<ADeploymentPoint*> LinkedPoints; // [-] - Managed deployment anchors

private:
    void LogTeamSnapshot(const FString& ContextLabel) const;
};
