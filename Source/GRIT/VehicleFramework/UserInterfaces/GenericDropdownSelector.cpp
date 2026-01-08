//GenericDropdownSelector.cpp
#include "GenericDropdownSelector.h"
#include "Components/BorderSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/OverlaySlot.h"
#include "TimerManager.h"

/*====================================================================================================================================
                                                         DROPDOWN SELECTOR ITEM IMPLEMENTATION
======================================================================================================================================*/

UGenericDropdownSelectorItem::UGenericDropdownSelectorItem(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , ItemBorder(nullptr)
    , ItemLabel(nullptr)
    , ItemButton(nullptr)
{
}

void UGenericDropdownSelectorItem::NativeConstruct()
{
    Super::NativeConstruct();

    if (ItemButton)
    {
        ItemButton->OnClicked.AddDynamic(this, &UGenericDropdownSelectorItem::OnButtonClicked);
    }
}

void UGenericDropdownSelectorItem::InitItem(const FDropdownSelectorItemConfig& Config, int32 Index)
{
    ItemConfig = Config;
    ItemIndex = Index;
    ItemData = Config.Data;

    if (ItemLabel)
    {
        ItemLabel->SetText(Config.Label);
        ItemLabel->SetColorAndOpacity(Config.TextColor);
    }

    if (ItemBorder)
    {
        ItemBorder->SetBrushColor(FLinearColor::Transparent);
    }
}

void UGenericDropdownSelectorItem::NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    Super::NativeOnMouseEnter(MyGeometry, MouseEvent);

    if (ItemLabel)
    {
        ItemLabel->SetColorAndOpacity(ItemConfig.HoverTextColor);
    }

    if (ItemBorder)
    {
        ItemBorder->SetBrushColor(ItemConfig.HoverBackgroundColor);
    }
}

void UGenericDropdownSelectorItem::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
    Super::NativeOnMouseLeave(MouseEvent);

    if (ItemLabel)
    {
        ItemLabel->SetColorAndOpacity(ItemConfig.TextColor);
    }

    if (ItemBorder)
    {
        ItemBorder->SetBrushColor(FLinearColor::Transparent);
    }
}

void UGenericDropdownSelectorItem::OnButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdownSelectorItem: Item %d clicked"), ItemIndex);
    OnItemClicked.Broadcast(this);
}

float UGenericDropdownSelectorItem::GetCornerRadius(ECornerStyle CornerStyle) const
{
    if (CornerStyle == ECornerStyle::None)
    {
        return 0.0f;
    }

    // Get item size for radius calculation
    FVector2D ItemSize = GetDesiredSize();
    if (ItemSize.IsZero())
    {
        ItemSize = FVector2D(200.0f, 32.0f); // Default item size
    }

    // Use the smaller dimension for radius calculation
    float MinDimension = FMath::Min(ItemSize.X, ItemSize.Y);
    if (MinDimension <= 0.0f)
    {
        MinDimension = 32.0f; // Fallback size
    }

    // Return percentage of smallest dimension (same as GenericButton)
    switch (CornerStyle)
    {
        case ECornerStyle::None:
            return 0.0f;
        case ECornerStyle::Slight:
            return MinDimension * 0.1f;   // 10% of min dimension
        case ECornerStyle::Medium:
            return MinDimension * 0.2f;   // 20% of min dimension
        case ECornerStyle::Rounded:
            return MinDimension * 0.35f;  // 35% of min dimension
        case ECornerStyle::Pill:
            return MinDimension * 0.5f;   // 50% = full pill
        default:
            return MinDimension * 0.1f;   // Default to slight
    }
}

void UGenericDropdownSelectorItem::ApplyCornerStyling(ECornerStyle CornerStyle)
{
    if (!ItemBorder) return;

    FSlateBrush Brush;
    Brush.DrawAs = (CornerStyle == ECornerStyle::None) ? ESlateBrushDrawType::Box : ESlateBrushDrawType::RoundedBox;
    Brush.TintColor = FSlateColor(FLinearColor::Transparent);

    float Radius = GetCornerRadius(CornerStyle);
    Brush.OutlineSettings.Width = 0.0f;
    Brush.OutlineSettings.Color = FLinearColor::Transparent;
    Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);

    ItemBorder->SetBrush(Brush);
}

void UGenericDropdownSelectorItem::ApplyThemeColors(bool bUseTheme, const UObject* WorldContextObject)
{
    if (!bUseTheme || !WorldContextObject) return;

    // TODO: Apply theme colors - will be set via editor
    
    // Apply theme colors to item text
    if (ItemLabel)
    {
        // TODO: Set item label styling via editor
    }
    
    // Set border colors based on theme
    if (ItemBorder)
    {
        // TODO: Set item border color via editor
    }
}

/*====================================================================================================================================
                                                         GENERIC DROPDOWN SELECTOR IMPLEMENTATION
======================================================================================================================================*/

UGenericDropdownSelector::UGenericDropdownSelector(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , RootBorder(nullptr)
    , MainContainer(nullptr)
    , TriggerBorder(nullptr)
    , TriggerBox(nullptr)
    , TriggerLabel(nullptr)
    , ChevronIcon(nullptr)
    , TriggerButton(nullptr)
    , DropdownOverlay(nullptr)
    , ItemsSizeBox(nullptr)
    , ItemsPanel(nullptr)
    , ItemsContainer(nullptr)
{
}

void UGenericDropdownSelector::NativePreConstruct()
{
    Super::NativePreConstruct();

    if (TriggerLabel)
    {
        TriggerLabel->SetText(TriggerText);
    }
}

void UGenericDropdownSelector::NativeConstruct()
{
    Super::NativeConstruct();

    ConfigureSlotProperties();

    // Setup trigger
    if (TriggerLabel)
    {
        TriggerLabel->SetText(TriggerText);
    }

    if (TriggerButton)
    {
        TriggerButton->OnClicked.AddDynamic(this, &UGenericDropdownSelector::OnTriggerClicked);
    }

    // Setup items size box for animation
    if (ItemsSizeBox)
    {
        ItemsSizeBox->SetVisibility(ESlateVisibility::Hidden);
        ItemsSizeBox->SetClipping(EWidgetClipping::Inherit);
        ItemsSizeBox->SetRenderTransformPivot(FVector2D(0.5f, 0.0f));
        
        // Set initial size to 0 for animation
        if (bAnimateHeight)
        {
            ItemsSizeBox->SetHeightOverride(0.0f);
        }
        if (bAnimateWidth)
        {
            ItemsSizeBox->SetWidthOverride(0.0f);
        }
    }

    // Populate from DefaultItems array
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdownSelector: Populating %d default items"), DefaultItems.Num());
    
    for (const FDropdownSelectorItemConfig& Config : DefaultItems)
    {
        AddItem(Config);
    }

    // Calculate initial target size after construction
    if (GetWorld() && DropdownItems.Num() > 0)
    {
        FTimerHandle DelayedSizeTimer;
        GetWorld()->GetTimerManager().SetTimer(
            DelayedSizeTimer,
            FTimerDelegate::CreateLambda([this]()
            {
                CalculateTargetSize();
                
                // Apply the calculated size immediately (closed state)
                if (ItemsSizeBox)
                {
                    if (bAnimateHeight)
                    {
                        ItemsSizeBox->SetHeightOverride(0.0f);
                    }
                    if (bAnimateWidth)
                    {
                        ItemsSizeBox->SetWidthOverride(0.0f);
                    }
                }
                
                UE_LOG(LogTemp, Warning, TEXT("GenericDropdownSelector: Delayed size calculation complete - Target: %.1fx%.1f"), TargetWidth, TargetHeight);
            }),
            0.1f,
            false
        );
    }
    else
    {
        CalculateTargetSize();
    }
}

void UGenericDropdownSelector::NativeDestruct()
{
    StopAllItemAnimations();
    
    // Clean up animation timers
    if (GetWorld())
    {
        if (SizeAnimationTimer.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(SizeAnimationTimer);
        }
        if (ChevronAnimTimer.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(ChevronAnimTimer);
        }
    }
    
    DropdownItems.Empty();
    ItemAnimStates.Empty();
    Super::NativeDestruct();
}

void UGenericDropdownSelector::ConfigureSlotProperties()
{
    // Configure trigger elements
    if (TriggerLabel && TriggerBox)
    {
        if (UHorizontalBoxSlot* LabelSlot = Cast<UHorizontalBoxSlot>(TriggerLabel->Slot))
        {
            LabelSlot->SetHorizontalAlignment(HAlign_Left);
            LabelSlot->SetVerticalAlignment(VAlign_Center);
            LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    }

    if (ChevronIcon && TriggerBox)
    {
        if (UHorizontalBoxSlot* IconSlot = Cast<UHorizontalBoxSlot>(ChevronIcon->Slot))
        {
            IconSlot->SetHorizontalAlignment(HAlign_Right);
            IconSlot->SetVerticalAlignment(VAlign_Center);
            IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }

    // Configure overlay slot for proper Z-ordering
    if (ItemsSizeBox && DropdownOverlay)
    {
        if (UOverlaySlot* SizeBoxSlot = Cast<UOverlaySlot>(ItemsSizeBox->Slot))
        {
            SizeBoxSlot->SetHorizontalAlignment(HAlign_Fill);
            SizeBoxSlot->SetVerticalAlignment(VAlign_Top);
            // Position below trigger
            SizeBoxSlot->SetPadding(FMargin(0.0f, 40.0f, 0.0f, 0.0f));
        }
    }
}

void UGenericDropdownSelector::Toggle()
{
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdownSelector::Toggle() - Current state: %s"), bIsOpen ? TEXT("OPEN") : TEXT("CLOSED"));
    SetOpen(!bIsOpen);
}

void UGenericDropdownSelector::SetOpen(bool bNewOpen)
{
    if (bIsOpen == bNewOpen) return;

    bIsOpen = bNewOpen;
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdownSelector::SetOpen(%s)"), bIsOpen ? TEXT("true") : TEXT("false"));

    // Animate chevron rotation
    if (bAnimateChevron)
    {
        AnimateChevron();
    }
    else if (ChevronIcon)
    {
        float TargetAngle = bIsOpen ? 180.0f : 0.0f;
        ChevronIcon->SetRenderTransformAngle(TargetAngle);
    }

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

void UGenericDropdownSelector::AddItem(const FDropdownSelectorItemConfig& ItemConfig)
{
    if (!ItemsContainer || !ItemWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("GenericDropdownSelector::AddItem - ItemsContainer or ItemWidgetClass not set!"));
        return;
    }

    UGenericDropdownSelectorItem* NewItem = CreateWidget<UGenericDropdownSelectorItem>(this, ItemWidgetClass);
    if (!NewItem)
    {
        UE_LOG(LogTemp, Error, TEXT("GenericDropdownSelector::AddItem - Failed to create item widget!"));
        return;
    }

    int32 ItemIndex = DropdownItems.Num();
    NewItem->InitItem(ItemConfig, ItemIndex);
    NewItem->OnItemClicked.AddDynamic(this, &UGenericDropdownSelector::OnItemClicked);
    
    // Apply corner styling and theme colors
    ApplyItemStyling(NewItem);

    UVerticalBoxSlot* ItemSlot = ItemsContainer->AddChildToVerticalBox(NewItem);
    if (ItemSlot)
    {
        ItemSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        ItemSlot->SetPadding(FMargin(0.0f, 1.0f));
    }

    DropdownItems.Add(NewItem);
    
    // Setup animation state
    FItemAnimationState AnimState;
    AnimState.ItemWidget = NewItem;
    AnimState.StartDelay = ItemStaggerDelay * ItemIndex;
    ItemAnimStates.Add(AnimState);

    // Initial collapsed transform
    UpdateItemTransform(NewItem, 1.0f, false);
    
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdownSelector: Added item %d: %s"), ItemIndex, *ItemConfig.Label.ToString());
}

void UGenericDropdownSelector::ClearItems()
{
    StopAllItemAnimations();

    if (ItemsContainer)
    {
        ItemsContainer->ClearChildren();
    }

    DropdownItems.Empty();
    ItemAnimStates.Empty();
    SelectedItemIndex = -1;
    CalculateTargetSize();
}

void UGenericDropdownSelector::SetSelectedIndex(int32 Index)
{
    if (Index < 0 || Index >= DropdownItems.Num()) return;

    SelectedItemIndex = Index;

    if (DropdownItems.IsValidIndex(Index))
    {
        FString SelectedData = DropdownItems[Index]->GetItemData();
        OnSelectionChanged.Broadcast(Index, SelectedData);
        OnSelectionChangedBP(Index, SelectedData);

        if (TriggerLabel)
        {
            TriggerLabel->SetText(FText::FromString(SelectedData));
        }
    }
}

void UGenericDropdownSelector::CalculateTargetSize()
{
    if (!ItemsContainer || DropdownItems.Num() == 0)
    {
        TargetHeight = 0.0f;
        TargetWidth = 200.0f;
        return;
    }

    // Force layout update to get accurate measurements
    if (ItemsContainer->GetParent())
    {
        ItemsContainer->GetParent()->InvalidateLayoutAndVolatility();
    }

    // Calculate height based on actual content size when possible
    FVector2D ContainerDesiredSize = ItemsContainer->GetDesiredSize();
    
    if (ContainerDesiredSize.Y > 1.0f)
    {
        TargetHeight = FMath::Min(ContainerDesiredSize.Y, ItemsPanelMaxHeight);
    }
    else
    {
        // Fallback to estimated height
        float ItemHeight = 32.0f;
        float TotalHeight = DropdownItems.Num() * ItemHeight;
        TargetHeight = FMath::Min(TotalHeight, ItemsPanelMaxHeight);
    }
    
    if (ContainerDesiredSize.X > 1.0f)
    {
        TargetWidth = ContainerDesiredSize.X;
    }
    else
    {
        TargetWidth = 200.0f;
    }
    
    UE_LOG(LogTemp, Log, TEXT("GenericDropdownSelector: Calculated target size: %.1fx%.1f (Items: %d, Measured: %.1fx%.1f)"), 
           TargetWidth, TargetHeight, DropdownItems.Num(), ContainerDesiredSize.X, ContainerDesiredSize.Y);
}

void UGenericDropdownSelector::StartExpandAnimation()
{
    if (!ItemsSizeBox) return;

    ItemsSizeBox->SetVisibility(ESlateVisibility::Visible);
    
    // Start size animation
    bSizeAnimating = true;
    SizeAnimTime = 0.0f;
    StartHeight = bAnimateHeight ? 0.0f : TargetHeight;
    StartWidth = bAnimateWidth ? 0.0f : TargetWidth;
    
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdownSelector: Starting expand animation to size %.1fx%.1f"), TargetWidth, TargetHeight);
    
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(SizeAnimationTimer, this, &UGenericDropdownSelector::TickSizeAnimation, 0.016f, true);
    }

    // Start item animations
    StartItemAnimations(true);
}

void UGenericDropdownSelector::StartCollapseAnimation()
{
    if (!ItemsSizeBox) return;

    bSizeAnimating = true;
    SizeAnimTime = 0.0f;
    StartHeight = bAnimateHeight ? TargetHeight : 0.0f;
    StartWidth = bAnimateWidth ? TargetWidth : 0.0f;
    
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdownSelector: Starting collapse animation from size %.1fx%.1f"), StartWidth, StartHeight);
    
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(SizeAnimationTimer, this, &UGenericDropdownSelector::TickSizeAnimation, 0.016f, true);
    }

    // Start item animations
    StartItemAnimations(false);
}

void UGenericDropdownSelector::TickSizeAnimation()
{
    SizeAnimTime += 0.016f;
    float Progress = FMath::Clamp(SizeAnimTime / SizeAnimDuration, 0.0f, 1.0f);
    
    float EasedProgress = UUIToolkit::EvalFlowCurve(SizeAnimCurve, Progress);
    
    float CurrentHeight, CurrentWidth;
    
    if (bIsOpen)
    {
        CurrentHeight = bAnimateHeight ? FMath::Lerp(StartHeight, TargetHeight, EasedProgress) : TargetHeight;
        CurrentWidth = bAnimateWidth ? FMath::Lerp(StartWidth, TargetWidth, EasedProgress) : TargetWidth;
    }
    else
    {
        CurrentHeight = bAnimateHeight ? FMath::Lerp(StartHeight, 0.0f, EasedProgress) : 0.0f;
        CurrentWidth = bAnimateWidth ? FMath::Lerp(StartWidth, 0.0f, EasedProgress) : TargetWidth;
    }

    // Apply size animation
    if (ItemsSizeBox)
    {
        if (bAnimateHeight)
        {
            ItemsSizeBox->SetHeightOverride(CurrentHeight);
        }
        if (bAnimateWidth)
        {
            ItemsSizeBox->SetWidthOverride(CurrentWidth);
        }
    }

    // End animation
    if (Progress >= 1.0f)
    {
        bSizeAnimating = false;
        
        if (GetWorld() && SizeAnimationTimer.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(SizeAnimationTimer);
        }
        
        if (!bIsOpen && ItemsSizeBox)
        {
            ItemsSizeBox->SetVisibility(ESlateVisibility::Hidden);
        }
        
        UE_LOG(LogTemp, Log, TEXT("GenericDropdownSelector: Size animation complete - Final size: %.1fx%.1f"), CurrentWidth, CurrentHeight);
    }
}

void UGenericDropdownSelector::StartItemAnimations(bool bExpanding)
{
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdownSelector: Starting %s item animations for %d items"), 
           bExpanding ? TEXT("expand") : TEXT("collapse"), ItemAnimStates.Num());

    for (int32 i = 0; i < ItemAnimStates.Num(); ++i)
    {
        ItemAnimStates[i].ElapsedTime = 0.0f;
        ItemAnimStates[i].bIsAnimating = true;

        FTimerDelegate TimerDelegate;
        TimerDelegate.BindUObject(this, &UGenericDropdownSelector::AnimateItem, i, bExpanding);

        GetWorld()->GetTimerManager().SetTimer(
            ItemAnimStates[i].AnimTimer,
            TimerDelegate,
            0.016f,
            true,
            ItemAnimStates[i].StartDelay
        );
    }
}

void UGenericDropdownSelector::AnimateItem(int32 ItemIndex, bool bExpanding)
{
    if (!ItemAnimStates.IsValidIndex(ItemIndex)) return;

    FItemAnimationState& AnimState = ItemAnimStates[ItemIndex];

    if (!AnimState.bIsAnimating || !AnimState.ItemWidget) return;

    AnimState.ElapsedTime += 0.016f;

    float Alpha = FMath::Clamp(AnimState.ElapsedTime / ItemAnimDuration, 0.0f, 1.0f);
    Alpha = UUIToolkit::EvalFlowCurve(ItemAnimCurve, Alpha);

    UpdateItemTransform(AnimState.ItemWidget, Alpha, bExpanding);

    // Stop animation when complete
    if (AnimState.ElapsedTime >= ItemAnimDuration)
    {
        AnimState.bIsAnimating = false;
        GetWorld()->GetTimerManager().ClearTimer(AnimState.AnimTimer);

        // Collapse visibility after close animation completes
        if (!bExpanding && ItemIndex == ItemAnimStates.Num() - 1)
        {
            // Last item finished collapsing
        }
    }
}

void UGenericDropdownSelector::UpdateItemTransform(UGenericDropdownSelectorItem* Item, float Alpha, bool bExpanding)
{
    if (!Item) return;

    float CurrentAlpha = bExpanding ? Alpha : (1.0f - Alpha);

    // Lerp translation Y
    float TransY = FMath::Lerp(ItemCollapseOffset, 0.0f, CurrentAlpha);

    // Lerp scale
    float Scale = FMath::Lerp(ItemCollapseScale, 1.0f, CurrentAlpha);

    // Lerp opacity
    float Opacity = CurrentAlpha;

    FWidgetTransform Transform;
    Transform.Translation = FVector2D(0.0f, TransY);
    Transform.Scale = FVector2D(Scale, Scale);

    Item->SetRenderTransform(Transform);
    Item->SetRenderOpacity(Opacity);
}

void UGenericDropdownSelector::StopAllItemAnimations()
{
    if (!GetWorld()) return;

    for (FItemAnimationState& AnimState : ItemAnimStates)
    {
        if (AnimState.AnimTimer.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(AnimState.AnimTimer);
        }
        AnimState.bIsAnimating = false;
    }
}

void UGenericDropdownSelector::AnimateChevron()
{
    if (!ChevronIcon || !GetWorld()) return;

    // Stop any existing chevron animation first
    if (ChevronAnimTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(ChevronAnimTimer);
    }

    float StartAngle = ChevronIcon->GetRenderTransformAngle();
    float TargetAngle = bIsOpen ? 180.0f : 0.0f;
    
    // Normalize angles to prevent jittering
    StartAngle = FMath::Fmod(StartAngle, 360.0f);
    if (StartAngle < 0.0f) StartAngle += 360.0f;
    
    UE_LOG(LogTemp, Log, TEXT("GenericDropdownSelector: Animating chevron from %.1f° to %.1f°"), StartAngle, TargetAngle);

    float ChevronAnimTime = 0.0f;
    
    GetWorld()->GetTimerManager().SetTimer(
        ChevronAnimTimer,
        FTimerDelegate::CreateLambda([this, StartAngle, TargetAngle, &ChevronAnimTime]() mutable
        {
            ChevronAnimTime += 0.016f;
            float Progress = FMath::Clamp(ChevronAnimTime / ChevronAnimDuration, 0.0f, 1.0f);
            
            float EasedProgress = UUIToolkit::EvalFlowCurve(EFlowCurve::QuadOut, Progress);
            float CurrentAngle = FMath::Lerp(StartAngle, TargetAngle, EasedProgress);
            
            if (ChevronIcon)
            {
                ChevronIcon->SetRenderTransformAngle(CurrentAngle);
            }
            
            if (Progress >= 1.0f)
            {
                if (ChevronIcon)
                {
                    ChevronIcon->SetRenderTransformAngle(TargetAngle);
                }
                
                if (GetWorld() && ChevronAnimTimer.IsValid())
                {
                    GetWorld()->GetTimerManager().ClearTimer(ChevronAnimTimer);
                }
            }
        }),
        0.016f,
        true
    );
}

void UGenericDropdownSelector::OnTriggerClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdownSelector::OnTriggerClicked()"));
    Toggle();
}

void UGenericDropdownSelector::OnItemClicked(UGenericDropdownSelectorItem* Item)
{
    if (!Item) return;

    UE_LOG(LogTemp, Warning, TEXT("GenericDropdownSelector: Item clicked at index %d"), Item->GetItemIndex());
    SetSelectedIndex(Item->GetItemIndex());
    SetOpen(false);
}

float UGenericDropdownSelector::GetItemCornerRadius(const FVector2D& ItemSize) const
{
    if (ItemCornerStyle == ECornerStyle::None)
    {
        return 0.0f;
    }

    // Use the smaller dimension for radius calculation
    float MinDimension = FMath::Min(ItemSize.X, ItemSize.Y);
    if (MinDimension <= 0.0f)
    {
        MinDimension = 32.0f; // Fallback size
    }

    // Return percentage of smallest dimension (same as GenericButton)
    switch (ItemCornerStyle)
    {
        case ECornerStyle::None:
            return 0.0f;
        case ECornerStyle::Slight:
            return MinDimension * 0.1f;   // 10% of min dimension
        case ECornerStyle::Medium:
            return MinDimension * 0.2f;   // 20% of min dimension
        case ECornerStyle::Rounded:
            return MinDimension * 0.35f;  // 35% of min dimension
        case ECornerStyle::Pill:
            return MinDimension * 0.5f;   // 50% = full pill
        default:
            return MinDimension * 0.1f;   // Default to slight
    }
}

void UGenericDropdownSelector::ApplyItemStyling(UGenericDropdownSelectorItem* Item)
{
    if (!Item) return;

    // Apply corner styling and theme colors via public methods
    Item->ApplyCornerStyling(ItemCornerStyle);
    Item->ApplyThemeColors(bUseThemeColors, this);
}