#pragma once

#include "CoreMinimal.h"
#include "../VehicleFramework/UserInterfaces/Verified/Components/ThemeConfig.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ThemeUtil.generated.h"

/*====================================================================================================================================
                                                         THEME UTILITIES
======================================================================================================================================*/

/** Static theme access for all widgets */
UCLASS()
class GRIT_API UThemeUtil : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Fetch active theme from game instance */
    UFUNCTION(BlueprintPure, Category = "Theme", meta = (WorldContext = "WorldContext"))
    static FThemeConfig FetchTheme(const UObject* WorldContext);

    /** Fetch color palette from active theme */
    UFUNCTION(BlueprintPure, Category = "Theme", meta = (WorldContext = "WorldContext"))
    static FPalette FetchPalette(const UObject* WorldContext);

    /** Fetch typography scale from active theme */
    UFUNCTION(BlueprintPure, Category = "Theme", meta = (WorldContext = "WorldContext"))
    static FTypeScale FetchTypeScale(const UObject* WorldContext);

    /** Fetch spacing grid from active theme */
    UFUNCTION(BlueprintPure, Category = "Theme", meta = (WorldContext = "WorldContext"))
    static FSpaceGrid FetchSpaceGrid(const UObject* WorldContext);

    /** Fetch border spec from active theme */
    UFUNCTION(BlueprintPure, Category = "Theme", meta = (WorldContext = "WorldContext"))
    static FBorderSpec FetchBorderSpec(const UObject* WorldContext);

    /** Fetch motion timing from active theme */
    UFUNCTION(BlueprintPure, Category = "Theme", meta = (WorldContext = "WorldContext"))
    static FMotionTiming FetchMotionTiming(const UObject* WorldContext);

    /** Apply type spec to text block */
    UFUNCTION(BlueprintCallable, Category = "Theme")
    static void ApplyTypeSpec(class UTextBlock* TextBlock, const FTypeSpec& Spec);

    /** Apply border styling to border widget */
    UFUNCTION(BlueprintCallable, Category = "Theme")
    static void ApplyBorderStyling(class UBorder* Border, FLinearColor Color, float Radius, float Thickness = 0.0f);

    /** Blend color with overlay (for hover states) */
    UFUNCTION(BlueprintPure, Category = "Theme")
    static FLinearColor BlendOverlay(FLinearColor Base, FLinearColor Overlay);
};
