//DropdownBase.cpp
#include "DropdownBase.h"
#include "Styling/SlateBrush.h"
#include "Components/BorderSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "TimerManager.h"

/*====================================================================================================================================
                                                         INITIALIZATION
======================================================================================================================================*/

UDropdownBase::UDropdownBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , RootBorder(nullptr)
    , MainContainer(nullptr)
    , TriggerBorder(nullptr)
    , TriggerBox(nullptr)
    , TriggerLabel(nullptr)
    , ChevronIcon(nullptr)
    , ItemsContainer(nullptr)
    , bIsOpen(false)
    , SelectedItemIndex(-1)
{
}

void UDropdownBase::NativePreConstruct()
{
    Super::NativePreConstruct();

    // Update trigger label text in editor preview
    if (TriggerLabel)
    {
        TriggerLabel->SetText(TriggerText);
    }
}

void UDropdownBase::NativeConstruct()
{
    Super::NativeConstruct();

    ConfigureSlotProperties();

    // Update trigger label and hide items initially
    if (TriggerLabel)
    {
        TriggerLabel->SetText(TriggerText);
    }

    if (ItemsContainer)
    {
        ItemsContainer->SetVisibility(ESlateVisibility::Collapsed);
        ItemsContainer->SetRenderTransformPivot(FVector2D(0.5f, 0.0f));
        // FIXED: Don't clip dropdown items - let them render outside bounds
        ItemsContainer->SetClipping(EWidgetClipping::Inherit);
        
        // FIXED: Ensure dropdown items render on top
        ItemsContainer->SetRenderTransformPivot(FVector2D(0.5f, 0.0f));
    }

    // Make TriggerBorder clickable by setting its visibility to Visible
    if (TriggerBorder)
    {
        TriggerBorder->SetVisibility(ESlateVisibility::Visible);
    }

    // Populate from DefaultItems array set in editor
    UE_LOG(LogTemp, Warning, TEXT("DropdownBase: Populating %d default items"), DefaultItems.Num());
    
    for (const TSubclassOf<UDropdownItemBase>& ItemClass : DefaultItems)
    {
        if (ItemClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("DropdownBase: Adding item from class"));
            AddItemFromClass(ItemClass);
        }
    }
}

void UDropdownBase::NativeDestruct()
{
    StopAllAnimations();
    Super::NativeDestruct();
}

//------------------------------------------------------------------------------
//                                    Slot configuration
//------------------------------------------------------------------------------

void UDropdownBase::ConfigureSlotProperties()
{
    // Set TriggerLabel slot (child of TriggerBox)
    if (TriggerLabel)
    {
        if (UHorizontalBoxSlot* LabelSlot = Cast<UHorizontalBoxSlot>(TriggerLabel->Slot))
        {
            LabelSlot->SetHorizontalAlignment(HAlign_Left);
            LabelSlot->SetVerticalAlignment(VAlign_Center);
            LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    }

    // Set ChevronIcon slot (child of TriggerBox)
    if (ChevronIcon)
    {
        if (UHorizontalBoxSlot* IconSlot = Cast<UHorizontalBoxSlot>(ChevronIcon->Slot))
        {
            IconSlot->SetHorizontalAlignment(HAlign_Right);
            IconSlot->SetVerticalAlignment(VAlign_Center);
            IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }

    // Set TriggerBorder slot (child of MainContainer)
    if (TriggerBorder)
    {
        if (UVerticalBoxSlot* TriggerSlot = Cast<UVerticalBoxSlot>(TriggerBorder->Slot))
        {
            TriggerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }

    // Set ItemsContainer slot (child of MainContainer)
    if (ItemsContainer)
    {
        if (UVerticalBoxSlot* ItemsSlot = Cast<UVerticalBoxSlot>(ItemsContainer->Slot))
        {
            ItemsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }
}

//------------------------------------------------------------------------------
//                                    Item management pipeline
//------------------------------------------------------------------------------

void UDropdownBase::AddItem(const FString& ItemData)
{
    if (!ItemsContainer)
    {
        UE_LOG(LogTemp, Error, TEXT("DropdownBase::AddItem - ItemsContainer not bound!"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("DropdownBase::AddItem(FString) is deprecated. Use AddItemFromClass with configured Blueprint classes instead."));
}

void UDropdownBase::AddItemFromClass(TSubclassOf<UDropdownItemBase> ItemClass)
{
    // Requires valid item class and container
    if (!ItemClass || !ItemsContainer)
    {
        UE_LOG(LogTemp, Error, TEXT("DropdownBase::AddItemFromClass - Invalid ItemClass or ItemsContainer not bound!"));
        return;
    }

    UDropdownItemBase* NewItem = CreateWidget<UDropdownItemBase>(this, ItemClass);

    if (NewItem)
    {
        int32 ItemIndex = ItemsContainer->GetChildrenCount();
        NewItem->InitItem(NewItem->GetItemData(), ItemIndex);
        NewItem->OnItemSelected.AddDynamic(this, &UDropdownBase::OnItemClicked);

        UVerticalBoxSlot* ItemSlot = ItemsContainer->AddChildToVerticalBox(NewItem);
        if (ItemSlot)
        {
            ItemSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            ItemSlot->SetPadding(FMargin(0.0f, 2.0f));
        }

        // Setup animation state
        FItemAnimState AnimState;
        AnimState.ItemWidget = NewItem;
        AnimState.StartDelay = AnimConfig.StaggerDelay * ItemIndex;
        ItemAnimStates.Add(AnimState);

        // Initial collapsed transform - Alpha 1.0 with bExpanding=false gives collapsed state
        UpdateItemTransform(NewItem, 1.0f, false);
        
        UE_LOG(LogTemp, Warning, TEXT("DropdownBase: Successfully added item at index %d"), ItemIndex);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DropdownBase::AddItemFromClass - Failed to create widget from class!"));
    }
}

void UDropdownBase::ClearItems()
{
    StopAllAnimations();

    if (ItemsContainer)
    {
        ItemsContainer->ClearChildren();
    }

    ItemAnimStates.Empty();
    SelectedItemIndex = -1;
}

void UDropdownBase::SetSelectedIndex(int32 Index)
{
    if (Index < 0 || Index >= ItemAnimStates.Num()) { return; }

    SelectedItemIndex = Index;

    if (ItemAnimStates.IsValidIndex(Index) && ItemAnimStates[Index].ItemWidget)
    {
        FString SelectedData = ItemAnimStates[Index].ItemWidget->GetItemData();
        OnSelectionChanged.Broadcast(Index, SelectedData);
        OnSelectionChangedBP(Index, SelectedData);

        if (TriggerLabel)
        {
            TriggerLabel->SetText(FText::FromString(SelectedData));
        }
    }
}

//------------------------------------------------------------------------------
//                                    Animation pipeline
//------------------------------------------------------------------------------

void UDropdownBase::Toggle()
{
    UE_LOG(LogTemp, Warning, TEXT("DropdownBase::Toggle() called - Current state: %s"), bIsOpen ? TEXT("OPEN") : TEXT("CLOSED"));
    SetOpen(!bIsOpen);
}

void UDropdownBase::SetOpen(bool bNewOpen)
{
    if (bIsOpen == bNewOpen) { return; }

    bIsOpen = bNewOpen;
    UE_LOG(LogTemp, Warning, TEXT("DropdownBase::SetOpen(%s)"), bIsOpen ? TEXT("true") : TEXT("false"));

    // Rotate chevron icon - 180° when open, 0° when closed
    if (ChevronIcon)
    {
        float TargetAngle = bIsOpen ? 180.0f : 0.0f;  // [deg]
        ChevronIcon->SetRenderTransformAngle(TargetAngle);
    } // End if (chevron rotation)

    if (bIsOpen)
    {
        StartExpandAnimation();
        OnDropdownOpenedBP();
    }
    else
    {
        StartCollapseAnimation();
        OnDropdownClosedBP();
    }
}

void UDropdownBase::StartExpandAnimation()
{
    if (!ItemsContainer) { return; }

    ItemsContainer->SetVisibility(ESlateVisibility::Visible);
    
    // FIXED: Bring dropdown to front when expanding
    if (UPanelWidget* ParentPanel = GetParent())
    {
        // Move this dropdown to the end of parent's children (renders on top)
        ParentPanel->RemoveChild(this);
        ParentPanel->AddChild(this);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("DropdownBase: Starting expand animation for %d items"), ItemAnimStates.Num());

    for (int32 i = 0; i < ItemAnimStates.Num(); ++i)
    {
        ItemAnimStates[i].ElapsedTime = 0.0f;
        ItemAnimStates[i].bIsAnimating = true;

        FTimerDelegate TimerDelegate;
        TimerDelegate.BindUObject(this, &UDropdownBase::AnimateItem, i);

        GetWorld()->GetTimerManager().SetTimer(
            ItemAnimStates[i].AnimTimer,
            TimerDelegate,
            0.016f,
            true,
            ItemAnimStates[i].StartDelay
        );
    }
}

void UDropdownBase::StartCollapseAnimation()
{
    if (!ItemsContainer) { return; }

    UE_LOG(LogTemp, Warning, TEXT("DropdownBase: Starting collapse animation for %d items"), ItemAnimStates.Num());

    for (int32 i = 0; i < ItemAnimStates.Num(); ++i)
    {
        ItemAnimStates[i].ElapsedTime = 0.0f;
        ItemAnimStates[i].bIsAnimating = true;

        FTimerDelegate TimerDelegate;
        TimerDelegate.BindUObject(this, &UDropdownBase::AnimateItem, i);

        GetWorld()->GetTimerManager().SetTimer(
            ItemAnimStates[i].AnimTimer,
            TimerDelegate,
            0.016f,
            true,
            ItemAnimStates[i].StartDelay
        );
    }
}

void UDropdownBase::AnimateItem(int32 ItemIndex)
{
    if (!ItemAnimStates.IsValidIndex(ItemIndex)) { return; }

    FItemAnimState& AnimState = ItemAnimStates[ItemIndex];

    if (!AnimState.bIsAnimating || !AnimState.ItemWidget) { return; }

    AnimState.ElapsedTime += 0.016f;

    float Alpha = FMath::Clamp(AnimState.ElapsedTime / AnimConfig.BaseDuration, 0.0f, 1.0f);
    Alpha = ApplyEasing(Alpha);  // Use selected easing type

    UpdateItemTransform(AnimState.ItemWidget, Alpha, bIsOpen);

    // Stop animation when complete
    if (AnimState.ElapsedTime >= AnimConfig.BaseDuration)
    {
        AnimState.bIsAnimating = false;
        GetWorld()->GetTimerManager().ClearTimer(AnimState.AnimTimer);

        // Collapse visibility after close animation completes
        if (!bIsOpen && ItemIndex == ItemAnimStates.Num() - 1)
        {
            ItemsContainer->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UDropdownBase::UpdateItemTransform(UDropdownItemBase* Item, float Alpha, bool bExpanding)
{
    if (!Item) { return; }

    float CurrentAlpha = bExpanding ? Alpha : (1.0f - Alpha);

    // Lerp translation Y
    float TransY = FMath::Lerp(AnimConfig.CollapseOffset, 0.0f, CurrentAlpha);

    // Lerp scale
    float Scale = FMath::Lerp(AnimConfig.CollapseScale, AnimConfig.ExpandScale, CurrentAlpha);

    // Lerp opacity
    float Opacity = CurrentAlpha;

    FWidgetTransform Transform;
    Transform.Translation = FVector2D(0.0f, TransY);
    Transform.Scale = FVector2D(Scale, Scale);

    Item->SetRenderTransform(Transform);
    Item->SetRenderOpacity(Opacity);
}

void UDropdownBase::StopAllAnimations()
{
    if (!GetWorld()) { return; }

    for (FItemAnimState& AnimState : ItemAnimStates)
    {
        if (AnimState.AnimTimer.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(AnimState.AnimTimer);
        }
        AnimState.bIsAnimating = false;
    }
}

float UDropdownBase::ApplyEasing(float t) const
{
    switch (AnimConfig.EasingType)
    {
        case EDropdownEasingType::Linear:
            return t;

        case EDropdownEasingType::EaseIn:
            return EaseInQuad(t);

        case EDropdownEasingType::EaseOut:
            return EaseOutQuad(t);

        case EDropdownEasingType::EaseInOutCubic:
            return EaseInOutCubic(t);

        case EDropdownEasingType::EaseInOut:
        default:
            return EaseInOutQuad(t);
    }
}

float UDropdownBase::EaseInQuad(float t) const
{
    return t * t;
}

float UDropdownBase::EaseOutQuad(float t) const
{
    return 1.0f - (1.0f - t) * (1.0f - t);
}

float UDropdownBase::EaseInOutQuad(float t) const
{
    return t < 0.5f ? (2.0f * t * t) : (1.0f - FMath::Pow(-2.0f * t + 2.0f, 2.0f) / 2.0f);
}

float UDropdownBase::EaseInOutCubic(float t) const
{
    return t < 0.5f
        ? 4.0f * t * t * t
        : 1.0f - FMath::Pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

//------------------------------------------------------------------------------
//                                    Input event pipeline
//------------------------------------------------------------------------------

FReply UDropdownBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // Check if click is within TriggerBorder bounds
        if (TriggerBorder)
        {
            FGeometry TriggerGeometry = TriggerBorder->GetCachedGeometry();
            FVector2D LocalMousePos = TriggerGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
            
            if (TriggerGeometry.GetLocalSize().X > 0 && TriggerGeometry.GetLocalSize().Y > 0)
            {
                // Check if within bounds
                if (LocalMousePos.X >= 0 && LocalMousePos.X <= TriggerGeometry.GetLocalSize().X &&
                    LocalMousePos.Y >= 0 && LocalMousePos.Y <= TriggerGeometry.GetLocalSize().Y)
                {
                    UE_LOG(LogTemp, Warning, TEXT("DropdownBase: Trigger clicked!"));
                    Toggle();
                    return FReply::Handled();
                }
            }
        }
        
        // Fallback: just toggle if clicked anywhere on the widget
        UE_LOG(LogTemp, Warning, TEXT("DropdownBase: Widget clicked (fallback)!"));
        Toggle();
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UDropdownBase::OnTriggerClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("DropdownBase::OnTriggerClicked()"));
    Toggle();
}

void UDropdownBase::OnItemClicked(UDropdownItemBase* Item)
{
    if (!Item) { return; }

    UE_LOG(LogTemp, Warning, TEXT("DropdownBase: Item clicked at index %d"), Item->GetItemIndex());
    SetSelectedIndex(Item->GetItemIndex());
    SetOpen(false);
}