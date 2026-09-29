#include "SpatialGridSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Landscape.h"
#include "LandscapeProxy.h"
#include "Components/RuntimeVirtualTextureComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Camera/PlayerCameraManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSpatialGrid, Log, All);

//==============================================================================
//                          CONSOLE VARIABLES (debug toggles)
//==============================================================================
//
//  At runtime in PIE/Editor, open the console (~) and type:
//      grit.GridDebugL1 1     (landscape entries)
//      grit.GridDebugL2 1     (200m region cells)
//      grit.GridDebugL3 1     (50m detail cells)
//      grit.GridDebugAll 1    (toggle all three at once)
//      grit.GridDebugHeight 800   (set debug box height in cm)
//
//  Pass 0 to turn off. Toggles take effect on the next tick — no restart.
//
static TAutoConsoleVariable<int32> CVarGridDebugL1(
    TEXT("grit.GridDebugL1"),
    0,
    TEXT("Draw Level 1 (landscape) debug boxes. 0=off, 1=on."),
    ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarGridDebugL2(
    TEXT("grit.GridDebugL2"),
    0,
    TEXT("Draw Level 2 (200m region) debug boxes. 0=off, 1=on."),
    ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarGridDebugL3(
    TEXT("grit.GridDebugL3"),
    0,
    TEXT("Draw Level 3 (50m detail) debug boxes. 0=off, 1=on."),
    ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarGridDebugAll(
    TEXT("grit.GridDebugAll"),
    0,
    TEXT("Toggle all spatial grid debug levels. 0=off, 1=on."),
    ECVF_Cheat);

static TAutoConsoleVariable<float> CVarGridDebugHeight(
    TEXT("grit.GridDebugHeight"),
    -1.0f,
    TEXT("Override debug box height in cm. -1 = use UPROPERTY default."),
    ECVF_Cheat);

//==============================================================================
//                          LIFECYCLE
//==============================================================================

void USpatialGridSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    TickCounter = 0;

    OnCellEntered.AddUObject(this, &USpatialGridSubsystem::HandleCellEntered);
    OnCellExited.AddUObject(this,  &USpatialGridSubsystem::HandleCellExited);
}

void USpatialGridSubsystem::Deinitialize()
{
    LandscapeRegistry.Empty();
    RegionGrid.Empty();
    DetailGrid.Empty();
    ActorToRegionCell.Empty();
    Super::Deinitialize();
}

TStatId USpatialGridSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(USpatialGridSubsystem, STATGROUP_Tickables);
}

//==============================================================================
//                          TICK
//==============================================================================

void USpatialGridSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Resolve primary actor (fallback to first local player pawn)
    AActor* Pawn = PrimaryActor.Get();
    if (!Pawn)
    {
        if (UWorld* World = GetWorld())
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                Pawn = PC->GetPawn();
            }
        }
    }

    if (!Pawn)
        return;

    const FVector PawnLocation = Pawn->GetActorLocation();

    // Cell-transition detection for all registered vehicles (fires delegates on boundary cross)
    UpdateVehicleCellTransitions();

    // Level 3: every tick (cheap — only checks ~9 cells)
    UpdateDetailGrid(PawnLocation);

    // Fluid-sim lifecycle (fade out empty cells)
    TickDetailFluidSims(DeltaTime);

    // Level 2: every 6 ticks (~5Hz at 30fps)
    if (TickCounter % 6 == 0)
    {
        UpdateRegionGrid(PawnLocation);
    }

    // Level 1: every 60 ticks (~0.5Hz)
    if (TickCounter % 60 == 0)
    {
        UpdateLandscapeStreaming(PawnLocation);
    }

    // Debug draw — UPROPERTYs OR console variables can enable each level.
    // grit.GridDebugAll forces all three on (overrides the per-level CVars and UPROPERTYs).
    const bool bAllOn = CVarGridDebugAll.GetValueOnGameThread() != 0;
    const bool bL1 = bAllOn || bDrawDebugLevel1 || CVarGridDebugL1.GetValueOnGameThread() != 0;
    const bool bL2 = bAllOn || bDrawDebugLevel2 || CVarGridDebugL2.GetValueOnGameThread() != 0;
    const bool bL3 = bAllOn || bDrawDebugLevel3 || CVarGridDebugL3.GetValueOnGameThread() != 0;
    if (bL1 || bL2 || bL3)
    {
        // Stash CVar overrides so DrawDebug can use them without changing its signature
        const bool SavedL1 = bDrawDebugLevel1, SavedL2 = bDrawDebugLevel2, SavedL3 = bDrawDebugLevel3;
        const float SavedHeight = DebugBoxHeight;
        bDrawDebugLevel1 = bL1;
        bDrawDebugLevel2 = bL2;
        bDrawDebugLevel3 = bL3;
        const float CVarHeight = CVarGridDebugHeight.GetValueOnGameThread();
        if (CVarHeight > 0.0f) DebugBoxHeight = CVarHeight;

        DrawDebug(PawnLocation);

        bDrawDebugLevel1 = SavedL1;
        bDrawDebugLevel2 = SavedL2;
        bDrawDebugLevel3 = SavedL3;
        DebugBoxHeight = SavedHeight;
    }

    ++TickCounter;
}

//==============================================================================
//                          REGISTRATION
//==============================================================================

void USpatialGridSubsystem::RegisterActor(AActor* Actor)
{
    if (!Actor)
        return;

    const FVector Pos = Actor->GetActorLocation();
    const FIntVector2 RegionCoord = WorldToCell(Pos, RegionCellSize);

    // Add to L2 region grid
    FRegionCell& Region = RegionGrid.FindOrAdd(RegionCoord);
    Region.Coord = RegionCoord;
    Region.WorldBounds = FBox2D(
        FVector2D(RegionCoord.X * RegionCellSize, RegionCoord.Y * RegionCellSize),
        FVector2D((RegionCoord.X + 1) * RegionCellSize, (RegionCoord.Y + 1) * RegionCellSize));
    Region.Actors.AddUnique(Actor);

    // Track reverse mapping
    ActorToRegionCell.Add(Actor, RegionCoord);

    // Add to L3 detail grid
    const FIntVector2 DetailCoord = WorldToCell(Pos, DetailCellSize);
    FDetailCell& Detail = DetailGrid.FindOrAdd(DetailCoord);
    Detail.Coord = DetailCoord;
    Detail.WorldBounds = FBox2D(
        FVector2D(DetailCoord.X * DetailCellSize, DetailCoord.Y * DetailCellSize),
        FVector2D((DetailCoord.X + 1) * DetailCellSize, (DetailCoord.Y + 1) * DetailCellSize));
    Detail.Actors.AddUnique(Actor);
}

void USpatialGridSubsystem::UnregisterActor(AActor* Actor)
{
    if (!Actor)
        return;

    // Remove from L2
    if (FIntVector2* CellCoord = ActorToRegionCell.Find(Actor))
    {
        if (FRegionCell* Cell = RegionGrid.Find(*CellCoord))
        {
            Cell->Actors.Remove(Actor);
            if (Cell->Actors.Num() == 0)
            {
                RegionGrid.Remove(*CellCoord);
            }
        }
        ActorToRegionCell.Remove(Actor);
    }

    // Remove from L3 (scan — actors may have moved)
    for (auto It = DetailGrid.CreateIterator(); It; ++It)
    {
        It.Value().Actors.Remove(Actor);
        if (It.Value().Actors.Num() == 0)
        {
            It.RemoveCurrent();
        }
    }
}

void USpatialGridSubsystem::RegisterTieredActor(AActor* Actor, ESpatialTier Tier)
{
    if (!Actor)
    {
        return;
    }
    if (Tier == ESpatialTier::Tier3)
    {
        // Tier3 (terrain / large static) stays always-on — no grid bookkeeping.
        return;
    }

    const FIntVector2 Coord = WorldToCell(Actor->GetActorLocation(), RegionCellSize);
    FRegionCell& Cell = RegionGrid.FindOrAdd(Coord);
    Cell.Coord = Coord;
    if (Cell.WorldBounds.bIsValid == 0 || Cell.WorldBounds.GetArea() <= 0.0f)
    {
        Cell.WorldBounds = FBox2D(
            FVector2D(Coord.X * RegionCellSize, Coord.Y * RegionCellSize),
            FVector2D((Coord.X + 1) * RegionCellSize, (Coord.Y + 1) * RegionCellSize));
    }

    TArray<TWeakObjectPtr<AActor>>& Bucket = (Tier == ESpatialTier::Tier1) ? Cell.Tier1Actors : Cell.Tier2Actors;
    Bucket.AddUnique(Actor);
    ActorToRegionCell.Add(Actor, Coord);
}

void USpatialGridSubsystem::UnregisterTieredActor(AActor* Actor, ESpatialTier Tier)
{
    if (!Actor || Tier == ESpatialTier::Tier3)
    {
        return;
    }
    if (FIntVector2* CellCoord = ActorToRegionCell.Find(Actor))
    {
        if (FRegionCell* Cell = RegionGrid.Find(*CellCoord))
        {
            if (Tier == ESpatialTier::Tier1)
            {
                Cell->Tier1Actors.Remove(Actor);
            }
            else
            {
                Cell->Tier2Actors.Remove(Actor);
            }
        }
    }
}

void USpatialGridSubsystem::RegisterLandscape(ALandscapeProxy* Landscape,
                                               URuntimeVirtualTextureComponent* RVT,
                                               TSoftObjectPtr<UWorld> LevelAsset)
{
    if (!Landscape)
        return;

    FLandscapeStreamingEntry Entry;
    Entry.Landscape = Landscape;
    Entry.RVTComponent = RVT;
    Entry.LevelAsset = LevelAsset;
    Entry.WorldBounds = Landscape->GetComponentsBoundingBox(true);
    Entry.StreamState = 2; // Already loaded if we can see it

    LandscapeRegistry.Add(Landscape, Entry);

    UE_LOG(LogSpatialGrid, Log, TEXT("Registered landscape: %s  Bounds: %s"),
        *Landscape->GetName(), *Entry.WorldBounds.ToString());
}

void USpatialGridSubsystem::UnregisterLandscape(ALandscapeProxy* Landscape)
{
    LandscapeRegistry.Remove(Landscape);
}

//==============================================================================
//                          QUERIES
//==============================================================================

float USpatialGridSubsystem::GetVFXQualityAt(const FVector& WorldPos) const
{
    const FIntVector2 Coord = WorldToCell(WorldPos, RegionCellSize);
    if (const FRegionCell* Cell = RegionGrid.Find(Coord))
    {
        return Cell->VFXQualityScale;
    }
    return 0.0f;
}

int32 USpatialGridSubsystem::GetVFXResolutionAt(const FVector& WorldPos) const
{
    // Find the nearest registered vehicle to this position.
    float NearestDistSq = MAX_FLT;
    for (const TWeakObjectPtr<AActor>& VW : RegisteredVehicles)
    {
        if (const AActor* V = VW.Get())
        {
            const float D2 = FVector::DistSquared(V->GetActorLocation(), WorldPos);
            NearestDistSq = FMath::Min(NearestDistSq, D2);
        }
    }
    const float NearestDist_cm = FMath::Sqrt(NearestDistSq);

    // Walk the distance bands: each band step drops resolution by 25.
    // e.g. Max=100, bands={2000,5000,10000,50000}
    //   0-20m   → 100
    //   20-50m  → 75
    //   50-100m → 50
    //   100-500m → 25
    //   >500m   → 0 (off)
    int32 Resolution = MaxFluidResolution;
    for (int32 i = 0; i < VFXDistanceBands_cm.Num(); ++i)
    {
        if (NearestDist_cm <= VFXDistanceBands_cm[i])
        {
            break;
        }
        Resolution -= 25;
    }
    // If we passed all bands, turn off.
    if (VFXDistanceBands_cm.Num() > 0 && NearestDist_cm > VFXDistanceBands_cm.Last())
    {
        Resolution = 0;
    }
    // Minimum useful resolution is 25; below that is off.
    if (Resolution < 25)
    {
        Resolution = 0;
    }
    return FMath::Clamp(Resolution, 0, MaxFluidResolution);
}

ESpatialVFXQuality USpatialGridSubsystem::GetVFXQualityBucketAt(const FVector& WorldPos) const
{
    const int32 Res = GetVFXResolutionAt(WorldPos);
    if (Res >= 75)  return ESpatialVFXQuality::High;
    if (Res >= 50)  return ESpatialVFXQuality::Medium;
    if (Res >= 25)  return ESpatialVFXQuality::Low;
    return ESpatialVFXQuality::Off;
}

URuntimeVirtualTextureComponent* USpatialGridSubsystem::GetRVTAt(const FVector& WorldPos) const
{
    // Find which landscape contains this position
    for (const auto& Pair : LandscapeRegistry)
    {
        if (Pair.Value.WorldBounds.IsInsideOrOn(WorldPos))
        {
            return Pair.Value.RVTComponent.Get();
        }
    }
    return nullptr;
}

bool USpatialGridSubsystem::IsInDetailZone(const FVector& WorldPos) const
{
    const FIntVector2 Coord = WorldToCell(WorldPos, DetailCellSize);
    if (const FDetailCell* Cell = DetailGrid.Find(Coord))
    {
        return Cell->bNiagaraActive;
    }
    return false;
}

FLandscapeStreamingEntry* USpatialGridSubsystem::GetLandscapeAt(const FVector& WorldPos)
{
    for (auto& Pair : LandscapeRegistry)
    {
        if (Pair.Value.WorldBounds.IsInsideOrOn(WorldPos))
        {
            return &Pair.Value;
        }
    }
    return nullptr;
}

void USpatialGridSubsystem::SetPrimaryActor(AActor* Actor)
{
    PrimaryActor = Actor;
}

//==============================================================================
//                          COORDINATE HELPERS
//==============================================================================

FIntVector2 USpatialGridSubsystem::WorldToCell(const FVector& WorldPos, float CellSize) const
{
    return FIntVector2(
        FMath::FloorToInt(WorldPos.X / CellSize),
        FMath::FloorToInt(WorldPos.Y / CellSize));
}

FVector USpatialGridSubsystem::CellToWorld(const FIntVector2& Coord, float CellSize) const
{
    return FVector(
        (Coord.X + 0.5f) * CellSize,
        (Coord.Y + 0.5f) * CellSize,
        0.0f);
}

FBox USpatialGridSubsystem::CellWorldBounds(const FIntVector2& Coord, float CellSize) const
{
    const FVector Min(Coord.X * CellSize, Coord.Y * CellSize, -DebugBoxHeight);
    const FVector Max((Coord.X + 1) * CellSize, (Coord.Y + 1) * CellSize, DebugBoxHeight);
    return FBox(Min, Max);
}

//==============================================================================
//                          LEVEL 3: DETAIL GRID UPDATE
//==============================================================================

void USpatialGridSubsystem::UpdateDetailGrid(const FVector& PawnLocation)
{
    // Deactivate all detail cells first
    for (auto& Pair : DetailGrid)
    {
        Pair.Value.bNiagaraActive = false;
        Pair.Value.RVTComponent = nullptr;
    }

    // Build the set of anchor cells — one per registered vehicle, falling back to the
    // subsystem's pawn when no vehicles have registered yet (e.g. editor).
    TArray<FIntVector2> AnchorCells;
    for (const TWeakObjectPtr<AActor>& VW : RegisteredVehicles)
    {
        if (const AActor* V = VW.Get())
        {
            AnchorCells.Add(WorldToCell(V->GetActorLocation(), DetailCellSize));
        }
    }
    if (AnchorCells.Num() == 0)
    {
        AnchorCells.Add(WorldToCell(PawnLocation, DetailCellSize));
    }

    // Activate cells within DetailActiveRadius of every anchor (union)
    for (const FIntVector2& Anchor : AnchorCells)
    {
        for (int32 DX = -DetailActiveRadius; DX <= DetailActiveRadius; ++DX)
        {
            for (int32 DY = -DetailActiveRadius; DY <= DetailActiveRadius; ++DY)
            {
                const FIntVector2 Coord(Anchor.X + DX, Anchor.Y + DY);

                FDetailCell& Cell = DetailGrid.FindOrAdd(Coord);
                Cell.Coord = Coord;
                Cell.WorldBounds = FBox2D(
                    FVector2D(Coord.X * DetailCellSize, Coord.Y * DetailCellSize),
                    FVector2D((Coord.X + 1) * DetailCellSize, (Coord.Y + 1) * DetailCellSize));
                Cell.bNiagaraActive = true;

                // Look up RVT from landscape registry
                const FVector CellCenter = CellToWorld(Coord, DetailCellSize);
                for (const auto& LandPair : LandscapeRegistry)
                {
                    if (LandPair.Value.WorldBounds.IsInsideOrOn(CellCenter))
                    {
                        Cell.RVTComponent = LandPair.Value.RVTComponent;
                        break;
                    }
                }
            }
        }
    }
}

//==============================================================================
//                          LEVEL 2: REGION GRID UPDATE
//==============================================================================

void USpatialGridSubsystem::UpdateRegionGrid(const FVector& PawnLocation)
{
    // Build anchor cells (one per registered vehicle, or fall back to the pawn).
    TArray<FIntVector2> AnchorCells;
    for (const TWeakObjectPtr<AActor>& VW : RegisteredVehicles)
    {
        if (const AActor* V = VW.Get())
        {
            AnchorCells.Add(WorldToCell(V->GetActorLocation(), RegionCellSize));
        }
    }
    if (AnchorCells.Num() == 0)
    {
        AnchorCells.Add(WorldToCell(PawnLocation, RegionCellSize));
    }

    for (auto& Pair : RegionGrid)
    {
        FRegionCell& Cell = Pair.Value;

        // Cell distance = Chebyshev distance to the nearest anchor vehicle
        int32 NearestDistance = MAX_int32;
        for (const FIntVector2& Anchor : AnchorCells)
        {
            const int32 DX = FMath::Abs(Cell.Coord.X - Anchor.X);
            const int32 DY = FMath::Abs(Cell.Coord.Y - Anchor.Y);
            NearestDistance = FMath::Min(NearestDistance, FMath::Max(DX, DY));
        }
        Cell.CellDistance = NearestDistance;

        // Determine VFX quality based on distance
        if (Cell.CellDistance == 0)
        {
            Cell.VFXQualityScale = 1.0f;
            Cell.bCollisionEnabled = true;
        }
        else if (Cell.CellDistance <= RegionVFXReducedRange)
        {
            Cell.VFXQualityScale = 0.5f;
            Cell.bCollisionEnabled = true;
        }
        else if (Cell.CellDistance <= RegionCullRange)
        {
            Cell.VFXQualityScale = 0.15f;
            Cell.bCollisionEnabled = false;
        }
        else
        {
            Cell.VFXQualityScale = 0.0f;
            Cell.bCollisionEnabled = false;
        }

        // 3-state collision push (no-ops inside if nothing changed per-primitive)
        ApplyCellCollision(Cell);
    }
}

//==============================================================================
//                          LEVEL 1: LANDSCAPE STREAMING
//==============================================================================

void USpatialGridSubsystem::UpdateLandscapeStreaming(const FVector& PawnLocation)
{
    // Find which landscape the pawn is currently on
    ALandscapeProxy* CurrentLandscape = nullptr;
    for (auto& Pair : LandscapeRegistry)
    {
        if (Pair.Value.WorldBounds.IsInsideOrOn(PawnLocation))
        {
            CurrentLandscape = Pair.Key.Get();
            Pair.Value.StreamState = 2; // Active
        }
    }

    // For now, just log state. Actual level load/unload integrates with SpatialBridge
    // which already handles portal-based teleportation between landscapes.
    // When SpatialBridge triggers a teleport, it should call:
    //   Grid->RegisterLandscape(NewLandscape, NewRVT, NextLevelAsset)
    //   Grid->UnregisterLandscape(OldLandscape)
}

//==============================================================================
//                          COLLISION TOGGLE
//==============================================================================

ECollisionEnabled::Type USpatialGridSubsystem::DesiredCollisionForDistance(int32 CellDistance) const
{
    // L2 cell distance from the nearest vehicle:
    //  0..RegionVFXReducedRange  -> full physics (query + sim)
    //  within RegionCullRange    -> query-only (traces hit, no push)
    //  beyond                    -> no collision at all
    if (CellDistance <= RegionVFXReducedRange)
    {
        return ECollisionEnabled::QueryAndPhysics;
    }
    if (CellDistance <= RegionCullRange)
    {
        return ECollisionEnabled::QueryOnly;
    }
    return ECollisionEnabled::NoCollision;
}

static void ApplyCollisionToBucket(TArray<TWeakObjectPtr<AActor>>& Bucket, ECollisionEnabled::Type Desired)
{
    for (int32 i = Bucket.Num() - 1; i >= 0; --i)
    {
        AActor* Actor = Bucket[i].Get();
        if (!Actor)
        {
            Bucket.RemoveAtSwap(i);
            continue;
        }
        TInlineComponentArray<UPrimitiveComponent*> Prims(Actor);
        for (UPrimitiveComponent* Prim : Prims)
        {
            if (Prim && Prim->GetCollisionEnabled() != Desired)
            {
                Prim->SetCollisionEnabled(Desired);
            }
        }
    }
}

void USpatialGridSubsystem::ApplyCellCollision(FRegionCell& Cell)
{
    const ECollisionEnabled::Type Desired = DesiredCollisionForDistance(Cell.CellDistance);
    ApplyCollisionToBucket(Cell.Tier1Actors, Desired);
    ApplyCollisionToBucket(Cell.Tier2Actors, Desired);
    // Tier3: commented out per design — terrain/large objects remain authoritative.
}

void USpatialGridSubsystem::UpdateVehicleCellTransitions()
{
    for (int32 i = RegisteredVehicles.Num() - 1; i >= 0; --i)
    {
        AActor* Vehicle = RegisteredVehicles[i].Get();
        if (!Vehicle)
        {
            RegisteredVehicles.RemoveAtSwap(i);
            continue;
        }

        const FVector Loc = Vehicle->GetActorLocation();
        const FIntVector2 NewRegion = WorldToCell(Loc, RegionCellSize);
        const FIntVector2 NewDetail = WorldToCell(Loc, DetailCellSize);

        FIntVector2* OldRegionPtr = VehicleLastRegionCell.Find(Vehicle);
        if (!OldRegionPtr || *OldRegionPtr != NewRegion)
        {
            if (OldRegionPtr)
            {
                OnCellExited.Broadcast(Vehicle, *OldRegionPtr, 2);
            }
            OnCellEntered.Broadcast(Vehicle, NewRegion, 2);
            VehicleLastRegionCell.Add(Vehicle, NewRegion);
        }

        FIntVector2* OldDetailPtr = VehicleLastDetailCell.Find(Vehicle);
        if (!OldDetailPtr || *OldDetailPtr != NewDetail)
        {
            if (OldDetailPtr)
            {
                OnCellExited.Broadcast(Vehicle, *OldDetailPtr, 3);
            }
            OnCellEntered.Broadcast(Vehicle, NewDetail, 3);
            VehicleLastDetailCell.Add(Vehicle, NewDetail);
        }
    }
}

void USpatialGridSubsystem::RegisterVehicle(AActor* Vehicle)
{
    if (!Vehicle)
    {
        return;
    }
    RegisteredVehicles.AddUnique(Vehicle);
}

void USpatialGridSubsystem::UnregisterVehicle(AActor* Vehicle)
{
    if (!Vehicle)
    {
        return;
    }
    RegisteredVehicles.RemoveAll([Vehicle](const TWeakObjectPtr<AActor>& P) { return P.Get() == Vehicle; });
    if (FIntVector2* Last = VehicleLastRegionCell.Find(Vehicle))
    {
        OnCellExited.Broadcast(Vehicle, *Last, 2);
        VehicleLastRegionCell.Remove(Vehicle);
    }
    if (FIntVector2* Last = VehicleLastDetailCell.Find(Vehicle))
    {
        OnCellExited.Broadcast(Vehicle, *Last, 3);
        VehicleLastDetailCell.Remove(Vehicle);
    }
}

//==============================================================================
//                          DEBUG DRAW
//==============================================================================

void USpatialGridSubsystem::DrawDebug(const FVector& PawnLocation)
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    const float LifeTime = -1.0f; // 1 frame
    const float Thickness = 2.0f;

    // Level 1: Landscape bounds (RED)
    if (bDrawDebugLevel1)
    {
        for (const auto& Pair : LandscapeRegistry)
        {
            const FBox& Bounds = Pair.Value.WorldBounds;
            const bool bActive = Bounds.IsInsideOrOn(PawnLocation);
            const FColor Color = bActive ? FColor(255, 80, 80, 200) : FColor(150, 40, 40, 100);

            DrawDebugBox(World, Bounds.GetCenter(), Bounds.GetExtent(), Color,
                false, LifeTime, 0, bActive ? 4.0f : Thickness);

            DrawDebugString(World,
                Bounds.GetCenter() + FVector(0, 0, Bounds.GetExtent().Z + 100.0f),
                FString::Printf(TEXT("L1 %s [%s]"),
                    Pair.Key.IsValid() ? *Pair.Key->GetName() : TEXT("???"),
                    bActive ? TEXT("ACTIVE") : TEXT("idle")),
                nullptr, Color, LifeTime);
        }
    }

    // Level 2: Region cells (YELLOW)
    if (bDrawDebugLevel2)
    {
        const FIntVector2 PawnCell = WorldToCell(PawnLocation, RegionCellSize);

        for (const auto& Pair : RegionGrid)
        {
            const FRegionCell& Cell = Pair.Value;
            const FBox Bounds = CellWorldBounds(Cell.Coord, RegionCellSize);
            const bool bPawnCell = (Cell.Coord == PawnCell);

            FColor Color;
            if (bPawnCell)
                Color = FColor(255, 255, 50, 200);
            else if (Cell.bCollisionEnabled)
                Color = FColor(200, 200, 30, 120);
            else
                Color = FColor(120, 120, 20, 60);

            DrawDebugBox(World, Bounds.GetCenter(), Bounds.GetExtent(), Color,
                false, LifeTime, 0, bPawnCell ? 4.0f : Thickness);

            DrawDebugString(World,
                Bounds.GetCenter() + FVector(0, 0, DebugBoxHeight + 50.0f),
                FString::Printf(TEXT("L2[%d,%d] VFX:%.1f COL:%s D:%d A:%d"),
                    Cell.Coord.X, Cell.Coord.Y,
                    Cell.VFXQualityScale,
                    Cell.bCollisionEnabled ? TEXT("ON") : TEXT("OFF"),
                    Cell.CellDistance,
                    Cell.Actors.Num()),
                nullptr, Color, LifeTime);
        }
    }

    // Level 3: Detail cells (GREEN)
    if (bDrawDebugLevel3)
    {
        const FIntVector2 PawnCell = WorldToCell(PawnLocation, DetailCellSize);

        for (const auto& Pair : DetailGrid)
        {
            const FDetailCell& Cell = Pair.Value;
            if (!Cell.bNiagaraActive && Cell.Actors.Num() == 0)
                continue; // Skip empty inactive cells

            const FBox Bounds = CellWorldBounds(Cell.Coord, DetailCellSize);
            const bool bPawnCell = (Cell.Coord == PawnCell);

            FColor Color;
            if (bPawnCell)
                Color = FColor(50, 255, 50, 200);
            else if (Cell.bNiagaraActive)
                Color = FColor(30, 200, 30, 120);
            else
                Color = FColor(20, 100, 20, 60);

            DrawDebugBox(World, Bounds.GetCenter(), Bounds.GetExtent(), Color,
                false, LifeTime, 0, bPawnCell ? 3.0f : 1.5f);

            DrawDebugString(World,
                Bounds.GetCenter() + FVector(0, 0, DebugBoxHeight + 20.0f),
                FString::Printf(TEXT("L3[%d,%d] %s RVT:%s"),
                    Cell.Coord.X, Cell.Coord.Y,
                    Cell.bNiagaraActive ? TEXT("ACTIVE") : TEXT("idle"),
                    Cell.RVTComponent.IsValid() ? TEXT("Y") : TEXT("N")),
                nullptr, Color, LifeTime);
        }
    }
}

//==============================================================================
//                          PER-CELL FLUID-SIM LIFECYCLE
//==============================================================================

UNiagaraComponent* USpatialGridSubsystem::EnsureFluidSimForCell(FDetailCell& Cell)
{
    if (UNiagaraComponent* Existing = Cell.FluidSim.Get())
    {
        return Existing;
    }
    if (!CellFluidSystem)
    {
        return nullptr;
    }
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    const FVector CellCenter = CellToWorld(Cell.Coord, DetailCellSize);
    UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        World, CellFluidSystem, CellCenter, FRotator::ZeroRotator,
        FVector(1.0f), /*bAutoDestroy*/ false, /*bAutoActivate*/ true);

    if (NC)
    {
        // Publish cell bounds as user parameters so the fluid sim can size its grid accordingly.
        static const FName BoundsCenterParam(TEXT("FluidBoundsCenter"));
        static const FName BoundsExtentParam(TEXT("FluidBoundsExtent"));
        static const FName ResolutionParam(TEXT("FluidResolution"));
        NC->SetVariableVec3(BoundsCenterParam, CellCenter);
        NC->SetVariableVec3(BoundsExtentParam, FVector(DetailCellSize * 0.5f));
        NC->SetVariableInt(ResolutionParam, GetVFXResolutionAt(CellCenter));
    }
    Cell.FluidSim = NC;
    return NC;
}

void USpatialGridSubsystem::HandleCellEntered(AActor* Vehicle, FIntVector2 Coord, int32 GridLevel)
{
    if (GridLevel != 3)
    {
        return;
    }
    FDetailCell& Cell = DetailGrid.FindOrAdd(Coord);
    Cell.Coord = Coord;
    Cell.WorldBounds = FBox2D(
        FVector2D(Coord.X * DetailCellSize, Coord.Y * DetailCellSize),
        FVector2D((Coord.X + 1) * DetailCellSize, (Coord.Y + 1) * DetailCellSize));

    ++Cell.VehicleRefCount;
    Cell.FadeOutRemaining = 0.0f;
    EnsureFluidSimForCell(Cell);
    if (UNiagaraComponent* NC = Cell.FluidSim.Get())
    {
        if (!NC->IsActive())
        {
            NC->Activate(false);
        }
    }
}

void USpatialGridSubsystem::HandleCellExited(AActor* Vehicle, FIntVector2 Coord, int32 GridLevel)
{
    if (GridLevel != 3)
    {
        return;
    }
    if (FDetailCell* Cell = DetailGrid.Find(Coord))
    {
        Cell->VehicleRefCount = FMath::Max(0, Cell->VehicleRefCount - 1);
        if (Cell->VehicleRefCount == 0)
        {
            Cell->FadeOutRemaining = EmptyCellFadeOutSeconds;
        }
    }
}

void USpatialGridSubsystem::TickDetailFluidSims(float DeltaTime)
{
    // Gather camera state for frustum culling (cheapest possible: dot-product behind check).
    FVector CamLoc  = FVector::ZeroVector;
    FVector CamFwd  = FVector::ForwardVector;
    bool bHaveCamera = false;
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (APlayerCameraManager* Cam = PC->PlayerCameraManager)
            {
                CamLoc = Cam->GetCameraLocation();
                CamFwd = Cam->GetActorForwardVector();
                bHaveCamera = true;
            }
        }
    }

    for (auto It = DetailGrid.CreateIterator(); It; ++It)
    {
        FDetailCell& Cell = It.Value();

        // Frustum cull: if the cell centre is well behind the camera, deactivate its sim.
        if (bHaveCamera && Cell.VehicleRefCount > 0)
        {
            UNiagaraComponent* NC = Cell.FluidSim.Get();
            if (NC)
            {
                const FVector CellCenter = CellToWorld(Cell.Coord, DetailCellSize);
                const FVector ToCell = (CellCenter - CamLoc).GetSafeNormal();
                const float Dot = FVector::DotProduct(CamFwd, ToCell);
                if (Dot < -0.2f)
                {
                    if (NC->IsActive())
                    {
                        NC->Deactivate();
                    }
                }
                else
                {
                    if (!NC->IsActive())
                    {
                        NC->Activate(false);
                    }
                }
            }
        }

        // Fade-out empty cells
        if (Cell.VehicleRefCount > 0)
        {
            continue;
        }
        if (Cell.FadeOutRemaining > 0.0f)
        {
            Cell.FadeOutRemaining -= DeltaTime;
            if (Cell.FadeOutRemaining <= 0.0f)
            {
                if (UNiagaraComponent* NC = Cell.FluidSim.Get())
                {
                    NC->Deactivate();
                    NC->DestroyComponent();
                }
                Cell.FluidSim = nullptr;
            }
        }
    }
}

void USpatialGridSubsystem::GetFluidSimsForPosition(const FVector& WorldPos, float InjectRadius_cm, TArray<UNiagaraComponent*>& OutSims)
{
    OutSims.Reset();
    const FIntVector2 Center = WorldToCell(WorldPos, DetailCellSize);
    const int32 R = FMath::Clamp(FMath::CeilToInt(InjectRadius_cm / DetailCellSize), 0, 2);

    for (int32 DX = -R; DX <= R; ++DX)
    {
        for (int32 DY = -R; DY <= R; ++DY)
        {
            const FIntVector2 C(Center.X + DX, Center.Y + DY);

            // Does the sphere of radius InjectRadius_cm at WorldPos overlap this cell's AABB?
            const FVector2D CellMin(C.X * DetailCellSize, C.Y * DetailCellSize);
            const FVector2D CellMax((C.X + 1) * DetailCellSize, (C.Y + 1) * DetailCellSize);
            const float ClampedX = FMath::Clamp(WorldPos.X, CellMin.X, CellMax.X);
            const float ClampedY = FMath::Clamp(WorldPos.Y, CellMin.Y, CellMax.Y);
            const float DistSq = FMath::Square(WorldPos.X - ClampedX) + FMath::Square(WorldPos.Y - ClampedY);
            if (DistSq > FMath::Square(InjectRadius_cm))
            {
                continue;
            }

            FDetailCell* Cell = DetailGrid.Find(C);
            if (!Cell || Cell->VehicleRefCount == 0)
            {
                continue;
            }
            if (UNiagaraComponent* NC = EnsureFluidSimForCell(*Cell))
            {
                OutSims.Add(NC);
            }
        }
    }
}
