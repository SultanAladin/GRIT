#pragma once

#include "CoreMinimal.h"
#include "Engine/Font.h"
#include "UObject/NoExportTypes.h"
#include "ThemeConfig.generated.h"

/*====================================================================================================================================
                                                         THEME CONFIGURATION
======================================================================================================================================*/

//------------------------------------------------------------------------------
//                          color palette
//------------------------------------------------------------------------------

/** Core color system for all UI elements */
USTRUCT(BlueprintType)
struct GRIT_API FPalette
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
    FLinearColor SurfacePrime;          // [RGBA] - Primary surface background

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
    FLinearColor SurfaceShift;          // [RGBA] - Secondary surface variant

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
    FLinearColor SurfaceRaised;         // [RGBA] - Elevated surface layer

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
    FLinearColor SurfaceInset;          // [RGBA] - Recessed surface depth

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    FLinearColor TextPrime;             // [RGBA] - Primary text content

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    FLinearColor TextShift;             // [RGBA] - Secondary text content

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    FLinearColor TextMute;              // [RGBA] - Tertiary muted text

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    FLinearColor TextOnAccent;          // [RGBA] - Text over accent surfaces

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accent")
    FLinearColor AccentCore;            // [RGBA] - Primary brand accent

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accent")
    FLinearColor AccentSharp;           // [RGBA] - High contrast accent

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accent")
    FLinearColor AccentSoft;            // [RGBA] - Subdued accent tone

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
    FLinearColor StateHover;            // [RGBA] - Interactive hover overlay

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
    FLinearColor StateActive;           // [RGBA] - Active selection state

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
    FLinearColor StateDisable;          // [RGBA] - Disabled element tint

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Semantic")
    FLinearColor SemanticCrit;          // [RGBA] - Critical error signal

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Semantic")
    FLinearColor SemanticWarn;          // [RGBA] - Warning notification

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Semantic")
    FLinearColor SemanticPass;          // [RGBA] - Success confirmation

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Semantic")
    FLinearColor SemanticInfo;          // [RGBA] - Information display

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Line")
    FLinearColor LinePrime;             // [RGBA] - Primary border/divider

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Line")
    FLinearColor LineShift;             // [RGBA] - Subtle border variant

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Line")
    FLinearColor LineFocus;             // [RGBA] - Focused element outline

    FPalette()
        : SurfacePrime(FLinearColor(0.02f, 0.02f, 0.02f, 1.0f))
        , SurfaceShift(FLinearColor(0.08f, 0.08f, 0.08f, 1.0f))
        , SurfaceRaised(FLinearColor(0.12f, 0.12f, 0.12f, 1.0f))
        , SurfaceInset(FLinearColor(0.01f, 0.01f, 0.01f, 1.0f))
        , TextPrime(FLinearColor(0.95f, 0.95f, 0.95f, 1.0f))
        , TextShift(FLinearColor(0.70f, 0.70f, 0.70f, 1.0f))
        , TextMute(FLinearColor(0.45f, 0.45f, 0.45f, 1.0f))
        , TextOnAccent(FLinearColor(0.05f, 0.05f, 0.05f, 1.0f))
        , AccentCore(FLinearColor(0.10f, 0.60f, 1.00f, 1.0f))
        , AccentSharp(FLinearColor(0.20f, 0.70f, 1.00f, 1.0f))
        , AccentSoft(FLinearColor(0.05f, 0.50f, 0.90f, 1.0f))
        , StateHover(FLinearColor(1.00f, 1.00f, 1.00f, 0.08f))
        , StateActive(FLinearColor(1.00f, 1.00f, 1.00f, 0.12f))
        , StateDisable(FLinearColor(0.30f, 0.30f, 0.30f, 0.50f))
        , SemanticCrit(FLinearColor(1.00f, 0.20f, 0.20f, 1.0f))
        , SemanticWarn(FLinearColor(1.00f, 0.65f, 0.00f, 1.0f))
        , SemanticPass(FLinearColor(0.20f, 0.80f, 0.20f, 1.0f))
        , SemanticInfo(FLinearColor(0.20f, 0.60f, 1.00f, 1.0f))
        , LinePrime(FLinearColor(0.20f, 0.20f, 0.20f, 1.0f))
        , LineShift(FLinearColor(0.15f, 0.15f, 0.15f, 1.0f))
        , LineFocus(FLinearColor(0.10f, 0.60f, 1.00f, 1.0f))
    {}
};

//------------------------------------------------------------------------------
//                          typography scale
//------------------------------------------------------------------------------

/** Font weight specification */
UENUM(BlueprintType)
enum class EFontWeight : uint8
{
    Thin    = 0     UMETA(DisplayName = "Thin"),
    Light   = 1     UMETA(DisplayName = "Light"),
    Regular = 2     UMETA(DisplayName = "Regular"),
    Medium  = 3     UMETA(DisplayName = "Medium"),
    Bold    = 4     UMETA(DisplayName = "Bold"),
    Black   = 5     UMETA(DisplayName = "Black")
};

/** Single typography spec for text rendering */
USTRUCT(BlueprintType)
struct GRIT_API FTypeSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
    UFont* FontAsset;                   // [UFont*] - Font family asset

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
    int32 Size;                         // [px] - Font size in pixels

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
    EFontWeight Weight;                 // [enum] - Font weight variant

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
    float LineHeight;                   // [ratio] - Line height multiplier

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
    float LetterSpace;                  // [px] - Character spacing offset

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
    FLinearColor Color;                 // [RGBA] - Default text color

    FTypeSpec()
        : FontAsset(nullptr)
        , Size(14)
        , Weight(EFontWeight::Regular)
        , LineHeight(1.5f)
        , LetterSpace(0.0f)
        , Color(FLinearColor::White)
    {}

    FTypeSpec(int32 InSize, EFontWeight InWeight, float InLineHeight, FLinearColor InColor)
        : FontAsset(nullptr)
        , Size(InSize)
        , Weight(InWeight)
        , LineHeight(InLineHeight)
        , LetterSpace(0.0f)
        , Color(InColor)
    {}
};

/** Complete typography system for all text roles */
USTRUCT(BlueprintType)
struct GRIT_API FTypeScale
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FTypeSpec DisplayXL;                // [FTypeSpec] - Extra large display

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FTypeSpec DisplayL;                 // [FTypeSpec] - Large display

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FTypeSpec DisplayM;                 // [FTypeSpec] - Medium display

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heading")
    FTypeSpec HeadingXL;                // [FTypeSpec] - Extra large heading

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heading")
    FTypeSpec HeadingL;                 // [FTypeSpec] - Large heading

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heading")
    FTypeSpec HeadingM;                 // [FTypeSpec] - Medium heading

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heading")
    FTypeSpec HeadingS;                 // [FTypeSpec] - Small heading

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body")
    FTypeSpec BodyL;                    // [FTypeSpec] - Large body text

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body")
    FTypeSpec BodyM;                    // [FTypeSpec] - Medium body text

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body")
    FTypeSpec BodyS;                    // [FTypeSpec] - Small body text

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Label")
    FTypeSpec LabelM;                   // [FTypeSpec] - Medium label

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Label")
    FTypeSpec LabelS;                   // [FTypeSpec] - Small label

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Label")
    FTypeSpec CaptionM;                 // [FTypeSpec] - Medium caption

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Label")
    FTypeSpec CaptionS;                 // [FTypeSpec] - Small caption

    FTypeScale()
        : DisplayXL(48, EFontWeight::Bold, 1.1f, FLinearColor::White)
        , DisplayL(36, EFontWeight::Bold, 1.1f, FLinearColor::White)
        , DisplayM(28, EFontWeight::Bold, 1.2f, FLinearColor::White)
        , HeadingXL(24, EFontWeight::Bold, 1.2f, FLinearColor::White)
        , HeadingL(20, EFontWeight::Bold, 1.3f, FLinearColor::White)
        , HeadingM(18, EFontWeight::Bold, 1.3f, FLinearColor::White)
        , HeadingS(16, EFontWeight::Bold, 1.4f, FLinearColor::White)
        , BodyL(16, EFontWeight::Regular, 1.5f, FLinearColor(0.95f, 0.95f, 0.95f, 1.0f))
        , BodyM(14, EFontWeight::Regular, 1.5f, FLinearColor(0.95f, 0.95f, 0.95f, 1.0f))
        , BodyS(12, EFontWeight::Regular, 1.5f, FLinearColor(0.95f, 0.95f, 0.95f, 1.0f))
        , LabelM(14, EFontWeight::Medium, 1.3f, FLinearColor(0.70f, 0.70f, 0.70f, 1.0f))
        , LabelS(12, EFontWeight::Medium, 1.3f, FLinearColor(0.70f, 0.70f, 0.70f, 1.0f))
        , CaptionM(11, EFontWeight::Regular, 1.4f, FLinearColor(0.70f, 0.70f, 0.70f, 1.0f))
        , CaptionS(10, EFontWeight::Regular, 1.4f, FLinearColor(0.70f, 0.70f, 0.70f, 1.0f))
    {}
};

//------------------------------------------------------------------------------
//                          spacing grid
//------------------------------------------------------------------------------

/** Spatial rhythm system for consistent layout */
USTRUCT(BlueprintType)
struct GRIT_API FSpaceGrid
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space")
    float BaseUnit;                     // [px] - Foundation spacing unit

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space")
    float Micro;                        // [px] - Minimal spacing (0.25x)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space")
    float Tiny;                         // [px] - Tiny spacing (0.5x)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space")
    float Small;                        // [px] - Small spacing (1x)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space")
    float Medium;                       // [px] - Medium spacing (2x)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space")
    float Large;                        // [px] - Large spacing (3x)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space")
    float XLarge;                       // [px] - Extra large (4x)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space")
    float Massive;                      // [px] - Massive spacing (6x)

    FSpaceGrid()
        : BaseUnit(8.0f)
        , Micro(2.0f)
        , Tiny(4.0f)
        , Small(8.0f)
        , Medium(16.0f)
        , Large(24.0f)
        , XLarge(32.0f)
        , Massive(48.0f)
    {}
};

//------------------------------------------------------------------------------
//                          border system
//------------------------------------------------------------------------------

/** Corner radius specification */
UENUM(BlueprintType)
enum class ECornerRadius : uint8
{
    None    = 0     UMETA(DisplayName = "None"),
    Tight   = 1     UMETA(DisplayName = "Tight"),
    Snug    = 2     UMETA(DisplayName = "Snug"),
    Loose   = 3     UMETA(DisplayName = "Loose"),
    Round   = 4     UMETA(DisplayName = "Round"),
    Full    = 5     UMETA(DisplayName = "Full")
};

/** Border and outline properties */
USTRUCT(BlueprintType)
struct GRIT_API FBorderSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border")
    float ThicknessThin;                // [px] - Thin border width

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border")
    float ThicknessBase;                // [px] - Standard border width

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border")
    float ThicknessThick;               // [px] - Thick border width

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radius")
    float RadiusNone;                   // [px] - No rounding (0)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radius")
    float RadiusTight;                  // [px] - Minimal rounding

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radius")
    float RadiusSnug;                   // [px] - Standard rounding

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radius")
    float RadiusLoose;                  // [px] - Generous rounding

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radius")
    float RadiusRound;                  // [px] - Highly rounded

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radius")
    float RadiusFull;                   // [px] - Circular (999px)

    FBorderSpec()
        : ThicknessThin(1.0f)
        , ThicknessBase(2.0f)
        , ThicknessThick(4.0f)
        , RadiusNone(0.0f)
        , RadiusTight(4.0f)
        , RadiusSnug(8.0f)
        , RadiusLoose(12.0f)
        , RadiusRound(16.0f)
        , RadiusFull(999.0f)
    {}
};

//------------------------------------------------------------------------------
//                          motion timing
//------------------------------------------------------------------------------

/** Animation duration presets */
USTRUCT(BlueprintType)
struct GRIT_API FMotionTiming
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duration")
    float Instant;                      // [s] - Instantaneous (0.1s)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duration")
    float Swift;                        // [s] - Quick transition

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duration")
    float Brisk;                        // [s] - Standard speed

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duration")
    float Smooth;                       // [s] - Moderate pace

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duration")
    float Gentle;                       // [s] - Slow transition

    FMotionTiming()
        : Instant(0.1f)
        , Swift(0.15f)
        , Brisk(0.25f)
        , Smooth(0.35f)
        , Gentle(0.5f)
    {}
};

//------------------------------------------------------------------------------
//                          master theme
//------------------------------------------------------------------------------

/** Complete UI theme configuration */
USTRUCT(BlueprintType)
struct GRIT_API FThemeConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FPalette Palette;                   // [FPalette] - Color system

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FTypeScale Type;                    // [FTypeScale] - Typography system

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FSpaceGrid Space;                   // [FSpaceGrid] - Spacing system

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FBorderSpec Border;                 // [FBorderSpec] - Border system

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FMotionTiming Motion;               // [FMotionTiming] - Animation timing

    FThemeConfig() {}
};
