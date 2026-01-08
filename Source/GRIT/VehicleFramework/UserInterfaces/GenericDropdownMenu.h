//GenericDropdownMenu.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Styling/SlateBrush.h"
#include "UIToolkit.h"
#include "GenericDropdownMenu.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDropdownMenuToggled, bool, bIsOpen);

/*====================================================================================================================================
                                                         GENERIC DROPDOWN MENU WIDGET
======================================================================================================================================*/

UCLASS(Blueprintable, BlueprintType)
class GRIT_API UGenericDropdownMenu : public UUserWidget
{
    GENERATED_BODY()

public:
    UGenericDropdownMenu(const FObjectInitializer& ObjectInitializer);

    /** Toggle dropdown open/closed state */
    UFUNCTION(BlueprintCallable, Category = "DropdownMenu") 
    void Toggle();
    
    /** Set dropdown state explicitly */
    UFUNCTION(BlueprintCallable, Category = "DropdownMenu") 
    void SetOpen(bool bNewOpen);
    
    /** Get if dropdown is open */
    UFUNCTION(BlueprintPure, Category = "DropdownMenu") 
    bool IsOpen() const { return bIsOpen; }

    /** Force recalculation of content size */
    UFUNCTION(BlueprintCallable, Category = "DropdownMenu")
    void RecalculateContentSize();

    /** Get current calculated content size for debugging */
    UFUNCTION(BlueprintPure, Category = "DropdownMenu")
    FVector2D GetCalculatedContentSize() const { return FVector2D(TargetWidth, TargetHeight); }

    /** Force immediate size measurement for debugging */
    UFUNCTION(BlueprintCallable, Category = "DropdownMenu")
    FVector2D DebugMeasureContentSize();

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnDropdownMenuToggled OnMenuToggled;

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnMenuOpenedBP();

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnMenuClosedBP();

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    //------------------------------------------------------------------------------
    // Widget bindings (bound to Blueprint widgets)
    //------------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) 
    UBorder* RootBorder;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) 
    UVerticalBox* MainContainer;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) 
    UBorder* TriggerBorder;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) 
    UHorizontalBox* TriggerBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) 
    UTextBlock* TriggerLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) 
    UImage* ChevronIcon;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) 
    UButton* TriggerButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) 
    UOverlay* DropdownOverlay;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) 
    USizeBox* ContentSizeBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) 
    UBorder* ContentPanel;

    // Content container where Blueprint widgets are manually added
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) 
    UVerticalBox* ContentContainer;

    //------------------------------------------------------------------------------
    // Configuration - SET THESE IN THE BLUEPRINT EDITOR
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    float AnimDuration = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    EFlowCurve AnimCurve = EFlowCurve::QuadOut;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    bool bAnimateChevron = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    float ChevronAnimDuration = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    bool bAnimateWidth = false;  // Animate X-axis (width)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    bool bAnimateHeight = true;  // Animate Y-axis (height) - DEFAULT

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    bool bUseSlideAnimation = true;  // Add subtle slide effect

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    FText TriggerText = FText::FromString("Menu");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    float MaxContentWidth = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    float MaxContentHeight = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    FVector2D FallbackContentSize = FVector2D(200.0f, 150.0f);

    /** Corner rounding style for dropdown content panel */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    ECornerStyle ContentCornerStyle = ECornerStyle::Slight;

    /** Apply theme colors to dropdown content */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    bool bUseThemeColors = true;

    //------------------------------------------------------------------------------
    // Runtime state
    //------------------------------------------------------------------------------
    bool bIsOpen = false;

    // Animation state
    bool bAnimating = false;
    float AnimTime = 0.0f;
    float StartHeight = 0.0f;
    float TargetHeight = 0.0f;
    float StartWidth = 0.0f;
    float TargetWidth = 0.0f;
    FTimerHandle AnimationTimer;
    FTimerHandle ChevronAnimTimer;

    //------------------------------------------------------------------------------
    // Internal methods
    //------------------------------------------------------------------------------

    void ConfigureSlotProperties();
    void StartExpandAnimation();
    void StartCollapseAnimation();
    void TickAnimation();
    void AnimateChevron();
    void CalculateContentSize();
    
    /** Calculate corner radius based on corner style and content size */
    float GetContentCornerRadius(const FVector2D& ContentSize) const;
    
    /** Apply corner styling to dropdown content panel */
    void ApplyContentStyling();

    UFUNCTION() 
    void OnTriggerClicked();
};