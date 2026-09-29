#include "AtlasFontStatics.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Paths.h"

// ============================================================================
// FAtlasTextParams
// ============================================================================

bool FAtlasTextParams::EnsureDescriptorLoaded()
{
    if (bDescriptorLoaded) return true;

    FString Path = DescriptorJsonPath.FilePath;

    // Auto-resolve: derive descriptor JSON path from the atlas texture name.
    // Convention: texture "Archivo-Regular_Atlas" -> "Descriptors/Archivo-Regular_Descriptor.json"
    if (Path.IsEmpty() && Atlas)
    {
        FString TexName = Atlas->GetName();                                    // e.g. "Archivo-Regular_Atlas"
        TexName.RemoveFromEnd(TEXT("_Atlas"));                                 // e.g. "Archivo-Regular"
        Path = FPaths::ProjectContentDir() / TEXT("EngineContent/FontArchives/Descriptors")
             / (TexName + TEXT("_Descriptor.json"));
        if (FPaths::FileExists(Path))
        {
            DescriptorJsonPath.FilePath = Path;
            UE_LOG(LogTemp, Log, TEXT("AtlasFont: Auto-resolved descriptor path from texture '%s' -> %s"), *Atlas->GetName(), *Path);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AtlasFont: Auto-resolve failed - tried '%s' but file not found"), *Path);
            Path.Empty();
        }
    }

    if (Path.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("AtlasFont: DescriptorJsonPath is EMPTY and auto-resolve failed (Atlas=%s)"),
            Atlas ? *Atlas->GetName() : TEXT("NULL"));
        return false;
    }
    if (FPaths::IsRelative(Path))
    {
        Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), Path);
    }
    bDescriptorLoaded = Descriptor.LoadFromJsonFile(Path);
    if (!bDescriptorLoaded)
    {
        UE_LOG(LogTemp, Error, TEXT("AtlasFont: LoadFromJsonFile FAILED for path: %s"), *Path);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("AtlasFont: Descriptor loaded OK - %d glyphs, atlas %dpx, cell %dpx"),
            Descriptor.Glyphs.Num(), Descriptor.AtlasSize, Descriptor.CellPx);
    }
    return bDescriptorLoaded;
}

// ============================================================================
// Internal ISM pool - hidden, per-world singleton via world tag
// ============================================================================

namespace AtlasFontInternal
{

constexpr int32 kCustomFloats = 8;

struct FGlyphSlot
{
    FVector2f PxOffset = FVector2f::ZeroVector;
    bool      bHidden  = false;
};

struct FLabel
{
    int32   Id            = INDEX_NONE;  // Stable monotonic ID; never recycled, unlike StartInstance.
    int32   StartInstance = INDEX_NONE;
    int32   Count         = 0;
    FVector Anchor        = FVector::ZeroVector;
    float   Scale         = 1.f;
    float   WorldCellSize = 6.f;
    int32   CellPx        = 64;
    double  ExpireTime    = -1.0;
    TArray<FGlyphSlot> Slots;
};

struct FBucket
{
    TWeakObjectPtr<UInstancedStaticMeshComponent> ISM;
    TWeakObjectPtr<UMaterialInstanceDynamic>      MID;
    TArray<FLabel>                                Labels;
    TArray<TPair<int32, int32>>                   FreeRanges;
};

struct FWorldPool
{
    TWeakObjectPtr<AActor> Host;
    TWeakObjectPtr<UStaticMesh> QuadMesh;
    TMap<int32, FBucket> Buckets;
    FTSTicker::FDelegateHandle TickHandle;
    int32 NextLabelId = 0;
};

static TMap<uint32, FWorldPool> GWorldPools;

static uint32 WorldKey(UWorld* W) { return GetTypeHash(W); }

static int32 BucketKeyFromMID(UMaterialInterface* Mat, FColor /*Color*/)
{
    // Color is per-instance custom data (slots 4-7), so don't include it in the bucket key.
    // Doing so created stale handles when a label's color changed (RPM White→Yellow→Red):
    // the new key resolved to a new bucket, but Handle.BucketId stayed pinned to the old one.
    return GetTypeHash(Mat);
}

static bool TickPool(float Dt, UWorld* W);

static FWorldPool& GetOrCreatePool(UWorld* W)
{
    const uint32 Key = WorldKey(W);
    if (FWorldPool* P = GWorldPools.Find(Key)) return *P;

    UE_LOG(LogTemp, Log, TEXT("AtlasFont: Creating NEW world pool for world '%s' (key=%u)"), *W->GetName(), Key);

    FWorldPool& P = GWorldPools.Add(Key);
    P.QuadMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));

    if (!P.QuadMesh.IsValid())
        UE_LOG(LogTemp, Error, TEXT("AtlasFont: FAILED to load /Engine/BasicShapes/Plane.Plane - ISM will have no mesh!"));

    FActorSpawnParameters Sp;
    Sp.ObjectFlags |= RF_Transient;
    Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    P.Host = W->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Sp);
    if (P.Host.IsValid())
    {
        P.Host->SetActorLabel(TEXT("AtlasFontPool"));
        auto* Root = NewObject<USceneComponent>(P.Host.Get(), TEXT("Root"));
        Root->RegisterComponent();
        P.Host->SetRootComponent(Root);
        UE_LOG(LogTemp, Log, TEXT("AtlasFont: Pool host actor spawned OK"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AtlasFont: FAILED to spawn pool host actor!"));
    }

    P.TickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda([W](float D) { return TickPool(D, W); }), 0.f);

    return P;
}

static FBucket& GetOrCreateBucket(FWorldPool& Pool, int32 BKey, FAtlasTextParams& Params)
{
    if (FBucket* B = Pool.Buckets.Find(BKey)) return *B;

    FBucket& B = Pool.Buckets.Add(BKey);
    if (!Pool.Host.IsValid() || !Pool.QuadMesh.IsValid()) return B;

    auto* ISM = NewObject<UInstancedStaticMeshComponent>(Pool.Host.Get());
    ISM->SetStaticMesh(Pool.QuadMesh.Get());
    // SetNumCustomDataFloats() is the proper API: it resizes PerInstanceSMCustomData and
    // initializes the internal layout. Direct field assignment leaves the buffer unsized,
    // so the very first AddInstances call after creation produces a per-instance buffer
    // that SetCustomDataValue then writes to at out-of-range slots — silently dropped on
    // instance 0, which is why the first character of every label rendered as garbage.
    ISM->SetNumCustomDataFloats(kCustomFloats);
    ISM->SetMobility(EComponentMobility::Movable);
    ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ISM->SetCastShadow(false);
    ISM->bCastDynamicShadow = false;
    ISM->SetReceivesDecals(false);
    ISM->AttachToComponent(Pool.Host->GetRootComponent(),
                           FAttachmentTransformRules::KeepRelativeTransform);
    ISM->RegisterComponent();
    B.ISM = ISM;

    auto* MID = UMaterialInstanceDynamic::Create(Params.Material, ISM);
    if (MID && Params.Atlas)
    {
        MID->SetTextureParameterValue(TEXT("AtlasTexture"), Params.Atlas);
        const float Sz = FMath::Max(static_cast<float>(Params.Descriptor.AtlasSize), 1.0f);
        MID->SetVectorParameterValue(TEXT("AtlasSize"), FLinearColor(Sz, Sz, 0, 0));
        const float Pr = FMath::Max(static_cast<float>(Params.Descriptor.PxRange), 1.0f);
        MID->SetScalarParameterValue(TEXT("PxRange"), Pr);
        UE_LOG(LogTemp, Log, TEXT("AtlasFont: MID created - AtlasSize=%.0f, PxRange=%.0f, Texture='%s', Material='%s'"),
            Sz, Pr, *Params.Atlas->GetName(), *Params.Material->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AtlasFont: MID creation FAILED - MID=%s, Atlas=%s, Material=%s"),
            MID ? TEXT("OK") : TEXT("NULL"),
            Params.Atlas ? TEXT("SET") : TEXT("NULL"),
            Params.Material ? TEXT("SET") : TEXT("NULL"));
    }
    ISM->SetMaterial(0, MID);
    B.MID = MID;
    return B;
}

static int32 AllocRange(FBucket& B, int32 Count)
{
    if (!B.ISM.IsValid() || Count <= 0) return INDEX_NONE;
    int32 BestIdx = INDEX_NONE, BestSz = MAX_int32;
    for (int32 i = 0; i < B.FreeRanges.Num(); ++i)
    {
        if (B.FreeRanges[i].Value >= Count && B.FreeRanges[i].Value < BestSz)
        { BestIdx = i; BestSz = B.FreeRanges[i].Value; }
    }
    if (BestIdx != INDEX_NONE)
    {
        int32 Start = B.FreeRanges[BestIdx].Key;
        int32 Sz = B.FreeRanges[BestIdx].Value;
        if (Sz == Count) B.FreeRanges.RemoveAtSwap(BestIdx);
        else { B.FreeRanges[BestIdx].Key = Start + Count; B.FreeRanges[BestIdx].Value = Sz - Count; }
        return Start;
    }
    auto* ISM = B.ISM.Get();
    int32 Start = ISM->GetInstanceCount();
    TArray<FTransform> Pad; Pad.Init(FTransform(FVector(0, 0, -1e6)), Count);
    ISM->AddInstances(Pad, false);
    return Start;
}

static void FreeRange(FBucket& B, int32 Start, int32 Count)
{
    if (!B.ISM.IsValid() || Count <= 0) return;
    const FTransform H(FQuat::Identity, FVector(0, 0, -1e6), FVector::OneVector);
    for (int32 i = 0; i < Count; ++i)
        B.ISM->UpdateInstanceTransform(Start + i, H, false, false, true);
    B.ISM->MarkRenderStateDirty();
    B.FreeRanges.Emplace(Start, Count);
}

static void WriteLabel(FBucket& B, FLabel& L, const FString& Text,
                       const FAtlasTextParams& Params, const FRotator& BillRot)
{
    if (!B.ISM.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("AtlasFont::WriteLabel - ISM is INVALID, cannot render"));
        return;
    }
    auto* ISM = B.ISM.Get();
    const auto& Desc = Params.Descriptor;
    const float Cell = static_cast<float>(Desc.CellPx);
    if (Cell <= 0.f)
    {
        UE_LOG(LogTemp, Error, TEXT("AtlasFont::WriteLabel - CellPx is %.0f (must be > 0). Descriptor may be corrupt."), Cell);
        return;
    }

    const FLinearColor Lin = FLinearColor::FromSRGBColor(Params.Color);
    L.CellPx = Desc.CellPx;
    L.WorldCellSize = Params.WorldCellSize;
    L.Scale = Params.Scale;
    // Slots must match L.Count (the allocated instance range), not Text.Len(): the transform
    // loop below iterates L.Count, and a mismatch leaves stale UVs from a longer prior text
    // bleeding into the recycled range. Caller guarantees Text.Len() == L.Count by destroy/recreating
    // on length change, but assert via Min in case that invariant slips.
    const int32 N = FMath::Min(Text.Len(), L.Count);
    L.Slots.SetNum(L.Count);

    // Plane.Plane is centered on its origin; glyph center sits at PenX so PenX=Cell/2 puts the
    // first plane's left edge at the anchor in pixel-space.
    float PenX = Cell * 0.5f;

    // Half-texel UV inset prevents bilinear sampling from picking up neighbor-cell MSDF at the
    // cell boundary, which median3 then reads as a valid distance and shows as speckles.
    const float UvInset = (Desc.AtlasSize > 0) ? (0.5f / static_cast<float>(Desc.AtlasSize)) : 0.f;

    static int32 GlyphLogCount = 0;

    for (int32 i = 0; i < N; ++i)
    {
        auto& S = L.Slots[i];
        const TCHAR Ch = Text[i];
        const FAtlasGlyph* G = Desc.FindGlyph(Ch);
        if (!G)
        {
            if (GlyphLogCount < 3)
            {
                UE_LOG(LogTemp, Warning, TEXT("AtlasFont: Glyph NOT FOUND for char '%c' (0x%04X). Descriptor has %d glyphs."),
                    static_cast<char>(Ch), static_cast<int32>(Ch), Desc.Glyphs.Num());
                ++GlyphLogCount;
            }
            S.bHidden = true; PenX += Cell * 0.5f + Params.CharSpacing; continue;
        }
        if (GlyphLogCount < 3)
        {
            UE_LOG(LogTemp, Log, TEXT("AtlasFont: Glyph '%c' UV=(%.4f,%.4f)-(%.4f,%.4f) CW=%d Adv=%.0f"),
                static_cast<char>(Ch), G->U0, G->V0, G->U1, G->V1, G->ContentWidth, G->Advance);
            ++GlyphLogCount;
        }

        S.PxOffset = FVector2f(PenX, 0.f);
        S.bHidden = false;

        const float U0 = G->U0 + UvInset;
        const float V0 = G->V0 + UvInset;
        const float U1 = G->U1 - UvInset;
        const float V1 = G->V1 - UvInset;

        const int32 Idx = L.StartInstance + i;
        ISM->SetCustomDataValue(Idx, 0, U0,       false);
        ISM->SetCustomDataValue(Idx, 1, V0,       false);
        ISM->SetCustomDataValue(Idx, 2, U1 - U0,  false);
        ISM->SetCustomDataValue(Idx, 3, V1 - V0,  false);
        ISM->SetCustomDataValue(Idx, 4, Lin.R, false);
        ISM->SetCustomDataValue(Idx, 5, Lin.G, false);
        ISM->SetCustomDataValue(Idx, 6, Lin.B, false);
        ISM->SetCustomDataValue(Idx, 7, Lin.A, false);

        PenX += (G->ContentWidth > 0 ? static_cast<float>(G->ContentWidth) : Cell * 0.5f)
              + Params.CharSpacing;
    }
    // Hide any allocated slots not consumed by the current text (defensive: should never fire
    // given the destroy/recreate-on-length-change rule in UpdateText).
    for (int32 i = N; i < L.Count; ++i)
    {
        L.Slots[i].bHidden = true;
        L.Slots[i].PxOffset = FVector2f::ZeroVector;
    }

    const float PxToWorld = (Params.WorldCellSize / Cell) * Params.Scale;
    // Plane.Plane normal is +Z. Rotate +90 around Y so normal faces -X (toward camera after billboard).
    // Then billboard rotation maps: local -X -> -CamForward, local +Y -> CamRight, local +Z -> CamUp.
    // Corrected rotations: 
    // 1. -90 on Y to face camera (+Z normal -> -X normal)
    // 2. 90 on local Up (Z) to spin characters on the plane surface
    const FQuat PlaneCorrection = FQuat(FVector::RightVector, FMath::DegreesToRadians(-90.f));
    const FQuat Glyph90Correction = FQuat(FVector::UpVector, FMath::DegreesToRadians(90.f));
    
    const FQuat BillQ = BillRot.Quaternion();
    const FQuat Q = BillQ * PlaneCorrection * Glyph90Correction;
    const FVector Right = BillQ.RotateVector(FVector::RightVector);
    const FTransform Hidden(FQuat::Identity, FVector(0, 0, -1e6), FVector::OneVector);

    static int32 TransformLogCount = 0;
    if (TransformLogCount < 3)
    {
        UE_LOG(LogTemp, Log, TEXT("AtlasFont: WriteLabel - PxToWorld=%.4f, CellPx=%.0f, WorldCellSize=%.1f, Scale=%.2f, Anchor=(%.0f,%.0f,%.0f), ISM instances=%d"),
            PxToWorld, Cell, Params.WorldCellSize, Params.Scale, L.Anchor.X, L.Anchor.Y, L.Anchor.Z, ISM->GetInstanceCount());
        ++TransformLogCount;
    }

    for (int32 i = 0; i < L.Count; ++i)
    {
        const auto& S = L.Slots[i];
        if (S.bHidden) { ISM->UpdateInstanceTransform(L.StartInstance + i, Hidden, true, false, true); continue; }
        FVector Pos = L.Anchor + Right * (S.PxOffset.X * PxToWorld);
        FTransform Xf; Xf.SetLocation(Pos); Xf.SetRotation(Q);
        Xf.SetScale3D(FVector(PxToWorld));
        ISM->UpdateInstanceTransform(L.StartInstance + i, Xf, true, false, true);
    }
    ISM->MarkRenderStateDirty();
}

static void BillboardLabel(FBucket& B, const FLabel& L, const FRotator& BillRot)
{
    if (!B.ISM.IsValid()) return;
    auto* ISM = B.ISM.Get();
    const float Cell = static_cast<float>(L.CellPx);
    if (Cell <= 0.f) return;
    const float PxToWorld = (L.WorldCellSize / Cell) * L.Scale;
    const FQuat PlaneCorrection = FQuat(FVector::RightVector, FMath::DegreesToRadians(-90.f));
    const FQuat Glyph90Correction = FQuat(FVector::UpVector, FMath::DegreesToRadians(90.f));
    
    const FQuat BillQ = BillRot.Quaternion();
    const FQuat Q = BillQ * PlaneCorrection * Glyph90Correction;
    const FVector Right = BillQ.RotateVector(FVector::RightVector);
    const FTransform Hidden(FQuat::Identity, FVector(0, 0, -1e6), FVector::OneVector);

    for (int32 i = 0; i < L.Count; ++i)
    {
        const auto& S = L.Slots[i];
        if (S.bHidden) { ISM->UpdateInstanceTransform(L.StartInstance + i, Hidden, true, false, true); continue; }
        FVector Pos = L.Anchor + Right * (S.PxOffset.X * PxToWorld);
        FTransform Xf; Xf.SetLocation(Pos); Xf.SetRotation(Q); Xf.SetScale3D(FVector(PxToWorld));
        ISM->UpdateInstanceTransform(L.StartInstance + i, Xf, true, false, true);
    }
    ISM->MarkRenderStateDirty();
}

static FRotator GetCameraRot(UWorld* W)
{
    if (auto* PC = W->GetFirstPlayerController())
    {
        FVector Loc; FRotator Rot;
        PC->GetPlayerViewPoint(Loc, Rot);
        return FRotator(Rot.Pitch, Rot.Yaw, 0.f);
    }
    return FRotator::ZeroRotator;
}

static bool TickPool(float, UWorld* W)
{
    if (!W || !GEngine) return false;

    // Validate the world is still alive — the captured pointer can dangle after PIE ends.
    bool bWorldAlive = false;
    for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
    {
        if (Ctx.World() == W) { bWorldAlive = true; break; }
    }
    if (!bWorldAlive)
    {
        const uint32 Key = WorldKey(W);
        GWorldPools.Remove(Key);
        return false;
    }

    const uint32 Key = WorldKey(W);
    FWorldPool* Pool = GWorldPools.Find(Key);
    if (!Pool) return false;
    if (!Pool->Host.IsValid())
    {
        GWorldPools.Remove(Key);
        return false;
    }

    const FRotator Bill = GetCameraRot(W);
    const double Now = FPlatformTime::Seconds();

    for (auto& Pair : Pool->Buckets)
    {
        FBucket& B = Pair.Value;
        if (!B.ISM.IsValid()) continue;
        for (int32 i = B.Labels.Num() - 1; i >= 0; --i)
        {
            FLabel& L = B.Labels[i];
            if (L.ExpireTime >= 0.0 && Now >= L.ExpireTime)
            {
                FreeRange(B, L.StartInstance, L.Count);
                B.Labels.RemoveAtSwap(i);
            }
        }
        for (const FLabel& L : B.Labels)
            BillboardLabel(B, L, Bill);
    }
    return true;
}

static FLabel* FindLabel(FWorldPool& Pool, int32 BucketId, int32 LabelId)
{
    FBucket* B = Pool.Buckets.Find(BucketId);
    if (!B) return nullptr;
    for (auto& L : B->Labels)
        if (L.Id == LabelId) return &L;
    return nullptr;
}

} // namespace AtlasFontInternal

// ============================================================================
// Public API
// ============================================================================

FAtlasTextHandle UAtlasFontStatics::CreateText(const UObject* WorldCtx,
                                               FAtlasTextParams& Params,
                                               const FVector& WorldPos,
                                               const FString& Text)
{
    FAtlasTextHandle H;
    if (Text.IsEmpty() || !Params.EnsureDescriptorLoaded() || !Params.IsValid()) return H;

    UWorld* W = GEngine ? GEngine->GetWorldFromContextObject(WorldCtx,
                                                             EGetWorldErrorMode::LogAndReturnNull) : nullptr;
    if (!W) return H;

    using namespace AtlasFontInternal;
    auto& Pool = GetOrCreatePool(W);
    const int32 BKey = BucketKeyFromMID(Params.Material, Params.Color);
    FBucket& B = GetOrCreateBucket(Pool, BKey, Params);
    const int32 Count = Text.Len();
    const int32 Start = AllocRange(B, Count);
    if (Start == INDEX_NONE) return H;

    const int32 NewIdx = B.Labels.AddDefaulted();
    FLabel& L = B.Labels[NewIdx];
    L.Id = Pool.NextLabelId++;
    L.StartInstance = Start;
    L.Count = Count;
    L.Anchor = WorldPos;
    L.ExpireTime = -1.0;

    WriteLabel(B, L, Text, Params, GetCameraRot(W));

    H.LabelId = L.Id;
    H.BucketId = BKey;
    return H;
}

void UAtlasFontStatics::UpdateText(const UObject* WorldCtx,
                                   FAtlasTextHandle& Handle,
                                   FAtlasTextParams& Params,
                                   const FVector& WorldPos,
                                   const FString& Text)
{
    if (!Handle.IsValid()) { Handle = CreateText(WorldCtx, Params, WorldPos, Text); return; }

    UWorld* W = GEngine ? GEngine->GetWorldFromContextObject(WorldCtx,
                                                             EGetWorldErrorMode::LogAndReturnNull) : nullptr;
    if (!W) return;

    using namespace AtlasFontInternal;
    FWorldPool* Pool = GWorldPools.Find(WorldKey(W));
    if (!Pool) { Handle.Reset(); Handle = CreateText(WorldCtx, Params, WorldPos, Text); return; }

    FBucket* B = Pool->Buckets.Find(Handle.BucketId);
    FLabel* L = B ? FindLabel(*Pool, Handle.BucketId, Handle.LabelId) : nullptr;

    if (!L || L->Count != Text.Len())
    {
        DestroyText(WorldCtx, Handle);
        Handle = CreateText(WorldCtx, Params, WorldPos, Text);
        return;
    }

    L->Anchor = WorldPos;
    Params.EnsureDescriptorLoaded();
    WriteLabel(*B, *L, Text, Params, GetCameraRot(W));
}

void UAtlasFontStatics::DestroyText(const UObject* WorldCtx,
                                    FAtlasTextHandle& Handle)
{
    if (!Handle.IsValid()) return;
    UWorld* W = GEngine ? GEngine->GetWorldFromContextObject(WorldCtx,
                                                             EGetWorldErrorMode::LogAndReturnNull) : nullptr;
    if (!W) { Handle.Reset(); return; }

    using namespace AtlasFontInternal;
    FWorldPool* Pool = GWorldPools.Find(WorldKey(W));
    if (!Pool) { Handle.Reset(); return; }

    FBucket* B = Pool->Buckets.Find(Handle.BucketId);
    if (B)
    {
        for (int32 i = 0; i < B->Labels.Num(); ++i)
        {
            if (B->Labels[i].Id == Handle.LabelId)
            {
                FreeRange(*B, B->Labels[i].StartInstance, B->Labels[i].Count);
                B->Labels.RemoveAtSwap(i);
                break;
            }
        }
    }
    Handle.Reset();
}

FAtlasTextHandle UAtlasFontStatics::DrawTextOneFrame(const UObject* WorldCtx,
                                                     FAtlasTextParams& Params,
                                                     const FVector& WorldPos,
                                                     const FString& Text)
{
    FAtlasTextHandle H;
    if (Text.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("AtlasFont::DrawTextOneFrame - Text is empty"));
        return H;
    }
    if (!Params.EnsureDescriptorLoaded())
    {
        UE_LOG(LogTemp, Warning, TEXT("AtlasFont::DrawTextOneFrame - EnsureDescriptorLoaded failed (Atlas=%s, Mat=%s, JsonPath='%s')"),
            Params.Atlas ? TEXT("SET") : TEXT("NULL"),
            Params.Material ? TEXT("SET") : TEXT("NULL"),
            *Params.DescriptorJsonPath.FilePath);
        return H;
    }
    if (!Params.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("AtlasFont::DrawTextOneFrame - IsValid false (Atlas=%s, Mat=%s, DescLoaded=%d)"),
            Params.Atlas ? TEXT("SET") : TEXT("NULL"),
            Params.Material ? TEXT("SET") : TEXT("NULL"),
            Params.bDescriptorLoaded);
        return H;
    }

    UWorld* W = GEngine ? GEngine->GetWorldFromContextObject(WorldCtx,
                                                             EGetWorldErrorMode::LogAndReturnNull) : nullptr;
    if (!W)
    {
        UE_LOG(LogTemp, Error, TEXT("AtlasFont::DrawTextOneFrame - World is NULL (GEngine=%s, WorldCtx=%s)"),
            GEngine ? TEXT("OK") : TEXT("NULL"),
            WorldCtx ? *WorldCtx->GetName() : TEXT("NULL"));
        return H;
    }

    using namespace AtlasFontInternal;
    auto& Pool = GetOrCreatePool(W);
    const int32 BKey = BucketKeyFromMID(Params.Material, Params.Color);
    FBucket& B = GetOrCreateBucket(Pool, BKey, Params);
    const int32 Count = Text.Len();
    const int32 Start = AllocRange(B, Count);
    if (Start == INDEX_NONE)
    {
        UE_LOG(LogTemp, Error, TEXT("AtlasFont::DrawTextOneFrame - AllocRange returned INDEX_NONE for %d instances"), Count);
        return H;
    }

    const int32 NewIdx = B.Labels.AddDefaulted();
    FLabel& L = B.Labels[NewIdx];
    L.Id = Pool.NextLabelId++;
    L.StartInstance = Start;
    L.Count = Count;
    L.Anchor = WorldPos;
    L.ExpireTime = FPlatformTime::Seconds() + 0.05;

    WriteLabel(B, L, Text, Params, GetCameraRot(W));

    H.LabelId = L.Id;
    H.BucketId = BKey;

    static int32 SuccessLogCount = 0;
    if (SuccessLogCount < 5)
    {
        UE_LOG(LogTemp, Log, TEXT("AtlasFont: DrawTextOneFrame SUCCESS - '%s' at (%.0f,%.0f,%.0f), %d instances, ISM=%s, BKey=%d"),
            *Text, WorldPos.X, WorldPos.Y, WorldPos.Z, Count,
            B.ISM.IsValid() ? TEXT("Valid") : TEXT("INVALID"), BKey);
        ++SuccessLogCount;
    }
    return H;
}
