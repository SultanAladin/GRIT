#include "TierRegion.h"
#include "TierClusterBase.h"
#include "TierWorldSubsystem.h"
#include "Tier1A.h"
#include "Tier1B.h"
#include "Tier1C.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

ATierRegion::ATierRegion()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = false;
    SetReplicateMovement(false);

    Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("RegionBounds"));
    SetRootComponent(Bounds);
    Bounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Bounds->SetCollisionObjectType(ECC_WorldStatic);
    Bounds->SetCollisionResponseToAllChannels(ECR_Ignore);
    Bounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Bounds->SetGenerateOverlapEvents(true);
    Bounds->SetMobility(EComponentMobility::Movable);

    Tier1Density.SetNum(TierLayout::Tier1NX);
    for (int32 i = 0; i < Tier1Density.Num(); ++i)
    {
        Tier1Density[i] = ETierDensity::Standard;
    }
}

void ATierRegion::BeginPlay()
{
    Super::BeginPlay();

    RegionOrigin = GetActorLocation();
    ensureMsgf(GetActorRotation().IsNearlyZero(), TEXT("ATierRegion rotation is not supported in v1."));

    // Center the bounds box on the region's footprint center (origin is lower-left corner).
    if (Bounds)
    {
        Bounds->SetRelativeLocation(FVector(TierLayout::RegionExtentX_Cm * 0.5f, TierLayout::RegionExtentY_Cm * 0.5f, 0.f));
        Bounds->SetBoxExtent(FVector(TierLayout::RegionExtentX_Cm * 0.5f, TierLayout::RegionExtentY_Cm * 0.5f, TierLayout::ClusterHeight_Cm * 0.5f));
        Bounds->OnComponentBeginOverlap.AddDynamic(this, &ATierRegion::OnPlayerEnter);
    }

    if (UWorld* World = GetWorld())
    {
        if (UTierWorldSubsystem* Sub = World->GetSubsystem<UTierWorldSubsystem>())
        {
            Sub->Register(this);
        }
    }

    // Seed the active set if the player is already in the region at startup.
    if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
    {
        EvaluateForPlayer(Pawn->GetActorLocation());
    }
}

void ATierRegion::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Bounds)
    {
        Bounds->OnComponentBeginOverlap.RemoveDynamic(this, &ATierRegion::OnPlayerEnter);
    }

    if (UWorld* World = GetWorld())
    {
        if (UTierWorldSubsystem* Sub = World->GetSubsystem<UTierWorldSubsystem>())
        {
            Sub->Unregister(this);
        }
    }

    DestroyAllChildren();
    Super::EndPlay(EndPlayReason);
}

void ATierRegion::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!Pawn) return;
    EvaluateForPlayer(Pawn->GetActorLocation());

#if ENABLE_DRAW_DEBUG
    if (bDrawDebug)
    {
        DrawDebugBox(GetWorld(), Bounds->GetComponentLocation(), Bounds->GetScaledBoxExtent(), FColor::Yellow, false, -1.f, 0, 50.f);
    }
#endif
}

bool ATierRegion::ContainsWorldPos(const FVector& WorldPos) const
{
    if (!Bounds) return false;
    return Bounds->Bounds.GetBox().IsInsideXY(WorldPos);
}

void ATierRegion::OnPlayerEnter(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    APawn* Pawn = Cast<APawn>(OtherActor);
    if (!Pawn || !Pawn->IsPlayerControlled()) return;
    EvaluateForPlayer(OtherActor->GetActorLocation());
}

TSubclassOf<ATierClusterBase> ATierRegion::Tier1ClassForDensity(ETierDensity InDensity) const
{
    switch (InDensity)
    {
        case ETierDensity::Sparse:   return ATier1A::StaticClass();
        case ETierDensity::Standard: return ATier1B::StaticClass();
        case ETierDensity::Dense:    return ATier1C::StaticClass();
    }
    return ATier1B::StaticClass();
}

ATierClusterBase* ATierRegion::SpawnTier1At(const FTierCoord& C)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    const ETierDensity D = (C.X >= 0 && C.X < Tier1Density.Num()) ? Tier1Density[C.X] : ETierDensity::Standard;
    TSubclassOf<ATierClusterBase> ChildClass = Tier1ClassForDensity(D);
    if (!ChildClass) return nullptr;

    const FVector ChildCenterWorld(
        RegionOrigin.X + (C.X + 0.5f) * TierLayout::Tier1CellX_Cm,
        RegionOrigin.Y + (C.Y + 0.5f) * TierLayout::Tier1CellY_Cm,
        RegionOrigin.Z);

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ATierClusterBase* Child = World->SpawnActor<ATierClusterBase>(ChildClass, ChildCenterWorld, FRotator::ZeroRotator, Params);
    if (!Child) return nullptr;

    Child->Region = this;
    Child->ParentCluster = nullptr;
    Child->Coord = C;
    Child->ConfigureBounds(FVector::ZeroVector, FVector2D(TierLayout::Tier1CellX_Cm, TierLayout::Tier1CellY_Cm));

    Tier1Children.Add(Child);
    return Child;
}

void ATierRegion::DestroyAllChildren()
{
    for (TObjectPtr<ATierClusterBase>& Child : Tier1Children)
    {
        if (Child) Child->Destroy();
    }
    Tier1Children.Reset();
}

void ATierRegion::EvaluateForPlayer(const FVector& PlayerWorld)
{
    if (!ContainsWorldPos(PlayerWorld))
    {
        if (Tier1Children.Num() > 0)
        {
            DestroyAllChildren();
            CurrentChildCoord = FTierCoord{ INT32_MIN, INT32_MIN };
        }
        return;
    }

    const FVector LocalPos = PlayerWorld - RegionOrigin;
    FTierCoord Center;
    Center.X = FMath::Clamp(FMath::FloorToInt(LocalPos.X / TierLayout::Tier1CellX_Cm), 0, TierLayout::Tier1NX - 1);
    Center.Y = FMath::Clamp(FMath::FloorToInt(LocalPos.Y / TierLayout::Tier1CellY_Cm), 0, TierLayout::Tier1NY - 1);

    if (Center == CurrentChildCoord && Tier1Children.Num() > 0)
    {
        for (TObjectPtr<ATierClusterBase>& Child : Tier1Children)
        {
            if (Child) Child->EvaluateForPlayer(PlayerWorld);
        }
        return;
    }

    TArray<FTierCoord, TInlineAllocator<9>> Desired;
    ATierClusterBase::Compute3x3Ring(Center, TierLayout::Tier1NX, TierLayout::Tier1NY, Desired);

    for (int32 i = Tier1Children.Num() - 1; i >= 0; --i)
    {
        ATierClusterBase* Child = Tier1Children[i];
        if (!Child) { Tier1Children.RemoveAt(i); continue; }
        if (!Desired.Contains(Child->Coord))
        {
            Child->Destroy();
            Tier1Children.RemoveAt(i);
        }
    }

    for (const FTierCoord& C : Desired)
    {
        bool bAlive = false;
        for (TObjectPtr<ATierClusterBase>& Child : Tier1Children)
        {
            if (Child && Child->Coord == C) { bAlive = true; break; }
        }
        if (!bAlive)
        {
            SpawnTier1At(C);
        }
    }

    CurrentChildCoord = Center;

    for (TObjectPtr<ATierClusterBase>& Child : Tier1Children)
    {
        if (Child) Child->EvaluateForPlayer(PlayerWorld);
    }
}

void ATierRegion::RecordDeferredTenant(const FTierTenantSpec& Spec)
{
    DeferredTenants.Add(Spec);
}

void ATierRegion::ConsumeDeferredTenantsInBox(const FBox2D& BoxXY, TArray<FTierTenantSpec>& Out)
{
    for (int32 i = DeferredTenants.Num() - 1; i >= 0; --i)
    {
        const FVector L = DeferredTenants[i].LocalXform.GetLocation();
        const FVector2D Pt(L.X, L.Y);
        if (BoxXY.IsInside(Pt))
        {
            Out.Add(DeferredTenants[i]);
            DeferredTenants.RemoveAtSwap(i);
        }
    }
}
