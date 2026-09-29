#pragma once

#include "CoreMinimal.h"
#include "AtlasFontTypes.generated.h"

/*====================================================================================================================================
                                                    ATLAS FONT GLYPH DESCRIPTOR
======================================================================================================================================*/

/** Per-glyph entry parsed from <Font>_Descriptor.json. UV is the full uniform cell.
 *  Content rect is the glyph ink bbox inside the cell, in cell-local pixels. */
USTRUCT(BlueprintType)
struct FAtlasGlyph
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float U0 = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float V0 = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float U1 = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float V1 = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 ContentLeft   = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 ContentTop    = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 ContentWidth  = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 ContentHeight = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Advance = 0.f;
};

/*====================================================================================================================================
                                                    ATLAS FONT DESCRIPTOR
======================================================================================================================================*/

USTRUCT(BlueprintType)
struct FAtlasFontDescriptor
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString FontName;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString AtlasType;       // "msdf" / "mtsdf"
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32   AtlasSize  = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32   PxRange    = 4;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32   CellPx     = 64;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32   CellWidth  = 64;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32   CellHeight = 64;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32   GlyphSize  = 48;

    /** Keyed by single character (TCHAR cast to int32) for fast lookup. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TMap<int32, FAtlasGlyph> Glyphs;

    bool LoadFromJsonFile(const FString& AbsoluteJsonPath);
    bool LoadFromJsonString(const FString& JsonText);

    const FAtlasGlyph* FindGlyph(TCHAR Ch) const
    {
        return Glyphs.Find(static_cast<int32>(Ch));
    }
};
