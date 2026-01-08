//FilterDropdownBase.cpp
#include "FilterDropdownBase.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"

UFilterDropdownBase::UFilterDropdownBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , RootBorder(nullptr)
    , MainContainer(nullptr)
    , FilterChipsContainer(nullptr)
    , TriggerBorder(nullptr)
    , TriggerBox(nullptr)
    , TriggerLabel(nullptr)
    , ChevronIcon(nullptr)
    , ItemsContainer(nullptr)
    , bIsOpen(false)
{
}

void UFilterDropdownBase::NativePreConstruct()
{
    Super::NativePreConstruct();

    if (TriggerLabel)
    {
        TriggerLabel->SetText(TriggerText);
    }
}

void UFilterDropdownBase::NativeConstruct()
{
    Super::NativeConstruct();

    ConfigureSlotProperties();

    if (TriggerLabel)
    {
        TriggerLabel->SetText(TriggerText);
    }

    if (ItemsContainer)
    {
        ItemsContainer->SetVisibility(ESlateVisibility::Collapsed);
        ItemsContainer->SetRenderTransformPivot(FVector2D(0.5f, 0.0f));
    }

    if (TriggerBorder)
    {
        TriggerBorder->SetVisibility(ESlateVisibility::Visible);
    }

    // Populate default items
    for (const TSubclassOf<UFilterableItem>& ItemClass : DefaultItems)
    {
        if (ItemClass)
        {
            AddFilterableItem(ItemClass);
        }
    }
}

void UFilterDropdownBase::NativeDestruct()
{
    AvailableItems.Empty();
    ActiveFilters.Empty();
    Super::NativeDestruct();
}

void UFilterDropdownBase::ConfigureSlotProperties()
{
    // Configure TriggerLabel
    if (TriggerLabel)
    {
        if (UHorizontalBoxSlot* LabelSlot = Cast<UHorizontalBoxSlot>(TriggerLabel->Slot))
        {
            LabelSlot->SetHorizontalAlignment(HAlign_Left);
            LabelSlot->SetVerticalAlignment(VAlign_Center);
            LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    }

    // Configure ChevronIcon
    if (ChevronIcon)
    {
        if (UHorizontalBoxSlot* IconSlot = Cast<UHorizontalBoxSlot>(ChevronIcon->Slot))
        {
            IconSlot->SetHorizontalAlignment(HAlign_Right);
            IconSlot->SetVerticalAlignment(VAlign_Center);
            IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }

    // Configure FilterChipsContainer
    if (FilterChipsContainer)
    {
        if (UVerticalBoxSlot* ChipsSlot = Cast<UVerticalBoxSlot>(FilterChipsContainer->Slot))
        {
            ChipsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }

    // Configure TriggerBorder
    if (TriggerBorder)
    {
        if (UVerticalBoxSlot* TriggerSlot = Cast<UVerticalBoxSlot>(TriggerBorder->Slot))
        {
            TriggerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }

    // Configure ItemsContainer
    if (ItemsContainer)
    {
        if (UVerticalBoxSlot* ItemsSlot = Cast<UVerticalBoxSlot>(ItemsContainer->Slot))
        {
            ItemsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }
}

//------------------------------------------------------------------------------
//                                    Item Management
//------------------------------------------------------------------------------

void UFilterDropdownBase::AddFilterableItem(TSubclassOf<UFilterableItem> ItemClass)
{
    if (!ItemClass || !ItemsContainer)
    {
        UE_LOG(LogTemp, Error, TEXT("FilterDropdownBase: Invalid ItemClass or ItemsContainer"));
        return;
    }

    UFilterableItem* NewItem = CreateWidget<UFilterableItem>(this, ItemClass);

    if (NewItem)
    {
        int32 ItemIndex = AvailableItems.Num();
        
        // Get default item data via getter
        FString ItemData = NewItem->GetDefaultItemData();
        if (ItemData.IsEmpty())
        {
            ItemData = FString::Printf(TEXT("Item %d"), ItemIndex);
        }
        
        NewItem->InitItem(ItemData, ItemIndex);
        NewItem->SetOriginalIndex(ItemIndex);

        // Bind events
        NewItem->OnItemSelected.AddDynamic(this, &UFilterDropdownBase::OnItemSelected);
        NewItem->OnCollapseComplete.AddDynamic(this, &UFilterDropdownBase::OnItemCollapseComplete);
        NewItem->OnExpandComplete.AddDynamic(this, &UFilterDropdownBase::OnItemExpandComplete);

        // Add to container
        UVerticalBoxSlot* ItemSlot = ItemsContainer->AddChildToVerticalBox(NewItem);
        if (ItemSlot)
        {
            ItemSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            ItemSlot->SetPadding(FMargin(0.0f, 2.0f));
        }

        AvailableItems.Add(NewItem);

        UE_LOG(LogTemp, Warning, TEXT("FilterDropdownBase: Added item '%s' at index %d"), *ItemData, ItemIndex);
    }
}

void UFilterDropdownBase::ClearAllItems()
{
    if (ItemsContainer)
    {
        ItemsContainer->ClearChildren();
    }

    AvailableItems.Empty();
}

TArray<FString> UFilterDropdownBase::GetActiveFilters() const
{
    TArray<FString> Filters;

    for (UFilterEntry* Entry : ActiveFilters)
    {
        if (Entry)
        {
            Filters.Add(Entry->GetFilterData());
        }
    }

    return Filters;
}

void UFilterDropdownBase::ClearAllFilters()
{
    // Remove all filter chips and restore items
    TArray<UFilterEntry*> FiltersToRemove = ActiveFilters;

    for (UFilterEntry* Entry : FiltersToRemove)
    {
        if (Entry)
        {
            RestoreItemToDropdown(Entry);
        }
    }
}

//------------------------------------------------------------------------------
//                                    Dropdown Control
//------------------------------------------------------------------------------

void UFilterDropdownBase::Toggle()
{
    SetOpen(!bIsOpen);
}

void UFilterDropdownBase::SetOpen(bool bNewOpen)
{
    if (bIsOpen == bNewOpen) { return; }

    bIsOpen = bNewOpen;

    // Rotate chevron icon - 180° when open, 0° when closed
    if (ChevronIcon)
    {
        float TargetAngle = bIsOpen ? 180.0f : 0.0f;  // [deg]
        ChevronIcon->SetRenderTransformAngle(TargetAngle);
    } // End if (chevron rotation)

    if (bIsOpen)
    {
        ShowDropdownItems();
        OnDropdownOpenedBP();
    }
    else
    {
        HideDropdownItems();
        OnDropdownClosedBP();
    }
}

void UFilterDropdownBase::ShowDropdownItems()
{
    if (!ItemsContainer) { return; }

    ItemsContainer->SetVisibility(ESlateVisibility::Visible);

    // Simple fade in for now (can add stagger later)
    for (UFilterableItem* Item : AvailableItems)
    {
        if (Item)
        {
            Item->SetRenderOpacity(1.0f);
        }
    }
}

void UFilterDropdownBase::HideDropdownItems()
{
    if (!ItemsContainer) { return; }

    ItemsContainer->SetVisibility(ESlateVisibility::Collapsed);
}

//------------------------------------------------------------------------------
//                                    Event Handlers
//------------------------------------------------------------------------------

void UFilterDropdownBase::OnItemSelected(UDropdownItemBase* Item)
{
    UFilterableItem* FilterableItem = Cast<UFilterableItem>(Item);

    if (!FilterableItem) { return; }

    UE_LOG(LogTemp, Warning, TEXT("FilterDropdownBase: Item selected, starting collapse"));

    // Start collapse animation
    FilterableItem->StartCollapseAnimation();
}

void UFilterDropdownBase::OnItemCollapseComplete(UFilterableItem* Item)
{
    if (!Item) { return; }

    UE_LOG(LogTemp, Warning, TEXT("FilterDropdownBase: Collapse complete, creating filter chip"));

    // Remove from available items
    AvailableItems.Remove(Item);

    // Hide the item widget
    Item->SetVisibility(ESlateVisibility::Collapsed);

    // Create filter chip
    CreateFilterChip(Item);

    // Close dropdown
    SetOpen(false);
}

void UFilterDropdownBase::OnItemExpandComplete(UFilterableItem* Item)
{
    if (!Item) { return; }

    UE_LOG(LogTemp, Warning, TEXT("FilterDropdownBase: Expand complete"));

    // Item is now fully visible again in dropdown
}

void UFilterDropdownBase::OnFilterRemoveClicked(UFilterEntry* Entry)
{
    if (!Entry) { return; }

    UE_LOG(LogTemp, Warning, TEXT("FilterDropdownBase: Filter remove clicked"));

    RestoreItemToDropdown(Entry);
}

//------------------------------------------------------------------------------
//                                    Filter Management
//------------------------------------------------------------------------------

void UFilterDropdownBase::CreateFilterChip(UFilterableItem* FromItem)
{
    if (!FromItem || !FilterChipsContainer || !FilterEntryClass)
    {
        UE_LOG(LogTemp, Error, TEXT("FilterDropdownBase: Cannot create filter chip"));
        return;
    }

    UFilterEntry* NewChip = CreateWidget<UFilterEntry>(this, FilterEntryClass);

    if (NewChip)
    {
        // Initialize with data from item
        NewChip->InitFilter(FromItem->GetItemData(), FromItem->GetOriginalIndex());

        // Bind remove event
        NewChip->OnFilterRemoved.AddDynamic(this, &UFilterDropdownBase::OnFilterRemoveClicked);

        // Add to container
        FilterChipsContainer->AddChild(NewChip);

        // Play pop-in animation
        NewChip->PlayPopInAnimation();

        // Track active filter
        ActiveFilters.Add(NewChip);

        BroadcastFiltersChanged();

        UE_LOG(LogTemp, Warning, TEXT("FilterDropdownBase: Filter chip created"));
    }
}

void UFilterDropdownBase::RestoreItemToDropdown(UFilterEntry* FromFilter)
{
    if (!FromFilter) { return; }

    // Find the corresponding item widget
    int32 OriginalIndex = FromFilter->GetOriginalIndex();
    FString ItemData = FromFilter->GetFilterData();

    UFilterableItem* ItemToRestore = nullptr;

    // Find the item in ItemsContainer children (it's still there, just hidden)
    for (int32 i = 0; i < ItemsContainer->GetChildrenCount(); ++i)
    {
        UFilterableItem* ChildItem = Cast<UFilterableItem>(ItemsContainer->GetChildAt(i));

        if (ChildItem && ChildItem->GetOriginalIndex() == OriginalIndex)
        {
            ItemToRestore = ChildItem;
            break;
        }
    }

    if (ItemToRestore)
    {
        // Make visible and start expand animation
        ItemToRestore->SetVisibility(ESlateVisibility::Visible);
        ItemToRestore->StartExpandAnimation();

        // Re-add to available items in correct position
        int32 InsertIndex = 0;
        for (int32 i = 0; i < AvailableItems.Num(); ++i)
        {
            if (AvailableItems[i]->GetOriginalIndex() > OriginalIndex)
            {
                break;
            }
            InsertIndex = i + 1;
        }

        AvailableItems.Insert(ItemToRestore, InsertIndex);
    }

    // Remove filter chip
    ActiveFilters.Remove(FromFilter);
    FromFilter->RemoveFromParent();

    BroadcastFiltersChanged();
}

void UFilterDropdownBase::BroadcastFiltersChanged()
{
    TArray<FString> CurrentFilters = GetActiveFilters();
    OnFiltersChanged.Broadcast(CurrentFilters);
    OnFiltersChangedBP(CurrentFilters);
}

//------------------------------------------------------------------------------
//                                    Input Handling
//------------------------------------------------------------------------------

FReply UFilterDropdownBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (TriggerBorder)
        {
            FGeometry TriggerGeometry = TriggerBorder->GetCachedGeometry();
            FVector2D LocalMousePos = TriggerGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

            if (TriggerGeometry.GetLocalSize().X > 0 && TriggerGeometry.GetLocalSize().Y > 0)
            {
                if (LocalMousePos.X >= 0 && LocalMousePos.X <= TriggerGeometry.GetLocalSize().X &&
                    LocalMousePos.Y >= 0 && LocalMousePos.Y <= TriggerGeometry.GetLocalSize().Y)
                {
                    Toggle();
                    return FReply::Handled();
                }
            }
        }

        Toggle();
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
