//FilterDropdownBase.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/WrapBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "FilterableItem.h"
#include "FilterEntry.h"
#include "FilterDropdownBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFiltersChanged, const TArray<FString>&, ActiveFilters);

/*====================================================================================================================================
                                                         FILTER DROPDOWN BASE
======================================================================================================================================*/

UCLASS(Blueprintable, BlueprintType)
class GRIT_API UFilterDropdownBase : public UUserWidget
{
    GENERATED_BODY()

public:
    UFilterDropdownBase(const FObjectInitializer& ObjectInitializer);

    /** Toggle dropdown open/closed */
    UFUNCTION(BlueprintCallable, Category = "Dropdown")
    void Toggle();

    /** Set dropdown state */
    UFUNCTION(BlueprintCallable, Category = "Dropdown")
    void SetOpen(bool bNewOpen);

    /** Add filterable item from class */
    UFUNCTION(BlueprintCallable, Category = "Dropdown")
    void AddFilterableItem(TSubclassOf<UFilterableItem> ItemClass);

    /** Clear all items */
    UFUNCTION(BlueprintCallable, Category = "Dropdown")
    void ClearAllItems();

    /** Get active filters */
    UFUNCTION(BlueprintPure, Category = "Filter")
    TArray<FString> GetActiveFilters() const;

    /** Remove all filters */
    UFUNCTION(BlueprintCallable, Category = "Filter")
    void ClearAllFilters();

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnFiltersChanged OnFiltersChanged;

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnFiltersChangedBP(const TArray<FString>& Filters);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnDropdownOpenedBP();

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnDropdownClosedBP();

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    //------------------------------------------------------------------------------
    // Widget bindings
    //------------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UBorder* RootBorder;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox* MainContainer;

    // Filter chips area
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UWrapBox* FilterChipsContainer;

    // Dropdown trigger
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UBorder* TriggerBorder;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UHorizontalBox* TriggerBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* TriggerLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage* ChevronIcon;

    // Items container
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox* ItemsContainer;

    //------------------------------------------------------------------------------
    // Configuration
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Items")
    TArray<TSubclassOf<UFilterableItem>> DefaultItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Filter")
    TSubclassOf<UFilterEntry> FilterEntryClass;  // Blueprint class for filter chips

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    FText TriggerText = FText::FromString("Add filter");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    float ItemStaggerDelay = 0.05f;  // [s]

private:
    bool bIsOpen = false;

    // Track items in dropdown and filters
    TArray<UFilterableItem*> AvailableItems;  // Items still in dropdown
    TArray<UFilterEntry*> ActiveFilters;      // Filter chips

    // Internal functions
    void ConfigureSlotProperties();
    void ShowDropdownItems();
    void HideDropdownItems();

    UFUNCTION()
    void OnItemSelected(UDropdownItemBase* Item);

    UFUNCTION()
    void OnItemCollapseComplete(UFilterableItem* Item);

    UFUNCTION()
    void OnItemExpandComplete(UFilterableItem* Item);

    UFUNCTION()
    void OnFilterRemoveClicked(UFilterEntry* Entry);

    void CreateFilterChip(UFilterableItem* FromItem);
    void RestoreItemToDropdown(UFilterEntry* FromFilter);
    void BroadcastFiltersChanged();
};
