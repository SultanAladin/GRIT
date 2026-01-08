#pragma once
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "UIThemeData.generated.h"

// Dedicated UI theme struct for consistent styling across all widgets, panels, and UI components
USTRUCT(BlueprintType)
struct GRIT_API FUIThemeData
{
    GENERATED_BODY()

    // Primary accent colors
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Primary Colors")
    FLinearColor AccentPrimary = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f); // White

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Primary Colors")
    FLinearColor AccentSecondary = FLinearColor(0.8f, 0.8f, 0.8f, 1.0f); // Light Grey

    // Background colors
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Background Colors")
    FLinearColor BackgroundDark = FLinearColor(0.067f, 0.067f, 0.067f, 1.0f); // #111111

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Background Colors")
    FLinearColor BackgroundCard = FLinearColor(0.102f, 0.102f, 0.102f, 1.0f); // #1a1a1a

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Background Colors")
    FLinearColor BackgroundHover = FLinearColor(0.145f, 0.145f, 0.145f, 1.0f); // #252525

    // Text colors
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text Colors")
    FLinearColor TextPrimary = FLinearColor(0.867f, 0.867f, 0.867f, 1.0f); // #dddddd

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text Colors")
    FLinearColor TextMuted = FLinearColor(0.533f, 0.533f, 0.533f, 1.0f); // #888888

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text Colors")
    FLinearColor TextOnAccent = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f); // Dark text for accent backgrounds

    // Border and outline settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border Settings")
    FLinearColor BorderNormal = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f); // #333333

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border Settings")
    FLinearColor BorderHover = FLinearColor(0.333f, 0.333f, 0.333f, 1.0f); // #555555

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border Settings")
    float BorderRadius = 12.0f;

    // Animation settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings")
    float AnimationSpeed = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings")
    float HoverScale = 1.05f;

    // Default constructor
    FUIThemeData()
    {
        // All values already initialized above
    }
};