//GenericDropdownMenu.cpp
#include "GenericDropdownMenu.h"
#include "Components/BorderSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/OverlaySlot.h"
#include "TimerManager.h"

/*====================================================================================================================================
                                                         GENERIC DROPDOWN MENU IMPLEMENTATION
======================================================================================================================================*/

UGenericDropdownMenu::UGenericDropdownMenu(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , RootBorder(nullptr)
    , MainContainer(nullptr)
    , TriggerBorder(nullptr)
    , TriggerBox(nullptr)
    , TriggerLabel(nullptr)
    , ChevronIcon(nullptr)
    , TriggerButton(nullptr)
    , DropdownOverlay(nullptr)
    , ContentSizeBox(nullptr)
    , ContentPanel(nullptr)
    , ContentContainer(nullptr)
{
}

void UGenericDropdownMenu::NativePreConstruct()
{
    Super::NativePreConstruct();

    if (TriggerLabel)
    {
        TriggerLabel->SetText(TriggerText);
    }
}

void UGenericDropdownMenu::NativeConstruct()
{
    Super::NativeConstruct();

    ConfigureSlotProperties();
    
    // Apply corner styling and theme colors
    ApplyContentStyling();

    // Setup trigger
    if (TriggerLabel)
    {
        TriggerLabel->SetText(TriggerText);
    }

    if (TriggerButton)
    {
        TriggerButton->OnClicked.AddDynamic(this, &UGenericDropdownMenu::OnTriggerClicked);
    }

    // Setup content size box for animation
    if (ContentSizeBox)
    {
        ContentSizeBox->SetVisibility(ESlateVisibility::Hidden); // Hidden but maintains layout space
        ContentSizeBox->SetClipping(EWidgetClipping::Inherit);
        ContentSizeBox->SetRenderTransformPivot(FVector2D(0.5f, 0.0f));
        
        // Set initial size to 0 for animation
        if (bAnimateHeight)
        {
            ContentSizeBox->SetHeightOverride(0.0f);
        }
        if (bAnimateWidth)
        {
            ContentSizeBox->SetWidthOverride(0.0f);
        }
        
        // Initial transform for slide animation
        if (bUseSlideAnimation)
        {
            FWidgetTransform InitialTransform;
            InitialTransform.Translation = FVector2D(0.0f, -5.0f);
            InitialTransform.Scale = FVector2D(1.0f, 1.0f);
            ContentSizeBox->SetRenderTransform(InitialTransform);
        }
    }

    // Force one layout pass before any measurement
    InvalidateLayoutAndVolatility();
}

void UGenericDropdownMenu::NativeDestruct()
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
    
    Super::NativeDestruct();
}

void UGenericDropdownMenu::ConfigureSlotProperties()
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
    if (ContentSizeBox && DropdownOverlay)
    {
        if (UOverlaySlot* SizeBoxSlot = Cast<UOverlaySlot>(ContentSizeBox->Slot))
        {
            SizeBoxSlot->SetHorizontalAlignment(HAlign_Fill);
            SizeBoxSlot->SetVerticalAlignment(VAlign_Top);
            // Position below trigger
            SizeBoxSlot->SetPadding(FMargin(0.0f, 40.0f, 0.0f, 0.0f));
        }
    }
}

void UGenericDropdownMenu::Toggle()
{
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdownMenu::Toggle() - Current state: %s"), bIsOpen ? TEXT("OPEN") : TEXT("CLOSED"));
    SetOpen(!bIsOpen);
}

void UGenericDropdownMenu::SetOpen(bool bNewOpen)
{
    if (bIsOpen == bNewOpen) return;

    bIsOpen = bNewOpen;
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdownMenu::SetOpen(%s)"), bIsOpen ? TEXT("true") : TEXT("false"));

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
        // Calculate size right before opening - this is the key timing!
        CalculateContentSize();
        StartExpandAnimation();
        OnMenuOpenedBP();
    }
    else
    {
        StartCollapseAnimation();
        OnMenuClosedBP();
    }

    OnMenuToggled.Broadcast(bIsOpen);
}

void UGenericDropdownMenu::RecalculateContentSize()
{
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdownMenu::RecalculateContentSize() - Forcing size recalculation"));
    
    // Use the proper calculation method
    CalculateContentSize();
}

// Updated CalculateContentSize() method - Replace in GenericDropdownMenu.cpp

void UGenericDropdownMenu::CalculateContentSize()
{
    if (!ContentContainer || !ContentSizeBox)
    {
        TargetHeight = FallbackContentSize.Y;
        TargetWidth = FallbackContentSize.X;
        UE_LOG(LogTemp, Warning, TEXT("[DropdownMenu] Missing ContentContainer or ContentSizeBox - using fallback: %.1fx%.1f"), TargetWidth, TargetHeight);
        return;
    }

    // Force layout pass to get accurate measurements
    if (ContentContainer->GetParent())
    {
        ContentContainer->GetParent()->InvalidateLayoutAndVolatility();
    }

    const FVector2D Desired = ContentContainer->GetDesiredSize();

    //------------------------------------------------------------------------------
    // Apply Sizing Strategy
    //------------------------------------------------------------------------------
    if (true)//AutoSizeing Enabled(we dont need this it can be set  ttrue always but just add it in case)
    {
        // Reason: Use actual content size without artificial limits
        TargetWidth = Desired.X;
        TargetHeight = Desired.Y;
        UE_LOG(LogTemp, Warning, TEXT("[DropdownMenu] ✅ Auto-sized to content: %.1fx%.1f (Desired: %.1fx%.1f)"), TargetWidth, TargetHeight, Desired.X, Desired.Y);
    }
    else
    {
        // Reason: Clamp to manual max limits
        TargetWidth = FMath::Min(Desired.X, MaxContentWidth);
        TargetHeight = FMath::Min(Desired.Y, MaxContentHeight);
        UE_LOG(LogTemp, Warning, TEXT("[DropdownMenu] Clamped to max: %.1fx%.1f (Desired: %.1fx%.1f, Max: %.1fx%.1f)"), TargetWidth, TargetHeight, Desired.X, Desired.Y, MaxContentWidth, MaxContentHeight);
    }

    // Reason: Fallback if measurement failed
    if (TargetWidth <= 0.0f || TargetHeight <= 0.0f)
    {
        TargetWidth = FallbackContentSize.X;
        TargetHeight = FallbackContentSize.Y;
        UE_LOG(LogTemp, Warning, TEXT("[DropdownMenu] ❌ Invalid size - using fallback: %.1fx%.1f"), TargetWidth, TargetHeight);
    }
} // End CalculateContentSize

void UGenericDropdownMenu::StartExpandAnimation()
{
    if (!ContentSizeBox) return;

    ContentSizeBox->SetVisibility(ESlateVisibility::Visible);
    
    bAnimating = true;
    AnimTime = 0.0f;
    StartHeight = bAnimateHeight ? 0.0f : TargetHeight;
    StartWidth = bAnimateWidth ? 0.0f : TargetWidth;
    
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdownMenu: Starting expand animation to size %.1fx%.1f"), TargetWidth, TargetHeight);
    
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(AnimationTimer, this, &UGenericDropdownMenu::TickAnimation, 0.016f, true);
    }
}

void UGenericDropdownMenu::StartCollapseAnimation()
{
    if (!ContentSizeBox) return;

    bAnimating = true;
    AnimTime = 0.0f;
    StartHeight = bAnimateHeight ? TargetHeight : 0.0f;
    StartWidth = bAnimateWidth ? TargetWidth : 0.0f;
    
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdownMenu: Starting collapse animation from size %.1fx%.1f"), StartWidth, StartHeight);
    
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(AnimationTimer, this, &UGenericDropdownMenu::TickAnimation, 0.016f, true);
    }
}

void UGenericDropdownMenu::TickAnimation()
{
    AnimTime += 0.016f;
    float Progress = FMath::Clamp(AnimTime / AnimDuration, 0.0f, 1.0f);
    
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

    // Apply size animation (content gets clipped, not squished)
    if (ContentSizeBox)
    {
        if (bAnimateHeight)
        {
            ContentSizeBox->SetHeightOverride(CurrentHeight);
        }
        if (bAnimateWidth)
        {
            ContentSizeBox->SetWidthOverride(CurrentWidth);
        }
        
        // Optional slide effect without scale squishing
        if (bUseSlideAnimation)
        {
            FWidgetTransform SlideTransform;
            
            if (bIsOpen)
            {
                float YOffset = FMath::Lerp(-5.0f, 0.0f, EasedProgress);
                SlideTransform.Translation = FVector2D(0.0f, YOffset);
                SlideTransform.Scale = FVector2D(1.0f, 1.0f);
            }
            else
            {
                float YOffset = FMath::Lerp(0.0f, -5.0f, EasedProgress);
                SlideTransform.Translation = FVector2D(0.0f, YOffset);
                SlideTransform.Scale = FVector2D(1.0f, 1.0f);
            }
            
            ContentSizeBox->SetRenderTransform(SlideTransform);
        }
    }

    // End animation
    if (Progress >= 1.0f)
    {
        bAnimating = false;
        
        if (GetWorld() && AnimationTimer.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(AnimationTimer);
        }
        
        if (!bIsOpen && ContentSizeBox)
        {
            ContentSizeBox->SetVisibility(ESlateVisibility::Hidden);
        }
        
        UE_LOG(LogTemp, Log, TEXT("GenericDropdownMenu: Animation complete - Final size: %.1fx%.1f"), CurrentWidth, CurrentHeight);
    }
}

void UGenericDropdownMenu::AnimateChevron()
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
    
    UE_LOG(LogTemp, Log, TEXT("GenericDropdownMenu: Animating chevron from %.1f° to %.1f°"), StartAngle, TargetAngle);

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

void UGenericDropdownMenu::OnTriggerClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdownMenu::OnTriggerClicked()"));
    Toggle();
}

FVector2D UGenericDropdownMenu::DebugMeasureContentSize()
{
    if (!ContentContainer || !ContentPanel)
    {
        UE_LOG(LogTemp, Error, TEXT("GenericDropdownMenu::DebugMeasureContentSize - Missing widgets"));
        return FVector2D::ZeroVector;
    }

    // Force layout updates with proper UMG API
    if (ContentContainer->GetParent())
    {
        ContentContainer->GetParent()->InvalidateLayoutAndVolatility();
    }
    
    // Get measurements
    FVector2D ContainerSize = ContentContainer->GetDesiredSize();
    int32 ChildCount = ContentContainer->GetChildrenCount();
    
    UE_LOG(LogTemp, Warning, TEXT("GenericDropdownMenu::DebugMeasureContentSize Results:"));
    UE_LOG(LogTemp, Warning, TEXT("  Container Size (after InvalidateLayoutAndVolatility): %.1fx%.1f"), ContainerSize.X, ContainerSize.Y);
    UE_LOG(LogTemp, Warning, TEXT("  Child Count: %d"), ChildCount);
    
    for (int32 i = 0; i < ChildCount; ++i)
    {
        if (UWidget* Child = ContentContainer->GetChildAt(i))
        {
            FVector2D ChildSize = Child->GetDesiredSize();
            UE_LOG(LogTemp, Warning, TEXT("  Child %d (%s): %.1fx%.1f"), i, *Child->GetClass()->GetName(), ChildSize.X, ChildSize.Y);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("  Current Target: %.1fx%.1f"), TargetWidth, TargetHeight);
    
    return FVector2D(TargetWidth, TargetHeight);
}

float UGenericDropdownMenu::GetContentCornerRadius(const FVector2D& ContentSize) const
{
    if (ContentCornerStyle == ECornerStyle::None)
    {
        return 0.0f;
    }

    // Use the smaller dimension for radius calculation
    float MinDimension = FMath::Min(ContentSize.X, ContentSize.Y);
    if (MinDimension <= 0.0f)
    {
        MinDimension = 100.0f; // Fallback size for content panels
    }

    // Return percentage of smallest dimension (same as GenericButton)
    switch (ContentCornerStyle)
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

void UGenericDropdownMenu::ApplyContentStyling()
{
    if (!ContentPanel) return;

    // Get content size for corner radius calculation
    FVector2D ContentSize = FVector2D(MaxContentWidth, MaxContentHeight);
    if (ContentContainer)
    {
        FVector2D DesiredSize = ContentContainer->GetDesiredSize();
        if (!DesiredSize.IsZero())
        {
            ContentSize = DesiredSize;
        }
    }

    FSlateBrush BorderBrush;
    BorderBrush.DrawAs = (ContentCornerStyle == ECornerStyle::None) ? ESlateBrushDrawType::Box : ESlateBrushDrawType::RoundedBox;
    BorderBrush.TintColor = FSlateColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));

    float Radius = GetContentCornerRadius(ContentSize);
    BorderBrush.OutlineSettings.Width = 2.0f;
    BorderBrush.OutlineSettings.Color = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);
    BorderBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    BorderBrush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);

    ContentPanel->SetBrush(BorderBrush);

    // Apply theme colors if enabled
    if (bUseThemeColors)
    {
        // TODO: Apply theme colors - will be set via editor
        
        // Apply theme colors to trigger label
        if (TriggerLabel)
        {
            // TODO: Set trigger label styling via editor
        }
        
        // TODO: Set content panel background color via editor
        
        // Apply theme to trigger border if it exists
        if (TriggerBorder)
        {
            UUIToolkit::ApplyThemeToBorder(TriggerBorder, this);
        }
    }
}