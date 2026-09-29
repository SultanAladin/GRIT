#include "TierClusterBase.h"
#include "TierRegion.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

ATierClusterBase::ATierClusterBase()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = false;
    SetReplicateMovement(false);

    Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
    SetRootComponent(Bounds);
    Bounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Bounds->SetCollisionObjectType(ECC_WorldStatic);
    Bounds->SetCollisionResponseToAllChannels(ECR_Ignore);
    Bounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Bounds->SetGenerateOverlapEvents(true);
    Bounds->SetMobility(EComponentMobility::Movable);
}

void ATierClusterBase::ConfigureBounds(const FVector& LocalCenter, const FVector2D& SizeCm)
{
    if (!Bounds) return;
    Bounds->SetRelativeLocation(LocalCenter);
    Bounds->SetBoxExtent(FVector(SizeCm.X * 0.5f, SizeCm.Y * 0.5f, TierLayout::ClusterHeight_Cm * 0.5f), false);
}

void ATierClusterBase::BeginPlay()
{
    Super::BeginPlay();
    if (Bounds)
    {
        Bounds->OnComponentBeginOverlap.AddDynamic(this, &ATierClusterBase::OnPlayerEnter);
    }
}

void ATierClusterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Bounds)
    {
        Bounds->OnComponentBeginOverlap.RemoveDynamic(this, &ATierClusterBase::OnPlayerEnter);
    }
    DestroyAllChildren();
    Super::EndPlay(EndPlayReason);
}

void ATierClusterBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!Pawn) return;
    EvaluateForPlayer(Pawn->GetActorLocation());
}

void ATierClusterBase::OnPlayerEnter(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    if (!OtherActor) return;
    APawn* Pawn = Cast<APawn>(OtherActor);
    if (!Pawn || !Pawn->IsPlayerControlled()) return;
    EvaluateForPlayer(OtherActor->GetActorLocation());
}

FTierCoord ATierClusterBase::WorldToChildCoord(const FVector& WorldPos) const
{
    const FVector2D CellSize = GetChildCellSizeCm();
    const int32 NX = GetChildNX();
    const int32 NY = GetChildNY();
    if (CellSize.X <= 0.f || CellSize.Y <= 0.f || NX <= 0 || NY <= 0) return FTierCoord{ 0, 0 };

    // Origin for child indexing = lower-left corner of THIS cluster's footprint.
    // For a cluster, that is its world location minus half-extent on each axis.
    const FVector ClusterOrigin = GetActorLocation() - FVector(NX * CellSize.X * 0.5f, NY * CellSize.Y * 0.5f, 0.f);
    const FVector LocalPos = WorldPos - ClusterOrigin;

    // Lower-index-wins on boundaries via FloorToInt.
    FTierCoord C;
    C.X = FMath::Clamp(FMath::FloorToInt(LocalPos.X / CellSize.X), 0, NX - 1);
    C.Y = FMath::Clamp(FMath::FloorToInt(LocalPos.Y / CellSize.Y), 0, NY - 1);
    return C;
}

void ATierClusterBase::Compute3x3Ring(const FTierCoord& Center, int32 NX, int32 NY, TArray<FTierCoord, TInlineAllocator<9>>& Out)
{
    Out.Reset();
    for (int32 dy = -1; dy <= 1; ++dy)
    {
        for (int32 dx = -1; dx <= 1; ++dx)
        {
            const int32 x = Center.X + dx;
            const int32 y = Center.Y + dy;
            if (x < 0 || x >= NX) continue;
            if (y < 0 || y >= NY) continue;
            Out.Add(FTierCoord{ x, y });
        }
    }
}

void ATierClusterBase::DestroyAllChildren()
{
    for (TObjectPtr<ATierClusterBase>& Child : ChildClusters)
    {
        if (Child)
        {
            Child->Destroy();
        }
    }
    ChildClusters.Reset();
}

ATierClusterBase* ATierClusterBase::SpawnChildAt(const FTierCoord& C)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    TSubclassOf<ATierClusterBase> ChildClass = ChildClassForDensity(Density);
    if (!ChildClass) return nullptr;

    const FVector2D CellSize = GetChildCellSizeCm();
    const int32 NX = GetChildNX();
    const int32 NY = GetChildNY();
    const FVector ClusterOrigin = GetActorLocation() - FVector(NX * CellSize.X * 0.5f, NY * CellSize.Y * 0.5f, 0.f);
    const FVector ChildCenterWorld(
        ClusterOrigin.X + (C.X + 0.5f) * CellSize.X,
        ClusterOrigin.Y + (C.Y + 0.5f) * CellSize.Y,
        ClusterOrigin.Z);

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ATierClusterBase* Child = World->SpawnActor<ATierClusterBase>(ChildClass, ChildCenterWorld, FRotator::ZeroRotator, Params);
    if (!Child) return nullptr;

    Child->Region = Region;
    Child->ParentCluster = this;
    Child->Coord = C;
    Child->ConfigureBounds(FVector::ZeroVector, CellSize);

    ChildClusters.Add(Child);
    return Child;
}

void ATierClusterBase::EvaluateForPlayer(const FVector& PlayerWorld)
{
    const int32 NX = GetChildNX();
    const int32 NY = GetChildNY();
    if (NX <= 0 || NY <= 0) return;

    const FTierCoord Center = WorldToChildCoord(PlayerWorld);
    if (Center == CurrentChildCoord && ChildClusters.Num() > 0)
    {
        // No diff at this tier; still cascade so deeper tiers re-evaluate.
        for (TObjectPtr<ATierClusterBase>& Child : ChildClusters)
        {
            if (Child) Child->EvaluateForPlayer(PlayerWorld);
        }
        return;
    }

    TArray<FTierCoord, TInlineAllocator<9>> Desired;
    Compute3x3Ring(Center, NX, NY, Desired);

    // Destroy children no longer in the desired set.
    for (int32 i = ChildClusters.Num() - 1; i >= 0; --i)
    {
        ATierClusterBase* Child = ChildClusters[i];
        if (!Child) { ChildClusters.RemoveAt(i); continue; }
        if (!Desired.Contains(Child->Coord))
        {
            Child->Destroy();
            ChildClusters.RemoveAt(i);
        }
    }

    // Spawn missing.
    for (const FTierCoord& C : Desired)
    {
        bool bAlive = false;
        for (TObjectPtr<ATierClusterBase>& Child : ChildClusters)
        {
            if (Child && Child->Coord == C) { bAlive = true; break; }
        }
        if (!bAlive)
        {
            SpawnChildAt(C);
        }
    }

    CurrentChildCoord = Center;

    // Cascade.
    for (TObjectPtr<ATierClusterBase>& Child : ChildClusters)
    {
        if (Child) Child->EvaluateForPlayer(PlayerWorld);
    }
}
