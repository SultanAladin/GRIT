#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "AppData.h"
#include "AppIcon.generated.h"

/*====================================================================================================================================
                                                         APPLICATION ICON WIDGET
======================================================================================================================================*/

/** Individual app icon with hover scaling */
UCLASS()
class GRIT_API UAppIcon : public UUserWidget
{
    GENERATED_BODY()

public:
    //------------------------------------------------------------------------------
    // widget bindings
    //------------------------------------------------------------------------------
    
    UPROPERTY(meta = (BindWidget))
    UBorder* AppBorder; // [UBorder*] - Interactive container

    UPROPERTY(meta = (BindWidget))
    UImage* AppImage; // [UImage*] - Icon display

    UPROPERTY(meta = (BindWidget))
    UTextBlock* AppText; // [UTextBlock*] - Label display

    /** Configure app data */
    void ConfigureApp(const FAppConfig& Config);

    /** Assign parent grid reference */
    void AssignParentGrid(class UAppGrid* Grid) { ParentGrid = Grid; }

    /** Retrieve app identifier */
    int32 FetchAppID() const { return AppID; }

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
    //------------------------------------------------------------------------------
    // internal state
    //------------------------------------------------------------------------------
    
    UPROPERTY()
    class UAppGrid* ParentGrid; // [UAppGrid*] - Parent container reference
    
    FAppConfig AppConfig; // [FAppConfig] - Cached configuration
    
    int32 AppID; // [int32] - Cached identifier
    
    FLinearColor BaseColor; // [RGBA] - Idle border color
    FLinearColor HilightColor; // [RGBA] - Hover border color
    
    float CurrentScale; // [scale] - Active scale multiplier
    float TargetScale; // [scale] - Desired scale multiplier
    float ScaleSpeed; // [s⁻¹] - Scale transition rate
    
    bool bIsHovered; // [bool] - Hover tracking
    bool bIsPressed; // [bool] - Press tracking

    /** Bootstrap theme styling */
    void BootstrapTheme();

    /** Drive scale animation */
    void DriveScale(float DeltaTime);

    /** Trigger app launch */
    void TriggerLaunch();
};
