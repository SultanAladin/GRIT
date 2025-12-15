#include "TeamSpawnRelay.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Logging/LogMacros.h"

//------------------------------------------------------------------------------
//                                      logging
//------------------------------------------------------------------------------
DEFINE_LOG_CATEGORY(LogTeamSpawnRelay);

ATeamSpawnRelay::ATeamSpawnRelay()
{
    UE_LOG(LogTeamSpawnRelay, Log, TEXT("📍 Team relay scaffold initialized | Actor: %s"), *GetName());
}

void ATeamSpawnRelay::BeginPlay()
{
    Super::BeginPlay();

    LogTeamSnapshot(TEXT("BeginPlay"));
}

void ATeamSpawnRelay::StageTeamPoints(int32 RelayCount, float RadialSpacing)
{
    if (RelayCount <= 0) return;

    const FVector BaseLocation = GetActorLocation();
    const FRotator BaseRotation = GetActorRotation();

    LinkedPoints.Reset();

    for (int32 Index = 0; Index < RelayCount; ++Index)
    {
        const float AngleDeg = (360.0f / RelayCount) * Index;
        const FVector Direction = BaseRotation.RotateVector(FVector(FMath::Cos(FMath::DegreesToRadians(AngleDeg)), FMath::Sin(FMath::DegreesToRadians(AngleDeg)), 0.0f));
        const FVector SpawnLocation = BaseLocation + Direction * RadialSpacing;

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        if (UWorld* World = GetWorld())
        {
            ADeploymentPoint* SpawnedPoint = World->SpawnActor<ADeploymentPoint>(ADeploymentPoint::StaticClass(), SpawnLocation, BaseRotation, SpawnParams);
            if (SpawnedPoint)
            {
                LinkedPoints.Add(SpawnedPoint);
                UE_LOG(LogTeamSpawnRelay, Log, TEXT("🏁 Linked deployment point | TeamID: %d | Index: %d | Actor: %s"), TeamID, Index, *SpawnedPoint->GetName());
            }
        }
    }

    LogTeamSnapshot(TEXT("StageTeamPoints"));
}

void ATeamSpawnRelay::LogTeamSnapshot(const FString& ContextLabel) const
{
    const FString LocationString = GetActorLocation().ToCompactString();
    const FString ColourString = TeamColour.ToString();
    UE_LOG(LogTeamSpawnRelay, Log, TEXT("🏳️ Team snapshot | Context: %s | TeamID: %d | TeamName: %s | Colour: %s | Linked: %d | Location: %s"), *ContextLabel, TeamID, *TeamName.ToString(), *ColourString, LinkedPoints.Num(), *LocationString);
}
