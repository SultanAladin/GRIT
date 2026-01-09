//UIToolkit.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/Font.h"
#include "UObject/NoExportTypes.h"
#include "Styling/SlateBrush.h"
#include "Fonts/SlateFontInfo.h"
#include "GenericButton.h"
#include "UserInterface/Components/ThemeConfig.h"
#include "UIToolkit.generated.h"

/*====================================================================================================================================
                                                         ENUMS
======================================================================================================================================*/

//------------------------------------------------------------------------------
// Theme mode (Light/Dark)
//------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EThemeMode : uint8
{
    Light       UMETA(DisplayName = "Light"),
    Dark        UMETA(DisplayName = "Dark")
};

//------------------------------------------------------------------------------
// Flow curve (Animation easing)
//------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EFlowCurve : uint8
{
    Linear          UMETA(DisplayName = "Linear"),
    QuadIn          UMETA(DisplayName = "Quad In"),
    QuadOut         UMETA(DisplayName = "Quad Out"),
    QuadInOut       UMETA(DisplayName = "Quad InOut"),
    CubicIn         UMETA(DisplayName = "Cubic In"),
    CubicOut        UMETA(DisplayName = "Cubic Out"),
    CubicInOut      UMETA(DisplayName = "Cubic InOut"),
    QuartIn         UMETA(DisplayName = "Quart In"),
    QuartOut        UMETA(DisplayName = "Quart Out"),
    QuartInOut      UMETA(DisplayName = "Quart InOut"),
    QuintIn         UMETA(DisplayName = "Quint In"),
    QuintOut        UMETA(DisplayName = "Quint Out"),
    QuintInOut      UMETA(DisplayName = "Quint InOut"),
    ExpoIn          UMETA(DisplayName = "Expo In"),
    ExpoOut         UMETA(DisplayName = "Expo Out"),
    ExpoInOut       UMETA(DisplayName = "Expo InOut"),
    CircIn          UMETA(DisplayName = "Circ In"),
    CircOut         UMETA(DisplayName = "Circ Out"),
    CircInOut       UMETA(DisplayName = "Circ InOut"),
    BackIn          UMETA(DisplayName = "Back In"),
    BackOut         UMETA(DisplayName = "Back Out"),
    BackInOut       UMETA(DisplayName = "Back InOut"),
    BounceIn        UMETA(DisplayName = "Bounce In"),
    BounceOut       UMETA(DisplayName = "Bounce Out"),
    BounceInOut     UMETA(DisplayName = "Bounce InOut"),
    ElasticIn       UMETA(DisplayName = "Elastic In"),
    ElasticOut      UMETA(DisplayName = "Elastic Out"),
    ElasticInOut    UMETA(DisplayName = "Elastic InOut"),
    SineIn          UMETA(DisplayName = "Sine In"),
    SineOut         UMETA(DisplayName = "Sine Out"),
    SineInOut       UMETA(DisplayName = "Sine InOut")
};

//------------------------------------------------------------------------------
// Typography System
//------------------------------------------------------------------------------

UENUM(BlueprintType, meta=(Bitflags))
enum class EFontStyle : uint8
{
    Regular = 0       UMETA(DisplayName="Regular"),
    Bold    = 1 << 0  UMETA(DisplayName="Bold"),
    Italic  = 1 << 1  UMETA(DisplayName="Italic")
    // You can add more if needed, e.g., Light, Medium, etc.
};

/*====================================================================================================================================
                                                         MATERIAL SPECIFICATION
======================================================================================================================================*/

/**
 * @brief FMaterialSpec - Material configuration for UI elements with texture and parameter binding.
 */
USTRUCT(BlueprintType)
struct GRIT_API FMaterialSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
    UMaterialInterface* BaseMaterial = nullptr;  // Base material instance

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
    UTexture2D* Texture = nullptr;  // Texture to apply

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
    TArray<FName> ParameterNames;  // Material parameter names for texture binding

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
    FLinearColor Color = FLinearColor::White;  // [RGBA] - Main fill/tint color

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
    FLinearColor BorderColor = FLinearColor::White;  // [RGBA] - Border color (if material supports it)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
    bool bUseBorderColor = false;  // Whether to apply border color parameters
};

/*====================================================================================================================================
                                                         THEME CONFIGURATION
======================================================================================================================================*/

/**
 * @brief FBorderConfiguration - Defines border styling for UI elements.
 */
USTRUCT(BlueprintType)
struct FBorderConfiguration
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border")
    ECornerStyle CornerStyle = ECornerStyle::Slight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border")
    FLinearColor DefaultBorderColor = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f);  // Dark gray

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border")
    FLinearColor HighlightBorderColor = FLinearColor(0.9f, 0.4f, 0.0f, 1.0f);  // Orange

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border")
    float BorderThickness = 2.0f;
};

/**
 * @brief FColorProfile - Defines the color scheme for a UI/visual theme.
 */
USTRUCT(BlueprintType)
struct FColorProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ColorProfile")
    FLinearColor AccentColor = FLinearColor(0.1f, 0.6f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ColorProfile")
    FLinearColor BackgroundColor = FLinearColor::Black;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ColorProfile")
    FLinearColor SecondaryBackgroundColor = FLinearColor(0.05f, 0.05f, 0.05f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ColorProfile")
    FLinearColor TextColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ColorProfile")
    FLinearColor SecondaryTextColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ColorProfile")
    FLinearColor HighlightColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ColorProfile")
    FLinearColor AlertColor = FLinearColor(1.0f, 0.1f, 0.1f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ColorProfile")
    FLinearColor DisabledColor = FLinearColor(0.3f, 0.3f, 0.3f, 0.5f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ColorProfile")
    FLinearColor SuccessColor = FLinearColor(0.2f, 0.8f, 0.2f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ColorProfile")
    FLinearColor InfoColor = FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);
};

/**
 * @brief FFontProfile - Defines font styles and text presentation for a UI/visual theme.
 */
USTRUCT(BlueprintType)
struct FFontProfile
{
    GENERATED_BODY()

    //------------------------------------------------------------------------------
    // Fonts
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fonts")
    UFont* HeaderFont = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fonts")
    UFont* TitleFont = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fonts")
    UFont* SentenceFont = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fonts")
    UFont* TooltipFont = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fonts")
    UFont* IconFont = nullptr;

    //------------------------------------------------------------------------------
    // Font Sizes
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font Sizes")
    int32 HeaderFontSize = 25;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font Sizes")
    int32 TitleFontSize = 18;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font Sizes")
    int32 SentenceFontSize = 14;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font Sizes")
    int32 TooltipFontSize = 12;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font Sizes")
    int32 IconFontSize = 16;

    //------------------------------------------------------------------------------
    // Legacy compatibility (kept for backward compatibility)
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legacy")
    UFont* PrimaryFont = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legacy")
    UFont* SecondaryFont = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legacy")
    int32 PrimaryFontSize = 25;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legacy")
    int32 SecondaryFontSize = 14;

    //------------------------------------------------------------------------------
    // Font Colors
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font Colors")
    FLinearColor HeadingFontColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font Colors")
    FLinearColor BodyFontColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font Colors")
    FLinearColor LinkFontColor = FLinearColor(0.1f, 0.6f, 1.0f, 1.0f);

    //------------------------------------------------------------------------------
    // Font Styling
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font Styling")
    bool bUseBoldForHeaders = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font Styling")
    bool bUseItalicForEmphasis = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font Styling")
    float LineHeight = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font Styling")
    float LetterSpacing = 0.0f;
};

/*====================================================================================================================================
                                                         TYPOGRAPHY SYSTEM
======================================================================================================================================*/

/**
 * @brief FTypographyStyle - Defines a complete typography style with font, size, color, and styling options.
 */
USTRUCT(BlueprintType)
struct FTypographyStyle
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    UFont* Font = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    int32 Size = 24; // [pt]

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    FLinearColor Color = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    int32 LetterSpacing = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    float LineHeight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    FLinearColor OutlineColor = FLinearColor::Black;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    int32 OutlineSize = 0;

    // Font style using bitflags
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography", meta=(Bitmask, BitmaskEnum="/Script/GRIT.EFontStyle"))
    uint8 FontStyleFlags = static_cast<uint8>(EFontStyle::Regular);

    // Underline separate
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    bool bUnderline = false;
};

/**
 * @brief FInterfaceStyle - Single typography theme containing all text styles for the interface.
 */
USTRUCT(BlueprintType)
struct FInterfaceStyle
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    FTypographyStyle DisplayLarge;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    FTypographyStyle DisplayMedium;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    FTypographyStyle DisplaySmall;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    FTypographyStyle Heading;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    FTypographyStyle SubHeading;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    FTypographyStyle Title;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    FTypographyStyle Subtitle;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    FTypographyStyle Body;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    FTypographyStyle Caption;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    FTypographyStyle Button;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    FTypographyStyle Label;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
    FTypographyStyle Emoji; // emoji and icon fonts
};

/**
 * @brief FThemeConfiguration - Container struct that combines color and font profiles into a full UI configuration.
 */
USTRUCT(BlueprintType)
struct FThemeConfiguration
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIConfiguration")
    EThemeMode ThemeMode = EThemeMode::Dark;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIConfiguration")
    FColorProfile ColorProfile;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIConfiguration")
    FFontProfile FontProfile;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIConfiguration")
    FBorderConfiguration BorderConfiguration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIConfiguration")
    FInterfaceStyle InterfaceStyle;
};

/*====================================================================================================================================
                                                         UI TOOLKIT
======================================================================================================================================*/

/**
 * Static utility library for common UI operations
 * Provides helper functions for widget manipulation, styling, and layout
 */
UCLASS()
class GRIT_API UUIToolkit : public UObject
{
    GENERATED_BODY()

public:
    //------------------------------------------------------------------------------
    // Widget Utilities
    //------------------------------------------------------------------------------

    /** Get theme configuration from SessionAdapter */
    UFUNCTION(BlueprintCallable, Category = "UIToolkit|Theme")
    static FThemeConfig GetTheme(const UObject* WorldContextObject);

    /** Apply theme colors to a text block */
    UFUNCTION(BlueprintCallable, Category = "UIToolkit|Theme")
    static void ApplyThemeToText(class UTextBlock* TextBlock, const UObject* WorldContextObject, bool bUseHeadingStyle = false);

    /** Apply theme colors to a border */
    UFUNCTION(BlueprintCallable, Category = "UIToolkit|Theme")
    static void ApplyThemeToBorder(class UBorder* Border, const UObject* WorldContextObject);

    /** Get interface style from SessionAdapter */
    UFUNCTION(BlueprintCallable, Category = "UIToolkit|Typography")
    static FTypeScale GetInterfaceStyle(const UObject* WorldContextObject);

    /** Apply typography style to a text block */
    UFUNCTION(BlueprintCallable, Category = "UIToolkit|Typography")
    static void ApplyTypographyStyle(class UTextBlock* TextBlock, const FTypeSpec& TypeSpec);

    //------------------------------------------------------------------------------
    // Layout Utilities
    //------------------------------------------------------------------------------

    /** Set widget visibility with optional animation */
    UFUNCTION(BlueprintCallable, Category = "UIToolkit|Layout")
    static void SetWidgetVisibility(class UWidget* Widget, bool bVisible, bool bAnimate = false);

    /** Center widget in overlay slot */
    UFUNCTION(BlueprintCallable, Category = "UIToolkit|Layout")
    static void CenterInOverlay(class UWidget* Widget);

    //------------------------------------------------------------------------------
    // Material Utilities
    //------------------------------------------------------------------------------

    /** Create dynamic material instance for widget */
    UFUNCTION(BlueprintCallable, Category = "UIToolkit|Material")
    static class UMaterialInstanceDynamic* CreateDynamicMaterialForWidget(class UImage* Image, class UMaterialInterface* BaseMaterial);

    /** Set material parameters from array of names */
    UFUNCTION(BlueprintCallable, Category = "UIToolkit|Material")
    static void SetMaterialParameters(class UMaterialInstanceDynamic* Material, const TArray<FName>& ParameterNames, FLinearColor Color);

    //------------------------------------------------------------------------------
    // Animation Utilities
    //------------------------------------------------------------------------------

    /** Evaluate easing curve at normalized time [0-1] */
    UFUNCTION(BlueprintPure, Category = "UIToolkit|Animation")
    static float EvalFlowCurve(EFlowCurve Curve, float NormalizedTime);

    /** Interpolate between two values using easing curve */
    UFUNCTION(BlueprintPure, Category = "UIToolkit|Animation")
    static FORCEINLINE float LerpWithCurve(float Start, float End, float NormalizedTime, EFlowCurve Curve)
    {
        float T = EvalFlowCurve(Curve, NormalizedTime);
        return FMath::Lerp(Start, End, T);
    }

    /** Interpolate between two vectors using easing curve */
    UFUNCTION(BlueprintPure, Category = "UIToolkit|Animation")
    static FORCEINLINE FVector LerpVectorWithCurve(FVector Start, FVector End, float NormalizedTime, EFlowCurve Curve)
    {
        float T = EvalFlowCurve(Curve, NormalizedTime);
        return FMath::Lerp(Start, End, T);
    }

    /** Interpolate between two colors using easing curve */
    UFUNCTION(BlueprintPure, Category = "UIToolkit|Animation")
    static FORCEINLINE FLinearColor LerpColorWithCurve(FLinearColor Start, FLinearColor End, float NormalizedTime, EFlowCurve Curve)
    {
        float T = EvalFlowCurve(Curve, NormalizedTime);
        return FMath::Lerp(Start, End, T);
    }

private:
    //------------------------------------------------------------------------------
    // Easing Implementation (Inline for performance)
    //------------------------------------------------------------------------------

    static FORCEINLINE float EaseQuadIn(float T) { return T * T; } // [unit²]
    static FORCEINLINE float EaseQuadOut(float T) { return 1.0f - (1.0f - T) * (1.0f - T); } // [unit²]
    static FORCEINLINE float EaseQuadInOut(float T) { return T < 0.5f ? 2.0f * T * T : 1.0f - FMath::Pow(-2.0f * T + 2.0f, 2.0f) / 2.0f; } // [unit²]
    static FORCEINLINE float EaseCubicIn(float T) { return T * T * T; } // [unit³]
    static FORCEINLINE float EaseCubicOut(float T) { float F = 1.0f - T; return 1.0f - F * F * F; } // [unit³]
    static FORCEINLINE float EaseCubicInOut(float T) { return T < 0.5f ? 4.0f * T * T * T : 1.0f - FMath::Pow(-2.0f * T + 2.0f, 3.0f) / 2.0f; } // [unit³]
    static FORCEINLINE float EaseQuartIn(float T) { return T * T * T * T; } // [unit⁴]
    static FORCEINLINE float EaseQuartOut(float T) { float F = 1.0f - T; return 1.0f - F * F * F * F; } // [unit⁴]
    static FORCEINLINE float EaseQuartInOut(float T) { return T < 0.5f ? 8.0f * T * T * T * T : 1.0f - FMath::Pow(-2.0f * T + 2.0f, 4.0f) / 2.0f; } // [unit⁴]
    static FORCEINLINE float EaseQuintIn(float T) { return T * T * T * T * T; } // [unit⁵]
    static FORCEINLINE float EaseQuintOut(float T) { float F = 1.0f - T; return 1.0f - F * F * F * F * F; } // [unit⁵]
    static FORCEINLINE float EaseQuintInOut(float T) { return T < 0.5f ? 16.0f * T * T * T * T * T : 1.0f - FMath::Pow(-2.0f * T + 2.0f, 5.0f) / 2.0f; } // [unit⁵]
    static FORCEINLINE float EaseExpoIn(float T) { return T == 0.0f ? 0.0f : FMath::Pow(2.0f, 10.0f * T - 10.0f); } // [unit²¹⁰]
    static FORCEINLINE float EaseExpoOut(float T) { return T == 1.0f ? 1.0f : 1.0f - FMath::Pow(2.0f, -10.0f * T); } // [unit²⁻¹⁰]
    static FORCEINLINE float EaseExpoInOut(float T) { return T == 0.0f ? 0.0f : T == 1.0f ? 1.0f : T < 0.5f ? FMath::Pow(2.0f, 20.0f * T - 10.0f) / 2.0f : (2.0f - FMath::Pow(2.0f, -20.0f * T + 10.0f)) / 2.0f; } // [unit²²⁰/²⁻²⁰]
    static FORCEINLINE float EaseCircIn(float T) { return 1.0f - FMath::Sqrt(1.0f - T * T); } // [unit^0.5]
    static FORCEINLINE float EaseCircOut(float T) { return FMath::Sqrt(1.0f - (T - 1.0f) * (T - 1.0f)); } // [unit^0.5]
    static FORCEINLINE float EaseCircInOut(float T) { return T < 0.5f ? (1.0f - FMath::Sqrt(1.0f - 4.0f * T * T)) / 2.0f : (FMath::Sqrt(1.0f - FMath::Pow(-2.0f * T + 2.0f, 2.0f)) + 1.0f) / 2.0f; } // [unit^0.5]
    static FORCEINLINE float EaseBackIn(float T) { const float C1 = 1.70158f; const float C3 = C1 + 1.0f; return C3 * T * T * T - C1 * T * T; } // [unit³]
    static FORCEINLINE float EaseBackOut(float T) { const float C1 = 1.70158f; const float C3 = C1 + 1.0f; return 1.0f + C3 * FMath::Pow(T - 1.0f, 3.0f) + C1 * FMath::Pow(T - 1.0f, 2.0f); } // [unit³]
    static FORCEINLINE float EaseBackInOut(float T) { const float C1 = 1.70158f; const float C2 = C1 * 1.525f; return T < 0.5f ? (FMath::Pow(2.0f * T, 2.0f) * ((C2 + 1.0f) * 2.0f * T - C2)) / 2.0f : (FMath::Pow(2.0f * T - 2.0f, 2.0f) * ((C2 + 1.0f) * (T * 2.0f - 2.0f) + C2) + 2.0f) / 2.0f; } // [unit³]
    static FORCEINLINE float EaseSineIn(float T) { return 1.0f - FMath::Cos((T * PI) / 2.0f); } // [rad]
    static FORCEINLINE float EaseSineOut(float T) { return FMath::Sin((T * PI) / 2.0f); } // [rad]
    static FORCEINLINE float EaseSineInOut(float T) { return -(FMath::Cos(PI * T) - 1.0f) / 2.0f; } // [rad]
    
    static float EaseBounceOut(float T);
    static FORCEINLINE float EaseBounceIn(float T) { return 1.0f - EaseBounceOut(1.0f - T); }
    static FORCEINLINE float EaseBounceInOut(float T) { return T < 0.5f ? (1.0f - EaseBounceOut(1.0f - 2.0f * T)) / 2.0f : (1.0f + EaseBounceOut(2.0f * T - 1.0f)) / 2.0f; }
    
    static float EaseElasticOut(float T);
    static FORCEINLINE float EaseElasticIn(float T) { return 1.0f - EaseElasticOut(1.0f - T); }
    static FORCEINLINE float EaseElasticInOut(float T) { return T < 0.5f ? (1.0f - EaseElasticOut(1.0f - 2.0f * T)) / 2.0f : (1.0f + EaseElasticOut(2.0f * T - 1.0f)) / 2.0f; }
};
