#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpatialGridSubsystem.generated.h"

class ALandscapeProxy;
class URuntimeVirtualTextureComponent;
class UNiagaraComponent;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnCellEntered, AActor* /*Pawn*/, FIntVector2 /*Cell*/, int32 /*GridLevel*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnCellExited,  AActor* /*Pawn*/, FIntVector2 /*Cell*/, int32 /*GridLevel*/);

//------------------------------------------------------------------------------
//                          SPATIAL TIER
//------------------------------------------------------------------------------

/** Importance tier for grid-managed actors. Higher tier = larger, more persistent objects. */
UENUM(BlueprintType)
enum class ESpatialTier : uint8
{
    Tier1       UMETA(DisplayName = "Tier 1 (small clutter: rocks/twigs/plants)"),
    Tier2       UMETA(DisplayName = "Tier 2 (medium: trees, boulders)"),
    Tier3       UMETA(DisplayName = "Tier 3 (large: terrain, structures) [disabled]"),
};

//------------------------------------------------------------------------------
//                          SPATIAL CELL (base)
//------------------------------------------------------------------------------

/** Sparse cell — only exists if actors registered into it */
USTRUCT()
struct FSpatialCell
{
    GENERATED_BODY()

    FIntVector2 Coord = FIntVector2(0, 0);
    FBox2D WorldBounds = FBox2D(FVector2D::ZeroVector, FVector2D::ZeroVector);
    TArray<TWeakObjectPtr<AActor>> Actors;
};

//------------------------------------------------------------------------------
//                          LEVEL 1: LANDSCAPE ENTRY
//------------------------------------------------------------------------------

/** Landscape-based streaming entry (no grid — registered by reference) */
USTRUCT()
struct FLandscapeStreamingEntry
{
    GENERATED_BODY()

    UPROPERTY()
    TSoftObjectPtr<UWorld> LevelAsset;

    UPROPERTY()
    TWeakObjectPtr<ALandscapeProxy> Landscape;

    UPROPERTY()
    TWeakObjectPtr<URuntimeVirtualTextureComponent> RVTComponent;

    /** World-space bounds of this landscape */
    FBox WorldBounds = FBox(ForceInit);

    /** Current streaming state: 0=unloaded, 1=loading, 2=loaded */
    uint8 StreamState = 0;
};

//------------------------------------------------------------------------------
//                          LEVEL 2: REGION CELL
//------------------------------------------------------------------------------

/** 200m region cell — VFX LOD + collision culling */
USTRUCT()
struct FRegionCell : public FSpatialCell
{
    GENERATED_BODY()

    /** VFX quality multiplier: 0.0=off, 0.5=reduced, 1.0=full */
    float VFXQualityScale = 0.0f;

    /** Whether collision is enabled for actors in this cell */
    bool bCollisionEnabled = false;

    /** Distance in cells from the pawn (recalculated each L2 update) */
    int32 CellDistance = MAX_int32;

    /** Per-tier registered actor buckets (for collision toggling on cell transitions) */
    TArray<TWeakObjectPtr<AActor>> Tier1Actors;
    TArray<TWeakObjectPtr<AActor>> Tier2Actors;
    // TArray<TWeakObjectPtr<AActor>> Tier3Actors;  // disabled per design — large objects stay always-on
};

//------------------------------------------------------------------------------
//                          LEVEL 3: DETAIL CELL
//------------------------------------------------------------------------------

/** 50m detail cell — full fidelity, Niagara active, RVT writes */
USTRUCT()
struct FDetailCell : public FSpatialCell
{
    GENERATED_BODY()

    /** RVT for this cell's landscape (looked up from L1) */
    TWeakObjectPtr<URuntimeVirtualTextureComponent> RVTComponent;

    /** Whether Niagara effects should be active in this cell */
    bool bNiagaraActive = false;

    /** Per-cell fluid sim, shared by any vehicles inside this cell */
    UPROPERTY()
    TWeakObjectPtr<UNiagaraComponent> FluidSim;

    /** Number of vehicles currently inside this cell (drives FluidSim lifecycle) */
    int32 VehicleRefCount = 0;

    /** Countdown (seconds) before destroying an empty cell's FluidSim (lets existing particles fade) */
    float FadeOutRemaining = 0.0f;
};

//------------------------------------------------------------------------------
//                          VFX QUALITY LEVEL
//------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ESpatialVFXQuality : uint8
{
    Off       = 0   UMETA(DisplayName = "Off"),
    Low       = 1   UMETA(DisplayName = "Low"),
    Medium    = 2   UMETA(DisplayName = "Medium"),
    High      = 3   UMETA(DisplayName = "High"),
};

//------------------------------------------------------------------------------
//                          SPATIAL GRID SUBSYSTEM
//------------------------------------------------------------------------------

/**
 * 3-level spatial management subsystem.
 *
 * Level 1: Landscape streaming (registered, not gridded)
 * Level 2: Region grid (200m sparse hash) — VFX LOD, collision culling
 * Level 3: Detail grid (50m sparse hash) — full VFX, RVT writes
 *
 * Driven by a primary actor (vehicle/pawn). Cells only exist where actors are registered.
 */
UCLASS()
class GRIT_API USpatialGridSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    //--------------------------------------------------------------------------
    //                          LIFECYCLE
    //--------------------------------------------------------------------------
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickableInEditor() const override { return false; }

    //--------------------------------------------------------------------------
    //                          REGISTRATION
    //--------------------------------------------------------------------------

    /** Register an actor into the region (L2) and detail (L3) grids */
    void RegisterActor(AActor* Actor);

    /** Remove an actor from all grid levels */
    void UnregisterActor(AActor* Actor);

    /** Register an actor into its tier bucket for collision management */
    void RegisterTieredActor(AActor* Actor, ESpatialTier Tier);

    /** Remove a tiered actor (no-op if not registered) */
    void UnregisterTieredActor(AActor* Actor, ESpatialTier Tier);

    /** Register a landscape with optional RVT and level asset for streaming */
    void RegisterLandscape(ALandscapeProxy* Landscape,
                           URuntimeVirtualTextureComponent* RVT = nullptr,
                           TSoftObjectPtr<UWorld> LevelAsset = nullptr);

    /** Unregister a landscape */
    void UnregisterLandscape(ALandscapeProxy* Landscape);

    //--------------------------------------------------------------------------
    //                          QUERIES
    //--------------------------------------------------------------------------

    /** Get VFX quality scale at a world position (0.0 - 1.0) */
    float GetVFXQualityAt(const FVector& WorldPos) const;

    /**
     * Get the discrete VFX resolution for volumetric systems (Niagara fluids).
     * Ladder: 0 (off) / 25 / 50 / 100 / 200. Capped at 250.
     * Maps from Chebyshev cell distance to pawn.
     */
    int32 GetVFXResolutionAt(const FVector& WorldPos) const;

    /** Get the ESpatialVFXQuality bucket at a world position */
    ESpatialVFXQuality GetVFXQualityBucketAt(const FVector& WorldPos) const;

    /** Get the RVT component for a world position (from landscape lookup) */
    URuntimeVirtualTextureComponent* GetRVTAt(const FVector& WorldPos) const;

    /** Is this position inside an active detail cell? */
    bool IsInDetailZone(const FVector& WorldPos) const;

    /**
     * Gather the shared FluidSim components for every detail cell this position overlaps
     * (within InjectRadius_cm). Creates the sim on first demand.
     */
    void GetFluidSimsForPosition(const FVector& WorldPos, float InjectRadius_cm, TArray<UNiagaraComponent*>& OutSims);

    /** Get the landscape streaming entry that contains this position */
    FLandscapeStreamingEntry* GetLandscapeAt(const FVector& WorldPos);

    //--------------------------------------------------------------------------
    //                          PRIMARY ACTOR
    //--------------------------------------------------------------------------

    /** Set the primary actor that drives grid updates (usually the player vehicle) */
    void SetPrimaryActor(AActor* Actor);

    UPROPERTY()
    TWeakObjectPtr<AActor> PrimaryActor;

    /** Register a vehicle pawn for per-frame cell tracking (drives OnCellEntered/OnCellExited) */
    void RegisterVehicle(AActor* Vehicle);

    /** Stop tracking a vehicle */
    void UnregisterVehicle(AActor* Vehicle);

    //--------------------------------------------------------------------------
    //                          CELL-TRANSITION DELEGATES
    //--------------------------------------------------------------------------

    /** Fired when a tracked vehicle enters a new cell at the given grid level (2 = region, 3 = detail) */
    FOnCellEntered OnCellEntered;

    /** Fired when a tracked vehicle leaves a cell */
    FOnCellExited OnCellExited;

    //--------------------------------------------------------------------------
    //                          CONFIGURATION
    //--------------------------------------------------------------------------

    /** Level 2 cell size in cm */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial Grid|Config")
    float RegionCellSize = 20000.0f;                                          // 200m

    /** Level 3 cell size in cm */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial Grid|Config")
    float DetailCellSize = 5000.0f;                                           // 50m

    /** L2 cell distance at which VFX is reduced */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial Grid|Config")
    int32 RegionVFXReducedRange = 1;

    /** L2 cell distance at which VFX is off and collision disabled */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial Grid|Config")
    int32 RegionCullRange = 3;

    /** L3 cell radius around pawn (1 = 3x3 grid of active cells) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial Grid|Config")
    int32 DetailActiveRadius = 1;

    /**
     * Hard cap on fluid-sim resolution (increments of 25). Default 100 for GTX 1060.
     * Closest cell gets MaxFluidResolution; each further cell drops by 25 until 25, then off.
     * e.g. Max=100 → distance 0→100, 1→75, 2→50, 3→25, 4+→0.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial Grid|Config", meta = (ClampMin = "25", ClampMax = "250", Multiple = "25"))
    int32 MaxFluidResolution = 100;

    /**
     * Distance thresholds (cm) for each resolution step. Resolution decreases as camera moves away.
     * Dist 0 → 0..VFXDistanceBands[0] → MaxFluidResolution
     * Dist 1 → band[0]..band[1]       → MaxFluidResolution - 25
     * ...until 25, then off.
     * Default: {2000, 5000, 10000, 50000} = 0-20m full, 20-50m -25, 50-100m -50, 100-500m -75, >500m off.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial Grid|Config")
    TArray<float> VFXDistanceBands_cm = { 2000.0f, 5000.0f, 10000.0f, 50000.0f };

    /** Niagara fluid-sim asset spawned per active detail cell (shared by all vehicles in cell) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial Grid|VFX")
    TObjectPtr<class UNiagaraSystem> CellFluidSystem = nullptr;

    /** Seconds to keep an empty cell's FluidSim alive before destroying it (lets particles dissipate) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial Grid|VFX", meta = (ClampMin = "0.0"))
    float EmptyCellFadeOutSeconds = 3.0f;

    //--------------------------------------------------------------------------
    //                          DEBUG
    //--------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial Grid|Debug")
    bool bDrawDebugLevel1 = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial Grid|Debug")
    bool bDrawDebugLevel2 = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial Grid|Debug")
    bool bDrawDebugLevel3 = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial Grid|Debug")
    float DebugBoxHeight = 500.0f;                                            // [cm]

private:
    //--------------------------------------------------------------------------
    //                          GRID DATA
    //--------------------------------------------------------------------------

    /** Level 1: landscape registry (not gridded) */
    TMap<TWeakObjectPtr<ALandscapeProxy>, FLandscapeStreamingEntry> LandscapeRegistry;

    /** Level 2: sparse region grid (200m cells) */
    TMap<FIntVector2, FRegionCell> RegionGrid;

    /** Level 3: sparse detail grid (50m cells) */
    TMap<FIntVector2, FDetailCell> DetailGrid;

    /** Reverse lookup: actor -> which L2 cell it's in */
    TMap<TWeakObjectPtr<AActor>, FIntVector2> ActorToRegionCell;

    /** Registered vehicles driving cell-transition events */
    TArray<TWeakObjectPtr<AActor>> RegisteredVehicles;

    /** Per-vehicle last known region (L2) cell */
    TMap<TWeakObjectPtr<AActor>, FIntVector2> VehicleLastRegionCell;

    /** Per-vehicle last known detail (L3) cell */
    TMap<TWeakObjectPtr<AActor>, FIntVector2> VehicleLastDetailCell;

    //--------------------------------------------------------------------------
    //                          TICK COUNTERS
    //--------------------------------------------------------------------------
    int32 TickCounter = 0;

    //--------------------------------------------------------------------------
    //                          INTERNAL
    //--------------------------------------------------------------------------

    /** Convert world position to cell coordinate */
    FIntVector2 WorldToCell(const FVector& WorldPos, float CellSize) const;

    /** Get cell center in world space */
    FVector CellToWorld(const FIntVector2& Coord, float CellSize) const;

    /** Get cell AABB in world space */
    FBox CellWorldBounds(const FIntVector2& Coord, float CellSize) const;

    /** Update Level 2 region grid (called at ~5Hz) */
    void UpdateRegionGrid(const FVector& PawnLocation);

    /** Update Level 3 detail grid (called every tick) */
    void UpdateDetailGrid(const FVector& PawnLocation);

    /** Update Level 1 landscape streaming (called at ~0.5Hz) */
    void UpdateLandscapeStreaming(const FVector& PawnLocation);

    /** Apply collision state to all tiered actors in a region cell (3-state: none/query/physics) */
    void ApplyCellCollision(FRegionCell& Cell);

    /** Compute collision state for a cell from its distance to the nearest vehicle */
    ECollisionEnabled::Type DesiredCollisionForDistance(int32 CellDistance) const;

    /** Track cell transitions for all registered vehicles and fire delegates */
    void UpdateVehicleCellTransitions();

    /** Spawn / destroy per-cell fluid sims based on ref counts and fade-out timers */
    void TickDetailFluidSims(float DeltaTime);

    /** Internal cell-transition handlers (ref-count FluidSim) */
    void HandleCellEntered(AActor* Vehicle, FIntVector2 Cell, int32 GridLevel);
    void HandleCellExited(AActor* Vehicle, FIntVector2 Cell, int32 GridLevel);

    /** Ensure a detail cell has a spawned FluidSim (returns existing if already spawned) */
    UNiagaraComponent* EnsureFluidSimForCell(FDetailCell& Cell);

    /** Draw debug visualization */
    void DrawDebug(const FVector& PawnLocation);
};
