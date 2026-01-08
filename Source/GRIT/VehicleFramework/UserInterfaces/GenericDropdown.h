//GenericDropdown.h
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
#include "GenericDropdown.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGenericDropdownSelectionChanged, int32, SelectedIndex, const FString&, SelectedData);

/*====================================================================================================================================
                                                         DROPDOWN ITEM CONFIG
======================================================================================================================================*/

USTRUCT(BlueprintType)
struct FDropdownItemConfig
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

    FDropdownItemConfig() {}
};

/*====================================================================================================================================
                                                         DROPDOWN ITEM WIDGET
======================================================================================================================================*/

UCLASS(Blueprintable, BlueprintType)
class GRIT_API UGenericDropdownItem : public UUserWidget
{
    GENERATED_BODY()

public:
    UGenericDropdownItem(const FObjectInitializer& ObjectInitializer);

    /** Initialize item with configuration */
    UFUNCTION(BlueprintCallable, Category = "DropdownItem")
    void InitItem(const FDropdownItemConfig& Config, int32 Index);

    /** Get item data */
    UFUNCTION(BlueprintPure, Category = "DropdownItem")
    FString GetItemData() const { return ItemData; }

    /** Get item index */
    UFUNCTION(BlueprintPure, Category = "DropdownItem")
    int32 GetItemIndex() const { return ItemIndex; }

    /** Apply corner styling to this item */
    UFUNCTION(BlueprintCallable, Category = "DropdownItem")
    void ApplyCornerStyling(ECornerStyle CornerStyle);

    /** Apply theme colors to this item */
    UFUNCTION(BlueprintCallable, Category = "DropdownItem")
    void ApplyThemeColors(bool bUseTheme, const UObject* WorldContextObject);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDropdownItemClicked, UGenericDropdownItem*, Item);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnDropdownItemClicked OnItemClicked;

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
    FDropdownItemConfig ItemConfig;

    /** Calculate corner radius based on corner style and item size */
    float GetCornerRadius(ECornerStyle CornerStyle) const;

    UFUNCTION()
    void OnButtonClicked();
};

/*====================================================================================================================================
                                                         GENERIC DROPDOWN WIDGET
======================================================================================================================================*/

UCLASS(Blueprintable, BlueprintType)
class GRIT_API UGenericDropdown : public UUserWidget
{
    GENERATED_BODY()

public:
    UGenericDropdown(const FObjectInitializer& ObjectInitializer);

    /** Toggle dropdown open/closed state */
    UFUNCTION(BlueprintCallable, Category = "Dropdown") 
    void Toggle();
    
    /** Set dropdown state explicitly */
    UFUNCTION(BlueprintCallable, Category = "Dropdown") 
    void SetOpen(bool bNewOpen);
    
    /** Add item from configuration struct */
    UFUNCTION(BlueprintCallable, Category = "Dropdown") 
    void AddItem(const FDropdownItemConfig& ItemConfig);
    
    /** Clear all items from list */
    UFUNCTION(BlueprintCallable, Category = "Dropdown") 
    void ClearItems();
    
    /** Set selected item by index */
    UFUNCTION(BlueprintCallable, Category = "Dropdown") 
    void SetSelectedIndex(int32 Index);
    
    /** Get currently selected index */
    UFUNCTION(BlueprintPure, Category = "Dropdown") 
    int32 GetSelectedIndex() const { return SelectedItemIndex; }

    /** Get if dropdown is open */
    UFUNCTION(BlueprintPure, Category = "Dropdown") 
    bool IsOpen() const { return bIsOpen; }

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnGenericDropdownSelectionChanged OnSelectionChanged;

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

    // FIXED: Use Overlay for proper Z-ordering instead of VerticalBox
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

    /** Array of item configs to populate on construction - SET THIS IN EDITOR! */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Items")
    TArray<FDropdownItemConfig> DefaultItems;

    /** Default item class for creating dropdown items */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Items")
    TSubclassOf<UGenericDropdownItem> ItemWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    float AnimDuration = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    EFlowCurve AnimCurve = EFlowCurve::QuadOut;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    bool bAnimateChevron = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    float ChevronAnimDuration = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    bool bUseSlideAnimation = true;  // Items slide down from behind title bar

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    bool bUseSizeAnimation = true;  // Animate size instead of scale transform

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    bool bAnimateWidth = false;  // Animate width (X-axis)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    bool bAnimateHeight = true;  // Animate height (Y-axis)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    FText TriggerText = FText::FromString("Choose option");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    int32 MaxVisibleItems = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    float ItemsPanelMaxHeight = 200.0f;

    /** Corner rounding style for dropdown items */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    ECornerStyle ItemCornerStyle = ECornerStyle::Slight;

    /** Corner rounding style for dropdown trigger */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    ECornerStyle TriggerCornerStyle = ECornerStyle::Medium;

    /** Apply theme colors to dropdown items */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    bool bUseThemeColors = true;

    //------------------------------------------------------------------------------
    // Runtime state
    //------------------------------------------------------------------------------
    bool bIsOpen = false;
    int32 SelectedItemIndex = -1;
    TArray<UGenericDropdownItem*> DropdownItems;

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
    void CalculateTargetHeight();
    
    /** Apply corner styling to dropdown item */
    void ApplyItemStyling(UGenericDropdownItem* Item);
    
    /** Apply corner styling to dropdown trigger */
    void ApplyTriggerStyling();

    UFUNCTION() 
    void OnItemClicked(UGenericDropdownItem* Item);

    UFUNCTION() 
    void OnTriggerClicked();
};