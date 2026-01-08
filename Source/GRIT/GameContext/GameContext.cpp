#include "GameContext.h"

#include "EngineUtils.h"
#include "GameContext/GameTracker.h"
#include "GameContext/PlayerTracker.h"
#include "SpawnControl/DeploymentPoint.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"

AGameContext::AGameContext()
{
    SpawnStrategy = ESpawnStrategy::Default;
    PlayerStateClass = APlayerTracker::StaticClass();
    GameStateClass = AGameTracker::StaticClass();
}

void AGameContext::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("🎮 GameContext::BeginPlay | Role: %s | bAutoSpawn: %d"), 
        GetLocalRole() == ROLE_Authority ? TEXT("SERVER") : TEXT("CLIENT"), bAutoSpawnPlayers);
    
    PurgeInvalidDeployedControllers();
    if (bAutoSpawnPlayers)
    {
        DeployAllPlayers(SpawnStrategy);
    }
}

void AGameContext::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    UE_LOG(LogTemp, Warning, TEXT("🎮 GameContext::PostLogin | Player: %s | Role: %s"), 
        NewPlayer ? *NewPlayer->GetName() : TEXT("NULL"),
        GetLocalRole() == ROLE_Authority ? TEXT("SERVER") : TEXT("CLIENT"));
    
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
    UE_LOG(LogTemp, Warning, TEXT("🚀 DeployAllPlayers called | Strategy: %d | bAutoSpawn: %d | Role: %s"), 
        (int32)Strategy, bAutoSpawnPlayers, 
        GetLocalRole() == ROLE_Authority ? TEXT("SERVER") : TEXT("CLIENT"));
    
    if (!bAutoSpawnPlayers) return; // Reason: manual spawn mode enabled
    if (GetLocalRole() != ROLE_Authority) return; // Reason: server authoritative spawning

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
    TArray<ADeploymentPoint*> Points = GetAllDeploymentPoints();
    UE_LOG(LogTemp, Warning, TEXT("🏁 DeployPlayers_Default | DeploymentPoints: %d"), Points.Num());
    
    if (Points.Num() == 0) return; // Reason: require at least one spawn point

    int32 PlayerCount = 0;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        PlayerCount++;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("👥 Total players found: %d"), PlayerCount);
    
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* Controller = It->Get();
        if (!Controller) continue; // Reason: skip null controllers
        
        UE_LOG(LogTemp, Warning, TEXT("👤 Processing player: %s | HasPawn: %d | IsDeployed: %d"), 
            *Controller->GetName(), Controller->GetPawn() != nullptr, HasControllerBeenDeployed(Controller));
        
        EnsureArrivalTimeEntry(Controller);
        if (HasControllerBeenDeployed(Controller)) continue; // Reason: already spawned
        
        const int32 PointIndex = DeployedControllers.Num(); // [-] - Current deployed count = next spawn point
        ADeploymentPoint* Point = Points[PointIndex % Points.Num()]; // [-] - Wrap-around indexing
        
        UE_LOG(LogTemp, Warning, TEXT("🎯 Spawning at point %d: %s | Location: %s"), 
            PointIndex, *Point->GetName(), *Point->GetActorLocation().ToString());
        
        if (APawn* SpawnedPawn = SpawnPawnForControllerAtPoint(Controller, Point))
        {
            UE_LOG(LogTemp, Warning, TEXT("✅ Successfully spawned pawn: %s at %s"), 
                *SpawnedPawn->GetName(), *SpawnedPawn->GetActorLocation().ToString());
            MarkControllerDeployed(Controller);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ Failed to spawn pawn for controller: %s"), *Controller->GetName());
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
    if (!Controller || !Point) // Reason: validate input parameters
    {
        UE_LOG(LogTemp, Error, TEXT("❌ SpawnPawnForControllerAtPoint - NULL parameter | Controller: %d | Point: %d"), 
            Controller != nullptr, Point != nullptr);
        return nullptr;
    } // End if (parameter validation)

    UClass* PawnClass = GetDefaultPawnClassForController(Controller);
    if (!PawnClass) // Reason: require valid pawn class
    {
        UE_LOG(LogTemp, Error, TEXT("❌ SpawnPawnForControllerAtPoint - No PawnClass for controller: %s"), *Controller->GetName());
        return nullptr;
    } // End if (pawn class check)
    
    UE_LOG(LogTemp, Warning, TEXT("🏗️ SpawnPawnForControllerAtPoint | Controller: %s | PawnClass: %s | Location: %s"), 
        *Controller->GetName(), *PawnClass->GetName(), *Point->GetActorLocation().ToString());

    //------------------------------------------------------------------------------
    // CRITICAL FIX: Set owner in spawn params for proper RPC routing
    //------------------------------------------------------------------------------
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Controller;  // [-] - Establishes RPC routing path
    SpawnParams.Instigator = Cast<APawn>(Controller->GetPawn());
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    FVector Location = Point->GetActorLocation();
    FRotator Rotation = Point->GetActorRotation();

    APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, Location, Rotation, SpawnParams);
    
    if (!SpawnedPawn) // Reason: spawn can fail due to collision or class issues
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to spawn pawn actor at location: %s"), *Location.ToString());
        return nullptr;
    } // End if (spawn failed)
    
    //------------------------------------------------------------------------------
    // CRITICAL FIX: Explicitly set owner BEFORE possession
    //------------------------------------------------------------------------------
    SpawnedPawn->SetOwner(Controller);
    
    UE_LOG(LogTemp, Warning, TEXT("✅ Pawn spawned: %s | Owner: %s | Location: %s"), 
        *SpawnedPawn->GetName(), 
        SpawnedPawn->GetOwner() ? *SpawnedPawn->GetOwner()->GetName() : TEXT("NONE"),
        *SpawnedPawn->GetActorLocation().ToString());
    
    //------------------------------------------------------------------------------
    // Possess pawn (also sets owner but we do it explicitly above for safety)
    //------------------------------------------------------------------------------
    if (APlayerController* PC = Cast<APlayerController>(Controller)) // Reason: only PlayerControllers can possess
    {
        PC->Possess(SpawnedPawn);
        
        UE_LOG(LogTemp, Warning, TEXT("✅ Controller possessed pawn | PC: %s | Pawn: %s | Owner: %s | NetConnection: %s"), 
            *PC->GetName(), 
            *SpawnedPawn->GetName(),
            SpawnedPawn->GetOwner() ? *SpawnedPawn->GetOwner()->GetName() : TEXT("NONE"),
            SpawnedPawn->GetNetConnection() ? TEXT("YES") : TEXT("NO"));
            
        MarkControllerDeployed(PC);
    } // End if (player controller possession)

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
