//GenericDropdown.cpp
#include "GenericDropdown.h"
#include "Components/BorderSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/OverlaySlot.h"
#include "TimerManager.h"

/*====================================================================================================================================
                                                         DROPDOWN ITEM IMPLEMENTATION
======================================================================================================================================*/

UGenericDropdownItem::UGenericDropdownItem(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , ItemBorder(nullptr)
    , ItemLabel(nullptr)
    , ItemButton(nullptr)
{
}

void UGenericDropdownItem::NativeConstruct()
{
    Super::NativeConstruct();

    if (ItemButton)
    {
        ItemButton->OnClicked.AddDynamic(this, &UGenericDropdownItem::OnButtonClicked);
    }
}

void UGenericDropdownItem::InitItem(const FDropdownItemConfig& Config, int32 Index)
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

void UGenericDropdownItem::NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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

void UGenericDropdownItem::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
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

void UGenericDropdownItem::OnButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdownItem: Item %d clicked"), ItemIndex);
    OnItemClicked.Broadcast(this);
}

float UGenericDropdownItem::GetCornerRadius(ECornerStyle CornerStyle) const
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

void UGenericDropdownItem::ApplyCornerStyling(ECornerStyle CornerStyle)
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

void UGenericDropdownItem::ApplyThemeColors(bool bUseTheme, const UObject* WorldContextObject)
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
                                                         GENERIC DROPDOWN IMPLEMENTATION
======================================================================================================================================*/

UGenericDropdown::UGenericDropdown(const FObjectInitializer& ObjectInitializer)
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

void UGenericDropdown::NativePreConstruct()
{
    Super::NativePreConstruct();

    if (TriggerLabel)
    {
        TriggerLabel->SetText(TriggerText);
    }
}

void UGenericDropdown::NativeConstruct()
{
    Super::NativeConstruct();

    ConfigureSlotProperties();
    
    // Apply trigger styling
    ApplyTriggerStyling();

    // Setup trigger
    if (TriggerLabel)
    {
        TriggerLabel->SetText(TriggerText);
    }

    if (TriggerButton)
    {
        TriggerButton->OnClicked.AddDynamic(this, &UGenericDropdown::OnTriggerClicked);
    }

    // FIXED: Setup items panel for overlay rendering with slide effect
    if (ItemsSizeBox)
    {
        // Don't set to collapsed initially - let it calculate proper size first
        ItemsSizeBox->SetVisibility(ESlateVisibility::Hidden); // Hidden but still takes layout space
        // Don't clip - allow items to render outside bounds
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
        
        // Initial transform for slide animation - start behind title bar
        if (bUseSlideAnimation)
        {
            FWidgetTransform InitialTransform;
            InitialTransform.Translation = FVector2D(0.0f, -5.0f); // Start slightly behind
            InitialTransform.Scale = FVector2D(1.0f, 1.0f); // No initial scale squishing
            ItemsSizeBox->SetRenderTransform(InitialTransform);
        }
    }

    // Populate from DefaultItems array
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdown: Populating %d default items"), DefaultItems.Num());
    
    for (const FDropdownItemConfig& Config : DefaultItems)
    {
        AddItem(Config);
    }

    // FIXED: Delay target height calculation until after all items are added and layout is complete
    if (GetWorld() && DropdownItems.Num() > 0)
    {
        // Use a timer to ensure layout is complete before calculating size
        FTimerHandle DelayedSizeTimer;
        GetWorld()->GetTimerManager().SetTimer(
            DelayedSizeTimer,
            FTimerDelegate::CreateLambda([this]()
            {
                CalculateTargetHeight();
                
                // Apply the calculated size immediately (closed state)
                if (ItemsSizeBox)
                {
                    if (bAnimateHeight)
                    {
                        ItemsSizeBox->SetHeightOverride(0.0f); // Start closed
                    }
                    if (bAnimateWidth)
                    {
                        ItemsSizeBox->SetWidthOverride(0.0f); // Start closed
                    }
                }
                
                UE_LOG(LogTemp, Warning, TEXT("GenericDropdown: Delayed size calculation complete - Target: %.1fx%.1f"), TargetWidth, TargetHeight);
            }),
            0.1f, // Small delay to ensure layout is complete
            false
        );
    }
    else
    {
        // No items, calculate immediately
        CalculateTargetHeight();
    }
}

void UGenericDropdown::NativeDestruct()
{
    // Clean up animation timers
    if (GetWorld())
    {
        if (AnimationTimer.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(AnimationTimer);
        }
        if (ChevronAnimTimer.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(ChevronAnimTimer);
        }
    }
    
    DropdownItems.Empty();
    Super::NativeDestruct();
}

void UGenericDropdown::ConfigureSlotProperties()
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

    // FIXED: Configure overlay slot for proper Z-ordering
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

void UGenericDropdown::Toggle()
{
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdown::Toggle() - Current state: %s"), bIsOpen ? TEXT("OPEN") : TEXT("CLOSED"));
    SetOpen(!bIsOpen);
}

void UGenericDropdown::SetOpen(bool bNewOpen)
{
    if (bIsOpen == bNewOpen) return;

    bIsOpen = bNewOpen;
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdown::SetOpen(%s)"), bIsOpen ? TEXT("true") : TEXT("false"));

    // Animate chevron rotation
    if (bAnimateChevron)
    {
        AnimateChevron();
    }
    else if (ChevronIcon)
    {
        // Instant rotation if animation disabled
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

void UGenericDropdown::AddItem(const FDropdownItemConfig& ItemConfig)
{
    if (!ItemsContainer || !ItemWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("GenericDropdown::AddItem - ItemsContainer or ItemWidgetClass not set!"));
        return;
    }

    UGenericDropdownItem* NewItem = CreateWidget<UGenericDropdownItem>(this, ItemWidgetClass);
    if (!NewItem)
    {
        UE_LOG(LogTemp, Error, TEXT("GenericDropdown::AddItem - Failed to create item widget!"));
        return;
    }

    int32 ItemIndex = DropdownItems.Num();
    NewItem->InitItem(ItemConfig, ItemIndex);
    NewItem->OnItemClicked.AddDynamic(this, &UGenericDropdown::OnItemClicked);
    
    // Apply corner styling and theme colors
    ApplyItemStyling(NewItem);

    UVerticalBoxSlot* ItemSlot = ItemsContainer->AddChildToVerticalBox(NewItem);
    if (ItemSlot)
    {
        ItemSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        ItemSlot->SetPadding(FMargin(0.0f, 1.0f));
    }

    DropdownItems.Add(NewItem);
    
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdown: Added item %d: %s"), ItemIndex, *ItemConfig.Label.ToString());
    
    // Don't recalculate target height immediately - let it be done in batch after construction
}

void UGenericDropdown::ClearItems()
{
    if (ItemsContainer)
    {
        ItemsContainer->ClearChildren();
    }

    DropdownItems.Empty();
    SelectedItemIndex = -1;
    CalculateTargetHeight();
}

void UGenericDropdown::SetSelectedIndex(int32 Index)
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

void UGenericDropdown::CalculateTargetHeight()
{
    if (!ItemsContainer || DropdownItems.Num() == 0)
    {
        TargetHeight = 0.0f;
        TargetWidth = 100.0f; // Default width
        return;
    }

    // FIXED: Force layout update to get accurate measurements
    if (ItemsContainer->GetParent())
    {
        ItemsContainer->GetParent()->InvalidateLayoutAndVolatility();
    }

    // Calculate height based on actual content size when possible
    FVector2D ContainerDesiredSize = ItemsContainer->GetDesiredSize();
    
    if (ContainerDesiredSize.Y > 1.0f)
    {
        // Use actual measured height
        TargetHeight = FMath::Min(ContainerDesiredSize.Y, ItemsPanelMaxHeight);
    }
    else
    {
        // Fallback to estimated height
        float ItemHeight = 32.0f; // Estimated item height
        float TotalHeight = DropdownItems.Num() * ItemHeight;
        TargetHeight = FMath::Min(TotalHeight, ItemsPanelMaxHeight);
    }
    
    // Calculate width - use container's desired width or default
    if (ContainerDesiredSize.X > 1.0f)
    {
        TargetWidth = ContainerDesiredSize.X;
    }
    else
    {
        TargetWidth = 200.0f; // Default width
    }
    
    UE_LOG(LogTemp, Log, TEXT("GenericDropdown: Calculated target size: %.1fx%.1f (Items: %d, Measured: %.1fx%.1f)"), 
           TargetWidth, TargetHeight, DropdownItems.Num(), ContainerDesiredSize.X, ContainerDesiredSize.Y);
}

void UGenericDropdown::StartExpandAnimation()
{
    if (!ItemsSizeBox) return;

    // FIXED: Make visible but don't change size until animation starts
    ItemsSizeBox->SetVisibility(ESlateVisibility::Visible);
    
    bAnimating = true;
    AnimTime = 0.0f;
    StartHeight = bAnimateHeight ? 0.0f : TargetHeight;
    StartWidth = bAnimateWidth ? 0.0f : TargetWidth;
    
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdown: Starting expand animation to size %.1fx%.1f"), TargetWidth, TargetHeight);
    
    // Start animation timer at 60fps
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(AnimationTimer, this, &UGenericDropdown::TickAnimation, 0.016f, true);
    }
}

void UGenericDropdown::StartCollapseAnimation()
{
    if (!ItemsSizeBox) return;

    bAnimating = true;
    AnimTime = 0.0f;
    StartHeight = bAnimateHeight ? TargetHeight : 0.0f;
    StartWidth = bAnimateWidth ? TargetWidth : 0.0f;
    
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdown: Starting collapse animation from size %.1fx%.1f"), StartWidth, StartHeight);
    
    // Start animation timer at 60fps
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(AnimationTimer, this, &UGenericDropdown::TickAnimation, 0.016f, true);
    }
}

void UGenericDropdown::TickAnimation()
{
    AnimTime += 0.016f; // Fixed 60fps timestep
    float Progress = FMath::Clamp(AnimTime / AnimDuration, 0.0f, 1.0f);
    
    // Apply easing curve
    float EasedProgress = UUIToolkit::EvalFlowCurve(AnimCurve, Progress);
    
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

    // FIXED: Use size animation instead of scale transform (like GenericTray)
    if (ItemsSizeBox && bUseSizeAnimation)
    {
        // Set size overrides - content gets clipped, not squished
        if (bAnimateHeight)
        {
            ItemsSizeBox->SetHeightOverride(CurrentHeight);
        }
        if (bAnimateWidth)
        {
            ItemsSizeBox->SetWidthOverride(CurrentWidth);
        }
        
        // Optional: Add subtle slide effect without scale squishing
        if (bUseSlideAnimation)
        {
            FWidgetTransform SlideTransform;
            
            if (bIsOpen)
            {
                // Slide down from behind title bar (translation only, no scale)
                float YOffset = FMath::Lerp(-5.0f, 0.0f, EasedProgress);
                SlideTransform.Translation = FVector2D(0.0f, YOffset);
                SlideTransform.Scale = FVector2D(1.0f, 1.0f); // No scale squishing
            }
            else
            {
                // Slide up behind title bar
                float YOffset = FMath::Lerp(0.0f, -5.0f, EasedProgress);
                SlideTransform.Translation = FVector2D(0.0f, YOffset);
                SlideTransform.Scale = FVector2D(1.0f, 1.0f); // No scale squishing
            }
            
            ItemsSizeBox->SetRenderTransform(SlideTransform);
        }
    }

    // End animation
    if (Progress >= 1.0f)
    {
        bAnimating = false;
        
        // Clear timer
        if (GetWorld() && AnimationTimer.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(AnimationTimer);
        }
        
        if (!bIsOpen && ItemsSizeBox)
        {
            ItemsSizeBox->SetVisibility(ESlateVisibility::Hidden); // Hidden but maintains layout space
        }
        
        UE_LOG(LogTemp, Log, TEXT("GenericDropdown: Animation complete - Final size: %.1fx%.1f"), CurrentWidth, CurrentHeight);
    }
}

void UGenericDropdown::AnimateChevron()
{
    if (!ChevronIcon || !GetWorld()) return;

    // FIXED: Stop any existing chevron animation first to prevent jittering
    if (ChevronAnimTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(ChevronAnimTimer);
    }

    float StartAngle = ChevronIcon->GetRenderTransformAngle();
    float TargetAngle = bIsOpen ? 180.0f : 0.0f;
    
    // FIXED: Normalize angles to prevent jittering from intermediate values
    StartAngle = FMath::Fmod(StartAngle, 360.0f);
    if (StartAngle < 0.0f) StartAngle += 360.0f;
    
    UE_LOG(LogTemp, Log, TEXT("GenericDropdown: Animating chevron from %.1f° to %.1f°"), StartAngle, TargetAngle);

    // Use a lambda to capture the rotation animation
    float ChevronAnimTime = 0.0f;
    
    GetWorld()->GetTimerManager().SetTimer(
        ChevronAnimTimer,
        FTimerDelegate::CreateLambda([this, StartAngle, TargetAngle, &ChevronAnimTime]() mutable
        {
            ChevronAnimTime += 0.016f;
            float Progress = FMath::Clamp(ChevronAnimTime / ChevronAnimDuration, 0.0f, 1.0f);
            
            // Use smooth easing for chevron
            float EasedProgress = UUIToolkit::EvalFlowCurve(EFlowCurve::QuadOut, Progress);
            float CurrentAngle = FMath::Lerp(StartAngle, TargetAngle, EasedProgress);
            
            if (ChevronIcon)
            {
                ChevronIcon->SetRenderTransformAngle(CurrentAngle);
            }
            
            // End chevron animation
            if (Progress >= 1.0f)
            {
                // FIXED: Ensure final angle is exactly the target
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

void UGenericDropdown::OnTriggerClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdown::OnTriggerClicked()"));
    Toggle();
}

void UGenericDropdown::OnItemClicked(UGenericDropdownItem* Item)
{
    if (!Item) return;

    UE_LOG(LogTemp, Warning, TEXT("GenericDropdown: Item clicked at index %d"), Item->GetItemIndex());
    SetSelectedIndex(Item->GetItemIndex());
    SetOpen(false);
}

void UGenericDropdown::ApplyItemStyling(UGenericDropdownItem* Item)
{
    if (!Item) return;

    // Apply corner styling and theme colors via public methods
    Item->ApplyCornerStyling(ItemCornerStyle);
    Item->ApplyThemeColors(bUseThemeColors, this);
}

void UGenericDropdown::ApplyTriggerStyling()
{
    if (!TriggerBorder) return;

    // Get trigger size for corner radius calculation
    FVector2D TriggerSize = TriggerBorder->GetDesiredSize();
    if (TriggerSize.IsZero())
    {
        TriggerSize = FVector2D(200.0f, 40.0f); // Default trigger size
    }

    FSlateBrush BorderBrush;
    BorderBrush.DrawAs = (TriggerCornerStyle == ECornerStyle::None) ? ESlateBrushDrawType::Box : ESlateBrushDrawType::RoundedBox;
    BorderBrush.TintColor = FSlateColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));

    float MinDimension = FMath::Min(TriggerSize.X, TriggerSize.Y);
    if (MinDimension <= 0.0f) { MinDimension = 40.0f; }

    float Radius = 0.0f;
    switch (TriggerCornerStyle)
    {
        case ECornerStyle::None:     Radius = 0.0f; break;
        case ECornerStyle::Slight:   Radius = MinDimension * 0.1f; break;
        case ECornerStyle::Medium:   Radius = MinDimension * 0.2f; break;
        case ECornerStyle::Rounded:  Radius = MinDimension * 0.35f; break;
        case ECornerStyle::Pill:     Radius = MinDimension * 0.5f; break;
        default:                     Radius = MinDimension * 0.2f; break;
    }

    BorderBrush.OutlineSettings.Width = 2.0f;
    BorderBrush.OutlineSettings.Color = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);
    BorderBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    BorderBrush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);

    TriggerBorder->SetBrush(BorderBrush);

    // Apply theme colors if enabled
    if (bUseThemeColors)
    {
        // TODO: Apply theme colors - will be set via editor
        
        // Apply theme colors to trigger label
        if (TriggerLabel)
        {
            // TODO: Set trigger label styling via editor
        }
        
        // TODO: Set trigger background color via editor
    }
}