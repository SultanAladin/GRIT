#include "GameContext.h"

#include "EngineUtils.h"
#include "SpawnControl/DeploymentPoint.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"

AGameContext::AGameContext()
{
    SpawnStrategy = ESpawnStrategy::Default;
}

void AGameContext::BeginPlay()
{
    Super::BeginPlay();
    PurgeInvalidDeployedControllers();
    if (bAutoSpawnPlayers)
    {
        DeployAllPlayers(SpawnStrategy);
    }
}

void AGameContext::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    EnsureArrivalTimeEntry(NewPlayer);
    if (bAutoSpawnPlayers)
    {
        DeployAllPlayers(SpawnStrategy);
    }
}

void AGameContext::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
    Super::HandleStartingNewPlayer_Implementation(NewPlayer);
    // Intentionally blank: Default spawn path suppressed so deployment system controls pawn creation.
}

APawn* AGameContext::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
    if (bAutoSpawnPlayers)
    {
        return nullptr;
    }

    return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, SpawnTransform);
}

void AGameContext::EnsureArrivalTimeEntry(APlayerController* Controller)
{
    if (!Controller) return;
    if (!ArrivalTimeMap.Contains(Controller))
    {
        ArrivalTimeMap.Add(Controller, FPlatformTime::Seconds());
    }
}

void AGameContext::DeployAllPlayers(ESpawnStrategy Strategy)
{
    if (!bAutoSpawnPlayers)
    {
        return;
    }

    PurgeInvalidDeployedControllers();

    switch (Strategy)
    {
    case ESpawnStrategy::Default:
        DeployPlayers_Default();
        break;
    case ESpawnStrategy::Random:
        DeployPlayers_Random();
        break;
    case ESpawnStrategy::ArrivalTime:
        DeployPlayers_ArrivalTime();
        break;
    default:
        DeployPlayers_Default();
        break;
    }
}

void AGameContext::DeployPlayers_Default()
{
    ADeploymentPoint* FirstPoint = GetFirstDeploymentPoint();
    if (!FirstPoint) return;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* Controller = It->Get();
        if (!Controller) continue;
        EnsureArrivalTimeEntry(Controller);
        if (HasControllerBeenDeployed(Controller))
        {
            continue;
        }
        if (APawn* SpawnedPawn = SpawnPawnForControllerAtPoint(Controller, FirstPoint))
        {
            MarkControllerDeployed(Controller);
        }
    }
}

void AGameContext::DeployPlayers_Random()
{
    TArray<ADeploymentPoint*> Points = GetAllDeploymentPoints();
    if (Points.Num() == 0) return;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* Controller = It->Get();
        if (!Controller) continue;
        EnsureArrivalTimeEntry(Controller);
        if (HasControllerBeenDeployed(Controller))
        {
            continue;
        }
        const int32 Index = UKismetMathLibrary::RandomInteger(Points.Num());
        if (APawn* SpawnedPawn = SpawnPawnForControllerAtPoint(Controller, Points[Index]))
        {
            MarkControllerDeployed(Controller);
        }
    }
}

void AGameContext::DeployPlayers_ArrivalTime()
{
    TArray<ADeploymentPoint*> Points = GetAllDeploymentPoints();
    if (Points.Num() == 0) return;

    int32 PointIndex = 0;
    TArray<TPair<APlayerController*, double>> SortedEntries;
    SortedEntries.Reserve(ArrivalTimeMap.Num());
    for (const TPair<APlayerController*, double>& Entry : ArrivalTimeMap)
    {
        SortedEntries.Add(Entry);
    }
    SortedEntries.Sort([](const TPair<APlayerController*, double>& A, const TPair<APlayerController*, double>& B)
    {
        return A.Value < B.Value;
    });

    for (const TPair<APlayerController*, double>& Entry : SortedEntries)
    {
        if (!Entry.Key) continue;
        if (HasControllerBeenDeployed(Entry.Key))
        {
            continue;
        }
        ADeploymentPoint* Point = Points.IsValidIndex(PointIndex) ? Points[PointIndex] : Points.Last();
        if (APawn* SpawnedPawn = SpawnPawnForControllerAtPoint(Entry.Key, Point))
        {
            MarkControllerDeployed(Entry.Key);
            ++PointIndex;
        }
    }
}

TArray<ADeploymentPoint*> AGameContext::GetAllDeploymentPoints() const
{
    TArray<ADeploymentPoint*> Result;
    for (TActorIterator<ADeploymentPoint> It(GetWorld()); It; ++It)
    {
        Result.Add(*It);
    }
    return Result;
}

ADeploymentPoint* AGameContext::GetFirstDeploymentPoint() const
{
    for (TActorIterator<ADeploymentPoint> It(GetWorld()); It; ++It)
    {
        return *It;
    }
    return nullptr;
}

APawn* AGameContext::SpawnPawnForControllerAtPoint(AController* Controller, ADeploymentPoint* Point)
{
    if (!Controller || !Point) return nullptr;

    UClass* PawnClass = GetDefaultPawnClassForController(Controller);
    if (!PawnClass) return nullptr;

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Controller;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    FVector Location = Point->GetActorLocation();
    FRotator Rotation = Point->GetActorRotation();

    APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, Location, Rotation, SpawnParams);
    if (SpawnedPawn && Controller->IsA<APlayerController>())
    {
        APlayerController* PC = Cast<APlayerController>(Controller);
        if (PC)
        {
            PC->Possess(SpawnedPawn);
        }
    }

    if (SpawnedPawn && Controller->IsA<APlayerController>())
    {
        MarkControllerDeployed(Cast<APlayerController>(Controller));
    }

    return SpawnedPawn;
}

bool AGameContext::HasControllerBeenDeployed(APlayerController* Controller) const
{
    return Controller && DeployedControllers.Contains(Controller);
}

void AGameContext::MarkControllerDeployed(APlayerController* Controller)
{
    if (Controller)
    {
        DeployedControllers.Add(Controller);
    }
}

void AGameContext::PurgeInvalidDeployedControllers()
{
    for (auto It = DeployedControllers.CreateIterator(); It; ++It)
    {
        if (!It->IsValid())
        {
            It.RemoveCurrent();
        }
    }
}
