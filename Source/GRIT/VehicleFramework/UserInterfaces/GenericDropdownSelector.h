//GenericDropdownSelector.h
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
#include "GenericDropdownSelector.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDropdownSelectorSelectionChanged, int32, SelectedIndex, const FString&, SelectedData);

/*====================================================================================================================================
                                                         DROPDOWN ITEM CONFIG
======================================================================================================================================*/

USTRUCT(BlueprintType)
struct FDropdownSelectorItemConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    FText Label = FText::FromString("Item");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    FString Data = "";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor TextColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor HoverTextColor = FLinearColor(0.8f, 0.8f, 0.8f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor HoverBackgroundColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.1f);

    FDropdownSelectorItemConfig() {}
};

/*====================================================================================================================================
                                                         DROPDOWN ITEM WIDGET
======================================================================================================================================*/

UCLASS(Blueprintable, BlueprintType)
class GRIT_API UGenericDropdownSelectorItem : public UUserWidget
{
    GENERATED_BODY()

public:
    UGenericDropdownSelectorItem(const FObjectInitializer& ObjectInitializer);

    /** Initialize item with configuration */
    UFUNCTION(BlueprintCallable, Category = "DropdownSelectorItem")
    void InitItem(const FDropdownSelectorItemConfig& Config, int32 Index);

    /** Get item data */
    UFUNCTION(BlueprintPure, Category = "DropdownSelectorItem")
    FString GetItemData() const { return ItemData; }

    /** Get item index */
    UFUNCTION(BlueprintPure, Category = "DropdownSelectorItem")
    int32 GetItemIndex() const { return ItemIndex; }

    /** Apply corner styling to this item */
    UFUNCTION(BlueprintCallable, Category = "DropdownSelectorItem")
    void ApplyCornerStyling(ECornerStyle CornerStyle);

    /** Apply theme colors to this item */
    UFUNCTION(BlueprintCallable, Category = "DropdownSelectorItem")
    void ApplyThemeColors(bool bUseTheme, const UObject* WorldContextObject);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDropdownSelectorItemClicked, UGenericDropdownSelectorItem*, Item);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnDropdownSelectorItemClicked OnItemClicked;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& MouseEvent) override;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UBorder* ItemBorder;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* ItemLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* ItemButton;

private:
    FString ItemData;
    int32 ItemIndex = -1;
    FDropdownSelectorItemConfig ItemConfig;

    /** Calculate corner radius based on corner style and item size */
    float GetCornerRadius(ECornerStyle CornerStyle) const;

    UFUNCTION()
    void OnButtonClicked();
};

/*====================================================================================================================================
                                                         ITEM ANIMATION STATE
======================================================================================================================================*/

USTRUCT()
struct FItemAnimationState
{
    GENERATED_BODY()

    FTimerHandle AnimTimer;
    float ElapsedTime = 0.0f;
    float StartDelay = 0.0f;
    bool bIsAnimating = false;
    UGenericDropdownSelectorItem* ItemWidget = nullptr;

    FItemAnimationState() {}
};

/*====================================================================================================================================
                                                         GENERIC DROPDOWN SELECTOR WIDGET
======================================================================================================================================*/

UCLASS(Blueprintable, BlueprintType)
class GRIT_API UGenericDropdownSelector : public UUserWidget
{
    GENERATED_BODY()

public:
    UGenericDropdownSelector(const FObjectInitializer& ObjectInitializer);

    /** Toggle dropdown open/closed state */
    UFUNCTION(BlueprintCallable, Category = "DropdownSelector") 
    void Toggle();
    
    /** Set dropdown state explicitly */
    UFUNCTION(BlueprintCallable, Category = "DropdownSelector") 
    void SetOpen(bool bNewOpen);
    
    /** Add item from configuration struct */
    UFUNCTION(BlueprintCallable, Category = "DropdownSelector") 
    void AddItem(const FDropdownSelectorItemConfig& ItemConfig);
    
    /** Clear all items from list */
    UFUNCTION(BlueprintCallable, Category = "DropdownSelector") 
    void ClearItems();
    
    /** Set selected item by index */
    UFUNCTION(BlueprintCallable, Category = "DropdownSelector") 
    void SetSelectedIndex(int32 Index);
    
    /** Get currently selected index */
    UFUNCTION(BlueprintPure, Category = "DropdownSelector") 
    int32 GetSelectedIndex() const { return SelectedItemIndex; }

    /** Get if dropdown is open */
    UFUNCTION(BlueprintPure, Category = "DropdownSelector") 
    bool IsOpen() const { return bIsOpen; }

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnDropdownSelectorSelectionChanged OnSelectionChanged;

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnSelectionChangedBP(int32 SelectedIndex, const FString& SelectedData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnDropdownOpenedBP();

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnDropdownClosedBP();

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
    USizeBox* ItemsSizeBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) 
    UBorder* ItemsPanel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) 
    UVerticalBox* ItemsContainer;

    //------------------------------------------------------------------------------
    // Configuration - SET THESE IN THE BLUEPRINT EDITOR
    //------------------------------------------------------------------------------

    /** Array of item configs to populate on construction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Items")
    TArray<FDropdownSelectorItemConfig> DefaultItems;

    /** Default item class for creating dropdown items */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Items")
    TSubclassOf<UGenericDropdownSelectorItem> ItemWidgetClass;

    // Size Animation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    float SizeAnimDuration = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    EFlowCurve SizeAnimCurve = EFlowCurve::QuadOut;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    bool bAnimateWidth = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    bool bAnimateHeight = true;

    // Item Animation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    float ItemAnimDuration = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    float ItemStaggerDelay = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    EFlowCurve ItemAnimCurve = EFlowCurve::QuadOut;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    float ItemCollapseOffset = -30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    float ItemCollapseScale = 0.9f;

    // Chevron Animation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    bool bAnimateChevron = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    float ChevronAnimDuration = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    FText TriggerText = FText::FromString("Choose option");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    float ItemsPanelMaxHeight = 200.0f;

    /** Corner rounding style for dropdown items */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    ECornerStyle ItemCornerStyle = ECornerStyle::Slight;

    /** Apply theme colors to dropdown items */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    bool bUseThemeColors = true;

    //------------------------------------------------------------------------------
    // Runtime state
    //------------------------------------------------------------------------------
    bool bIsOpen = false;
    int32 SelectedItemIndex = -1;
    TArray<UGenericDropdownSelectorItem*> DropdownItems;
    TArray<FItemAnimationState> ItemAnimStates;

    // Size animation state
    bool bSizeAnimating = false;
    float SizeAnimTime = 0.0f;
    float StartHeight = 0.0f;
    float TargetHeight = 0.0f;
    float StartWidth = 0.0f;
    float TargetWidth = 0.0f;
    FTimerHandle SizeAnimationTimer;
    FTimerHandle ChevronAnimTimer;

    //------------------------------------------------------------------------------
    // Internal methods
    //------------------------------------------------------------------------------

    void ConfigureSlotProperties();
    void StartExpandAnimation();
    void StartCollapseAnimation();
    void TickSizeAnimation();
    void AnimateChevron();
    void CalculateTargetSize();
    
    void StartItemAnimations(bool bExpanding);
    void AnimateItem(int32 ItemIndex, bool bExpanding);
    void UpdateItemTransform(UGenericDropdownSelectorItem* Item, float Alpha, bool bExpanding);
    void StopAllItemAnimations();
    
    /** Calculate corner radius based on corner style and item size */
    float GetItemCornerRadius(const FVector2D& ItemSize) const;
    
    /** Apply corner styling to dropdown item */
    void ApplyItemStyling(UGenericDropdownSelectorItem* Item);

    UFUNCTION() 
    void OnItemClicked(UGenericDropdownSelectorItem* Item);

    UFUNCTION() 
    void OnTriggerClicked();
};