//DropdownBase.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Styling/SlateBrush.h"
#include "DropdownItemBase.h"
#include "DropdownBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDropdownSelectionChanged, int32, SelectedIndex, const FString&, SelectedData);

/*====================================================================================================================================
                                                         EASING TYPES
======================================================================================================================================*/

UENUM(BlueprintType)
enum class EDropdownEasingType : uint8
{
    Linear          UMETA(DisplayName = "Linear (No Easing)"),
    EaseInOut       UMETA(DisplayName = "Ease In-Out (Smooth)"),
    EaseIn          UMETA(DisplayName = "Ease In (Slow Start)"),
    EaseOut         UMETA(DisplayName = "Ease Out (Slow End)"),
    EaseInOutCubic  UMETA(DisplayName = "Ease In-Out Cubic (Very Smooth)")
};

/*====================================================================================================================================
                                                         ANIMATION CONFIG
======================================================================================================================================*/

USTRUCT(BlueprintType)
struct FDropdownAnimConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
    float BaseDuration = 0.3f;  // [s] - Base animation length

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
    float StaggerDelay = 0.05f;  // [s] - Delay multiplier per item

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
    float CollapseOffset = -50.0f;  // [px] - Y offset when closed

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
    float CollapseScale = 0.95f;  // [dimensionless] - Scale when closed

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
    float ExpandScale = 1.0f;  // [dimensionless] - Scale when open

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Easing")
    float EaseExponent = 2.0f;  // [dimensionless] - Ease curve power (2.0 = quadratic)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Easing")
    EDropdownEasingType EasingType = EDropdownEasingType::EaseInOut;
};

/*====================================================================================================================================
                                                         ITEM ANIMATION STATE
======================================================================================================================================*/

USTRUCT()
struct FItemAnimState
{
    GENERATED_BODY()

    FTimerHandle AnimTimer;  // Timer handle for this item
    float ElapsedTime;  // [s] - Time since animation started
    float StartDelay;  // [s] - Initial delay before animating
    bool bIsAnimating;  // Animation active flag
    UDropdownItemBase* ItemWidget;  // Widget reference

    FItemAnimState() : ElapsedTime(0.0f), StartDelay(0.0f), bIsAnimating(false), ItemWidget(nullptr) {}
};

/*====================================================================================================================================
                                                         DROPDOWN BASE WIDGET
======================================================================================================================================*/

UCLASS(Blueprintable, BlueprintType)
class GRIT_API UDropdownBase : public UUserWidget
{
    GENERATED_BODY()

public:
    UDropdownBase(const FObjectInitializer& ObjectInitializer);

    /** Toggle dropdown open/closed state */
    UFUNCTION(BlueprintCallable, Category = "Dropdown") void Toggle();
    
    /** Set dropdown state explicitly */
    UFUNCTION(BlueprintCallable, Category = "Dropdown") void SetOpen(bool bNewOpen);
    
    /** Add item to dropdown list from string data */
    UFUNCTION(BlueprintCallable, Category = "Dropdown") void AddItem(const FString& ItemData);

    /** Add item to dropdown from specific Blueprint class */
    UFUNCTION(BlueprintCallable, Category = "Dropdown") void AddItemFromClass(TSubclassOf<UDropdownItemBase> ItemClass);
    
    /** Clear all items from list */
    UFUNCTION(BlueprintCallable, Category = "Dropdown") void ClearItems();
    
    /** Set selected item by index */
    UFUNCTION(BlueprintCallable, Category = "Dropdown") void SetSelectedIndex(int32 Index);
    
    /** Get currently selected index */
    UFUNCTION(BlueprintPure, Category = "Dropdown") int32 GetSelectedIndex() const { return SelectedItemIndex; }

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnDropdownSelectionChanged OnSelectionChanged;

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
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

protected:
    //------------------------------------------------------------------------------
    // Widget bindings (bound to Blueprint widgets)
    //------------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) UBorder* RootBorder;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) UVerticalBox* MainContainer;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) UBorder* TriggerBorder;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) UHorizontalBox* TriggerBox;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) UTextBlock* TriggerLabel;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) UImage* ChevronIcon;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) UVerticalBox* ItemsContainer;

    //------------------------------------------------------------------------------
    // Configuration - SET THESE IN THE BLUEPRINT EDITOR
    //------------------------------------------------------------------------------

    /** Array of item classes to populate on construction - SET THIS IN EDITOR! */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Items")
    TArray<TSubclassOf<UDropdownItemBase>> DefaultItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    FDropdownAnimConfig AnimConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    FText TriggerText = FText::FromString("Choose option");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    float ItemsZOrder = -1.0f;  // [dimensionless] - Items render behind trigger

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    int32 MaxVisibleItems = 8;  // Max items before needing scroll

    //------------------------------------------------------------------------------
    // Runtime state
    //------------------------------------------------------------------------------
    bool bIsOpen;  // Dropdown expanded state
    int32 SelectedItemIndex;  // Currently selected item
    TArray<FItemAnimState> ItemAnimStates;  // Animation state per item

    //------------------------------------------------------------------------------
    // Internal pipeline
    //------------------------------------------------------------------------------

    void ConfigureSlotProperties();
    void StartExpandAnimation();
    void StartCollapseAnimation();
    void AnimateItem(int32 ItemIndex);
    void StopAllAnimations();
    void UpdateItemTransform(UDropdownItemBase* Item, float Alpha, bool bExpanding);
    UFUNCTION() void OnItemClicked(UDropdownItemBase* Item);
    UFUNCTION() void OnTriggerClicked();
    float ApplyEasing(float t) const;
    float EaseInQuad(float t) const;
    float EaseOutQuad(float t) const;
    float EaseInOutQuad(float t) const;
    float EaseInOutCubic(float t) const;
};