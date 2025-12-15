#pragma once

#include "CoreMinimal.h"
#include "Engine/Font.h"
#include "UObject/NoExportTypes.h"
#include "ThemeConfiguration.generated.h"

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

    // Default font for UI text
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FontProfile")
    UFont* PrimaryFont = nullptr;

    // Secondary font (e.g., captions, tooltips)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FontProfile")
    UFont* SecondaryFont = nullptr;

    // Default size for main UI text (clamped ±3)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FontProfile", meta=(ClampMin="11", ClampMax="17"))
    int32 PrimaryFontSize = 14;

    // Smaller/alternative size (clamped ±3)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FontProfile", meta=(ClampMin="7", ClampMax="13"))
    int32 SecondaryFontSize = 10;

    // Bold/strong emphasis style
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FontProfile")
    bool bUseBoldForHeaders = true;

    // Italic style toggle
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FontProfile")
    bool bUseItalicForEmphasis = false;

    // Line height (multiple of font size)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FontProfile")
    float LineHeight = 1.2f;

    // Letter spacing
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FontProfile")
    float LetterSpacing = 0.0f;

    // Color for headings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FontProfile")
    FLinearColor HeadingFontColor = FLinearColor::White;

    // Color for body text
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FontProfile")
    FLinearColor BodyFontColor = FLinearColor::White;

    // Color for links or interactive text
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FontProfile")
    FLinearColor LinkFontColor = FLinearColor(0.1f, 0.6f, 1.0f, 1.0f);
};

/**
 * @brief FUIConfiguration - Container struct that combines color and font profiles into a full UI configuration.
 */
USTRUCT(BlueprintType)
struct FThemeConfiguration
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIConfiguration")
    FColorProfile ColorProfile;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIConfiguration")
    FFontProfile FontProfile;
};
