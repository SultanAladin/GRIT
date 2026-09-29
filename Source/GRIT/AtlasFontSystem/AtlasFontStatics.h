#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AtlasFontTypes.h"
#include "AtlasFontStatics.generated.h"

class UTexture2D;
class UMaterialInterface;

/*====================================================================================================================================
    FAtlasTextParams — everything needed to render atlas text. Put this as a UPROPERTY
    on any class and fill in Atlas + Material + DescriptorJsonPath in the editor.
======================================================================================================================================*/

USTRUCT(BlueprintType)
struct GRIT_API FAtlasTextParams
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AtlasText")
    TObjectPtr<UTexture2D> Atlas = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AtlasText")
    TObjectPtr<UMaterialInterface> Material = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AtlasText",
              meta = (FilePathFilter = "json"))
    FFilePath DescriptorJsonPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AtlasText")
    float Scale = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AtlasText")
    float CharSpacing = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AtlasText")
    float WorldCellSize = 6.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AtlasText")
    FColor Color = FColor::White;

    UPROPERTY(Transient, VisibleAnywhere, Category = "AtlasText")
    FAtlasFontDescriptor Descriptor;

    UPROPERTY(Transient)
    bool bDescriptorLoaded = false;

    bool EnsureDescriptorLoaded();
    bool IsValid() const { return Atlas && Material && bDescriptorLoaded; }
};

/*====================================================================================================================================
    FAtlasTextHandle — opaque handle returned by CreateText, used for Update/Destroy.
======================================================================================================================================*/

USTRUCT(BlueprintType)
struct GRIT_API FAtlasTextHandle
{
    GENERATED_BODY()

    UPROPERTY() int32 LabelId  = INDEX_NONE;
    UPROPERTY() int32 BucketId = INDEX_NONE;

    bool IsValid() const { return LabelId != INDEX_NONE; }
    void Reset() { LabelId = INDEX_NONE; BucketId = INDEX_NONE; }
};

/*====================================================================================================================================
    UAtlasFontStatics — static function library. No subsystem, no component.
    Call CreateText / UpdateText / DestroyText from anywhere.
======================================================================================================================================*/

UCLASS()
class GRIT_API UAtlasFontStatics : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "AtlasText", meta = (WorldContext = "WorldCtx"))
    static FAtlasTextHandle CreateText(const UObject* WorldCtx,
                                       UPARAM(ref) FAtlasTextParams& Params,
                                       const FVector& WorldPos,
                                       const FString& Text);

    UFUNCTION(BlueprintCallable, Category = "AtlasText", meta = (WorldContext = "WorldCtx"))
    static void UpdateText(const UObject* WorldCtx,
                           UPARAM(ref) FAtlasTextHandle& Handle,
                           UPARAM(ref) FAtlasTextParams& Params,
                           const FVector& WorldPos,
                           const FString& Text);

    UFUNCTION(BlueprintCallable, Category = "AtlasText", meta = (WorldContext = "WorldCtx"))
    static void DestroyText(const UObject* WorldCtx,
                            UPARAM(ref) FAtlasTextHandle& Handle);

    UFUNCTION(BlueprintCallable, Category = "AtlasText", meta = (WorldContext = "WorldCtx"))
    static FAtlasTextHandle DrawTextOneFrame(const UObject* WorldCtx,
                                             UPARAM(ref) FAtlasTextParams& Params,
                                             const FVector& WorldPos,
                                             const FString& Text);
};
