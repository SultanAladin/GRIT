#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UIToolkit.h"
#include "GenericTray.generated.h"

class UVerticalBox;
class USizeBox;
class UBorder;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrayToggled, bool, bIsOpen);

/*====================================================================================================================================
                                                         TRAY WIDGET
======================================================================================================================================*/

UCLASS()
class GRIT_API UGenericTray : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Toggle panel open/closed state */
    UFUNCTION(BlueprintCallable, Category = "Tray")
    void FlipPanel();

    /** Query panel state */
    UFUNCTION(BlueprintPure, Category = "Tray")
    bool IsOpen() const { return bIsOpen; }

    UPROPERTY(BlueprintAssignable, Category = "Tray")
    FOnTrayToggled OnTrayToggled;

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    //------------------------------------------------------------------------------
    // Bound Widgets
    //------------------------------------------------------------------------------
    UPROPERTY(meta = (BindWidget))
    USizeBox* ContentBox;                // Auto-sizing content panel

    UPROPERTY(meta = (BindWidget))
    UBorder* Border;                     // Content wrapper

    UPROPERTY(meta = (BindWidget))
    UVerticalBox* RootVertical;          // Content layout

    UPROPERTY(meta = (BindWidget))
    UButton* TriggerButton;              // Click target

    //------------------------------------------------------------------------------
    // Configuration
    //------------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tray|Animation", meta = (AllowPrivateAccess = "true"))
    float AnimDuration = 0.3f;          // [s] - Animation duration (FIXED: was 0.32f)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tray|Animation", meta = (AllowPrivateAccess = "true"))
    EFlowCurve AnimationCurve = EFlowCurve::QuadOut; // Animation easing curve (FIXED: was hardcoded)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tray|Sizing", meta = (AllowPrivateAccess = "true"))
    float TargetWidth = 300.0f;          // [px] - Expanded panel width

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tray|Sizing", meta = (AllowPrivateAccess = "true"))
    float TargetPanelHeight = 200.0f;    // [px] - Expanded panel height

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tray|Sizing", meta = (AllowPrivateAccess = "true"))
    FVector2D Margin = FVector2D(8.0f, 8.0f);  // [px] - Extra spacing around button when collapsed

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tray|Sizing", meta = (AllowPrivateAccess = "true"))
    bool bAutoSizeFromButton = true;     // Auto-detect button size instead of using arbitrary defaults

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tray|Sizing", meta = (AllowPrivateAccess = "true"))
    bool bAutoSizeFromContent = true;    // Auto-detect content size instead of using TargetWidth/Height

    //------------------------------------------------------------------------------
    // Measured Dimensions
    //------------------------------------------------------------------------------
    float CachedButtonHeight = 0.0f;     // [px] - Button-only height (collapsed state)
    float CachedButtonWidth = 0.0f;      // [px] - Button-only width (collapsed state)
    float CachedContentHeight = 0.0f;    // [px] - Full content height (expanded state)
    float CachedContentWidth = 0.0f;     // [px] - Full content width (expanded state)
    bool bDimensionsCached = false;      // [-] - Whether dimensions have been measured

    //------------------------------------------------------------------------------
    // Runtime State
    //------------------------------------------------------------------------------
    bool bIsOpen = false;
    bool bAnimating = false;
    float AnimTime = 0.0f;               // [s] - Current animation progress
    float StartHeight = 0.0f;            // [px] - Animation start value (height)
    float TargetAnimHeight = 0.0f;       // [px] - Animation end value (height)
    float StartWidth = 0.0f;             // [px] - Animation start value (width)
    float TargetAnimWidth = 0.0f;        // [px] - Animation end value (width)

    /** Button click handler */
    UFUNCTION()
    void OnTriggerPressed();

    /** Drive size animation each frame */
    void DriveMotion(float DeltaTime);

    /** Measure button and content dimensions */
    void CacheDimensions();

    /** Apply initial collapsed size */
    void ApplyCollapsedSize();
};