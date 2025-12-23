#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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
    UPROPERTY(EditAnywhere, Category = "Tray", meta = (AllowPrivateAccess = "true"))
    float AnimDuration = 0.32f;          // [s] - Smooth expansion time

    UPROPERTY(EditAnywhere, Category = "Tray", meta = (AllowPrivateAccess = "true"))
    float TargetWidth = 300.0f;          // [px] - Expanded panel width

    UPROPERTY(EditAnywhere, Category = "Tray", meta = (AllowPrivateAccess = "true"))
    float TargetPanelHeight = 200.0f;    // [px] - Expanded panel height

    UPROPERTY(EditAnywhere, Category = "Tray", meta = (AllowPrivateAccess = "true"))
    FVector2D Margin = FVector2D(8.0f, 8.0f);  // [px] - Extra spacing around button when collapsed

    //------------------------------------------------------------------------------
    // Measured Dimensions
    //------------------------------------------------------------------------------
    float CachedButtonHeight = 0.0f;     // [px] - Button-only height (collapsed state)
    float CachedButtonWidth = 0.0f;      // [px] - Button-only width (collapsed state)
    float CachedContentHeight = 0.0f;    // [px] - Full content height (expanded state)
    float CachedContentWidth = 0.0f;     // [px] - Full content width (expanded state)

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

    /** Smooth cubic easing curve */
    float BlendCurve(float t) const;
};