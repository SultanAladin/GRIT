// GenericButton.cpp
#include "GenericButton.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "TimerManager.h"

UGenericButton::UGenericButton(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UGenericButton::NativePreConstruct()
{
    Super::NativePreConstruct();
    ApplyButtonStyling();
    ApplyContent();
}

void UGenericButton::NativeConstruct()
{
    Super::NativeConstruct();

    if (MainButton)
    {
        OriginalScale = MainButton->GetRenderTransform().Scale;
        if (OriginalScale.IsZero()) { OriginalScale = FVector2D(1.0f, 1.0f); }
        StartScale = OriginalScale;
        TargetScale = OriginalScale;

        ApplyButtonStyling();
        ApplyContent();

        // Initialize color transition state
        GetTargetColors(StartOutlineColor, StartBackgroundColor);
        TargetOutlineColor = StartOutlineColor;
        TargetBackgroundColor = StartBackgroundColor;

        MainButton->OnHovered.AddDynamic(this, &UGenericButton::OnButtonHovered);
        MainButton->OnUnhovered.AddDynamic(this, &UGenericButton::OnButtonUnhovered);
        MainButton->OnClicked.AddDynamic(this, &UGenericButton::OnButtonClicked);
        MainButton->OnPressed.AddDynamic(this, &UGenericButton::OnButtonPressed);
        MainButton->OnReleased.AddDynamic(this, &UGenericButton::OnButtonReleased);
    }
}

void UGenericButton::NativeDestruct()
{
    RemoveTooltip();

    // Clear all timers
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ScaleAnimTimer);
        GetWorld()->GetTimerManager().ClearTimer(RotationAnimTimer);
        GetWorld()->GetTimerManager().ClearTimer(ColorAnimTimer);
        GetWorld()->GetTimerManager().ClearTimer(TooltipDelayTimer);
    }

    if (MainButton)
    {
        MainButton->OnHovered.RemoveAll(this);
        MainButton->OnUnhovered.RemoveAll(this);
        MainButton->OnClicked.RemoveAll(this);
        MainButton->OnPressed.RemoveAll(this);
        MainButton->OnReleased.RemoveAll(this);
    }

    Super::NativeDestruct();
}

//------------------------------------------------------------------------------
// Event handlers
//------------------------------------------------------------------------------

void UGenericButton::OnButtonHovered()
{
    bIsHovered = true;

    // Scale animation
    if (bEnableScaleOnHover && MainButton && GetWorld())
    {
        StartScale = MainButton->GetRenderTransform().Scale;
        TargetScale = OriginalScale * HoverScale;
        ScaleAnimProgress = 0.0f;

        float StepInterval = ScaleAnimationDuration / 30.0f;                // [s] - 30 steps for smooth animation
        GetWorld()->GetTimerManager().SetTimer(ScaleAnimTimer, this, &UGenericButton::ScaleAnimationStep, StepInterval, true);
    }

    // Image rotation
    if (bEnableImageRotation && ButtonImage && GetWorld())
    {
        StartRotation = ButtonImage->GetRenderTransform().Angle;
        TargetRotation = HoverRotationAngle;
        RotationAnimProgress = 0.0f;

        float StepInterval = RotationAnimationDuration / 30.0f;            // [s] - 30 steps
        GetWorld()->GetTimerManager().SetTimer(RotationAnimTimer, this, &UGenericButton::RotationAnimationStep, StepInterval, true);
    }

    // Color transition
    if (bEnableColorTransition)
    {
        StartColorTransition();
    }

    // Tooltip
    if (bEnableTooltip)
    {
        SpawnTooltip();
    }
}

void UGenericButton::OnButtonUnhovered()
{
    bIsHovered = false;
    bIsPressed = false;

    // Scale animation
    if (bEnableScaleOnHover && MainButton && GetWorld())
    {
        StartScale = MainButton->GetRenderTransform().Scale;
        TargetScale = OriginalScale;
        ScaleAnimProgress = 0.0f;

        float StepInterval = ScaleAnimationDuration / 30.0f;                // [s] - 30 steps
        GetWorld()->GetTimerManager().SetTimer(ScaleAnimTimer, this, &UGenericButton::ScaleAnimationStep, StepInterval, true);
    }

    // Image rotation
    if (bEnableImageRotation && ButtonImage && GetWorld())
    {
        StartRotation = ButtonImage->GetRenderTransform().Angle;
        TargetRotation = 0.0f;
        RotationAnimProgress = 0.0f;

        float StepInterval = RotationAnimationDuration / 30.0f;            // [s] - 30 steps
        GetWorld()->GetTimerManager().SetTimer(RotationAnimTimer, this, &UGenericButton::RotationAnimationStep, StepInterval, true);
    }

    // Color transition
    if (bEnableColorTransition)
    {
        StartColorTransition();
    }

    // Tooltip
    if (bEnableTooltip)
    {
        RemoveTooltip();
    }
}

void UGenericButton::OnButtonPressed()
{
    bIsPressed = true;

    if (bEnableColorTransition)
    {
        StartColorTransition();
    }
}

void UGenericButton::OnButtonReleased()
{
    bIsPressed = false;

    if (bEnableColorTransition)
    {
        StartColorTransition();
    }
}

void UGenericButton::OnButtonClicked()
{
    OnButtonClickedDelegate.Broadcast();
    OnButtonClickedBP();
}

//------------------------------------------------------------------------------
// Styling
//------------------------------------------------------------------------------

void UGenericButton::ApplyButtonStyling()
{
    if (MainButton)
    {
        FButtonStyle ButtonStyle = MainButton->GetStyle();

        // Set draw type based on corner style
        ESlateBrushDrawType::Type DrawType = (CornerStyle == ECornerStyle::None)
            ? ESlateBrushDrawType::Box
            : ESlateBrushDrawType::RoundedBox;

        ButtonStyle.Normal.DrawAs = DrawType;
        ButtonStyle.Hovered.DrawAs = DrawType;
        ButtonStyle.Pressed.DrawAs = DrawType;

        // Apply corner radius
        float Radius = GetCornerRadius();
        FVector4 CornerRadii(Radius, Radius, Radius, Radius);

        ButtonStyle.Normal.OutlineSettings.CornerRadii = CornerRadii;
        ButtonStyle.Hovered.OutlineSettings.CornerRadii = CornerRadii;
        ButtonStyle.Pressed.OutlineSettings.CornerRadii = CornerRadii;

        ButtonStyle.Normal.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        ButtonStyle.Hovered.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        ButtonStyle.Pressed.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;

        // Outline width for all states
        ButtonStyle.Normal.OutlineSettings.Width = OutlineWidth;
        ButtonStyle.Hovered.OutlineSettings.Width = OutlineWidth;
        ButtonStyle.Pressed.OutlineSettings.Width = OutlineWidth;

        // Outline colors per state
        ButtonStyle.Normal.OutlineSettings.Color = OutlineColorNormal;
        ButtonStyle.Hovered.OutlineSettings.Color = OutlineColorHovered;
        ButtonStyle.Pressed.OutlineSettings.Color = OutlineColorPressed;

        // Background colors per state
        ButtonStyle.Normal.TintColor = FSlateColor(BackgroundColorNormal);
        ButtonStyle.Hovered.TintColor = FSlateColor(BackgroundColorHovered);
        ButtonStyle.Pressed.TintColor = FSlateColor(BackgroundColorPressed);

        MainButton->SetStyle(ButtonStyle);
    }
}

float UGenericButton::GetCornerRadius() const
{
    // Get button size to compute relative corner radius
    float MinDimension = 32.0f; // Default fallback
    if (MainButton)
    {
        FVector2D ButtonSize = MainButton->GetCachedGeometry().GetLocalSize();
        if (!ButtonSize.IsZero())
        {
            MinDimension = FMath::Min(ButtonSize.X, ButtonSize.Y);
        }
    }

    // Return percentage of smallest dimension
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
            return MinDimension * 0.2f;
    }
}

void UGenericButton::ApplyContent()
{
    if (ButtonText && !LabelText.IsEmpty())
    {
        ButtonText->SetText(LabelText);
    }

    if (ButtonImage && IconTexture)
    {
        ButtonImage->SetBrushFromTexture(IconTexture);
    }
}

//------------------------------------------------------------------------------
// Tooltip
//------------------------------------------------------------------------------

void UGenericButton::SpawnTooltip()
{
    if (!TooltipWidgetClass || SpawnedTooltip) { return; }

    SpawnedTooltip = CreateWidget<UUserWidget>(GetWorld(), TooltipWidgetClass);
    if (SpawnedTooltip)
    {
        SpawnedTooltip->SetVisibility(ESlateVisibility::Hidden); // Keep hidden initially
        SpawnedTooltip->AddToViewport(999);
        
        // Delay showing tooltip by 2 frames (0.033s at 60fps) to allow geometry calculation
        GetWorld()->GetTimerManager().SetTimer(TooltipDelayTimer, this, &UGenericButton::ShowTooltipDelayed, 0.033f, false);
    }
}

void UGenericButton::RemoveTooltip()
{
    if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(TooltipDelayTimer); } // Clear pending timer
    
    if (SpawnedTooltip)
    {
        SpawnedTooltip->RemoveFromParent();
        SpawnedTooltip = nullptr;
    }
}

void UGenericButton::ShowTooltipDelayed()
{
    if (SpawnedTooltip && bIsHovered)
    {
        SpawnedTooltip->SetPositionInViewport(ComputeTooltipPosition(), false);
        SpawnedTooltip->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

        // Start position tracking timer
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().SetTimer(TooltipDelayTimer, this, &UGenericButton::UpdateTooltipPosition, 0.016f, true);
        }
    }
}

void UGenericButton::UpdateTooltipPosition()
{
    if (SpawnedTooltip && bIsHovered && SpawnedTooltip->GetVisibility() != ESlateVisibility::Hidden)
    {
        SpawnedTooltip->SetPositionInViewport(ComputeTooltipPosition(), false);
    }
}

FVector2D UGenericButton::ComputeTooltipPosition() const
{
    //------------------------------------------------------------------------------
    // Cursor-relative positioning
    //------------------------------------------------------------------------------
    
    if (TooltipPosition == ETooltipPosition::AtCursor)
    {
        FVector2D CursorPos = FVector2D::ZeroVector; // [px] - Screen space cursor position
        if (GetWorld() && GetWorld()->GetFirstPlayerController()) { GetWorld()->GetFirstPlayerController()->GetMousePosition(CursorPos.X, CursorPos.Y); }
        return CursorPos + TooltipOffset;
    } // End if (AtCursor check)

    //------------------------------------------------------------------------------
    // Button-relative positioning with scale compensation
    //------------------------------------------------------------------------------
    
    FGeometry ButtonGeometry = MainButton ? MainButton->GetCachedGeometry() : GetCachedGeometry();
    FVector2D LocalSize = ButtonGeometry.GetLocalSize(); // [px] - Unscaled button dimensions
    
    // Extract current render scale factor [dimensionless]
    FVector2D CurrentScale = MainButton ? MainButton->GetRenderTransform().Scale : FVector2D(1.0f, 1.0f);
    if (CurrentScale.IsZero()) { CurrentScale = FVector2D(1.0f, 1.0f); }
    
    // Compute button center in viewport space [px]
    FVector2D PixelPos, ViewportCenter;
    USlateBlueprintLibrary::LocalToViewport(GetWorld(), ButtonGeometry, LocalSize * 0.5f, PixelPos, ViewportCenter);
    
    FVector2D ScaledHalfSize = (LocalSize * CurrentScale) * 0.5f; // [px] - Half-dimensions after scale
    
    // Compute scaled edge positions [px]
    FVector2D EdgeLeft   = FVector2D(ViewportCenter.X - ScaledHalfSize.X, ViewportCenter.Y);
    FVector2D EdgeRight  = FVector2D(ViewportCenter.X + ScaledHalfSize.X, ViewportCenter.Y);
    FVector2D EdgeTop    = FVector2D(ViewportCenter.X, ViewportCenter.Y - ScaledHalfSize.Y);
    FVector2D EdgeBottom = FVector2D(ViewportCenter.X, ViewportCenter.Y + ScaledHalfSize.Y);
    
    //------------------------------------------------------------------------------
    // Tooltip centering offset
    //------------------------------------------------------------------------------
    
    FVector2D TooltipSize = FVector2D::ZeroVector; // [px] - Tooltip dimensions
    if (SpawnedTooltip)
    {
        TooltipSize = SpawnedTooltip->GetCachedGeometry().GetLocalSize();
    }
    
    FVector2D TooltipHalfSize = TooltipSize * 0.5f; // [px] - Half-dimensions for centering
    
    //------------------------------------------------------------------------------
    // Position selection by spawn anchor
    //------------------------------------------------------------------------------
    
    FVector2D Position = FVector2D::ZeroVector; // [px] - Final tooltip position
    
    switch (TooltipPosition)
    {
        case ETooltipPosition::Left:   Position = EdgeLeft   + FVector2D(-TooltipOffset.X, -TooltipHalfSize.Y); break; // Anchor left, center vertically
        case ETooltipPosition::Right:  Position = EdgeRight  + FVector2D(TooltipOffset.X, -TooltipHalfSize.Y); break; // Anchor right, center vertically
        case ETooltipPosition::Above:  Position = EdgeTop    + FVector2D(-TooltipHalfSize.X, -TooltipOffset.Y); break; // Anchor top, center horizontally
        case ETooltipPosition::Below:  Position = EdgeBottom + FVector2D(-TooltipHalfSize.X, TooltipOffset.Y); break; // Anchor bottom, center horizontally
        default: break;
    }
    
    return Position;
}

//------------------------------------------------------------------------------
// Animation step callbacks
//------------------------------------------------------------------------------

void UGenericButton::ScaleAnimationStep()
{
    if (!MainButton) { return; }

    ScaleAnimProgress += 1.0f / 30.0f;                                     // Increment by 1/30th per step

    if (ScaleAnimProgress >= 1.0f)
    {
        ScaleAnimProgress = 1.0f;
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(ScaleAnimTimer);
        }
    }

    float Alpha = ApplyScaleEasing(ScaleAnimProgress);
    FVector2D CurrentScale = FMath::Lerp(StartScale, TargetScale, Alpha);

    FWidgetTransform Transform = MainButton->GetRenderTransform();
    Transform.Scale = CurrentScale;
    MainButton->SetRenderTransform(Transform);
}

void UGenericButton::RotationAnimationStep()
{
    if (!ButtonImage) { return; }

    RotationAnimProgress += 1.0f / 30.0f;                                  // Increment by 1/30th per step

    if (RotationAnimProgress >= 1.0f)
    {
        RotationAnimProgress = 1.0f;
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(RotationAnimTimer);
        }
    }

    float Alpha = ApplyRotationEasing(RotationAnimProgress);
    float CurrentRotation = FMath::Lerp(StartRotation, TargetRotation, Alpha);

    FWidgetTransform Transform = ButtonImage->GetRenderTransform();
    Transform.Angle = CurrentRotation;
    ButtonImage->SetRenderTransform(Transform);
}

void UGenericButton::ColorAnimationStep()
{
    if (!MainButton) { return; }

    ColorAnimProgress += 1.0f / 30.0f;                                     // Increment by 1/30th per step

    if (ColorAnimProgress >= 1.0f)
    {
        ColorAnimProgress = 1.0f;
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(ColorAnimTimer);
        }
    }

    // Ease out curve
    float Alpha = 1.0f - FMath::Pow(1.0f - ColorAnimProgress, 3.0f);

    // Interpolate colors
    FLinearColor CurrentOutlineColor = FMath::Lerp(StartOutlineColor, TargetOutlineColor, Alpha);
    FLinearColor CurrentBackgroundColor = FMath::Lerp(StartBackgroundColor, TargetBackgroundColor, Alpha);

    // Apply to button style
    FButtonStyle ButtonStyle = MainButton->GetStyle();
    ButtonStyle.Normal.OutlineSettings.Color = CurrentOutlineColor;
    ButtonStyle.Hovered.OutlineSettings.Color = CurrentOutlineColor;
    ButtonStyle.Pressed.OutlineSettings.Color = CurrentOutlineColor;
    ButtonStyle.Normal.TintColor = FSlateColor(CurrentBackgroundColor);
    ButtonStyle.Hovered.TintColor = FSlateColor(CurrentBackgroundColor);
    ButtonStyle.Pressed.TintColor = FSlateColor(CurrentBackgroundColor);
    MainButton->SetStyle(ButtonStyle);
}

//------------------------------------------------------------------------------
// Easing functions
//------------------------------------------------------------------------------

float UGenericButton::ApplyScaleEasing(float Alpha) const
{
    switch (ScaleEasingType)
    {
        case EScaleEasing::Linear:
            return Alpha;

        case EScaleEasing::EaseIn:
            return Alpha * Alpha;

        case EScaleEasing::EaseOut:
            return 1.0f - (1.0f - Alpha) * (1.0f - Alpha);

        case EScaleEasing::EaseInOut:
            return Alpha < 0.5f ? 2.0f * Alpha * Alpha : 1.0f - FMath::Pow(-2.0f * Alpha + 2.0f, 2.0f) / 2.0f;

        case EScaleEasing::Cubic:
            return Alpha < 0.5f ? 4.0f * Alpha * Alpha * Alpha : 1.0f - FMath::Pow(-2.0f * Alpha + 2.0f, 3.0f) / 2.0f;

        default:
            return Alpha;
    }
}

float UGenericButton::ApplyRotationEasing(float Alpha) const
{
    switch (RotationEasingType)
    {
        case ERotationEasing::Linear:
            return Alpha;

        case ERotationEasing::EaseIn:
            return Alpha * Alpha;

        case ERotationEasing::EaseOut:
            return 1.0f - (1.0f - Alpha) * (1.0f - Alpha);

        case ERotationEasing::EaseInOut:
            return Alpha < 0.5f ? 2.0f * Alpha * Alpha : 1.0f - FMath::Pow(-2.0f * Alpha + 2.0f, 2.0f) / 2.0f;

        case ERotationEasing::Cubic:
            return Alpha < 0.5f ? 4.0f * Alpha * Alpha * Alpha : 1.0f - FMath::Pow(-2.0f * Alpha + 2.0f, 3.0f) / 2.0f;

        default:
            return Alpha;
    }
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void UGenericButton::SetLabelText(const FText& NewText)
{
    LabelText = NewText;
    if (ButtonText)
    {
        ButtonText->SetText(LabelText);
    }
}

void UGenericButton::SetIcon(UTexture2D* NewTexture)
{
    IconTexture = NewTexture;
    if (ButtonImage && IconTexture)
    {
        ButtonImage->SetBrushFromTexture(IconTexture);
    }
}

//------------------------------------------------------------------------------
// Color transition
//------------------------------------------------------------------------------

void UGenericButton::StartColorTransition()
{
    if (!GetWorld()) { return; }

    // Store current colors as start
    StartOutlineColor = TargetOutlineColor;
    StartBackgroundColor = TargetBackgroundColor;

    // Get new target colors
    GetTargetColors(TargetOutlineColor, TargetBackgroundColor);

    // Start transition timer
    ColorAnimProgress = 0.0f;
    float StepInterval = ColorTransitionDuration / 30.0f;                  // [s] - 30 steps
    GetWorld()->GetTimerManager().SetTimer(ColorAnimTimer, this, &UGenericButton::ColorAnimationStep, StepInterval, true);
}

void UGenericButton::GetTargetColors(FLinearColor& OutOutlineColor, FLinearColor& OutBackgroundColor) const
{
    // Priority: Pressed > Hovered > Normal
    if (bIsPressed)
    {
        OutOutlineColor = OutlineColorPressed;
        OutBackgroundColor = BackgroundColorPressed;
    }
    else if (bIsHovered)
    {
        OutOutlineColor = OutlineColorHovered;
        OutBackgroundColor = BackgroundColorHovered;
    }
    else
    {
        OutOutlineColor = OutlineColorNormal;
        OutBackgroundColor = BackgroundColorNormal;
    }
}

void UGenericButton::ApplyButtonColors()
{
    if (!MainButton) { return; }

    FLinearColor OutlineColor, BackgroundColor;
    GetTargetColors(OutlineColor, BackgroundColor);

    FButtonStyle ButtonStyle = MainButton->GetStyle();
    ButtonStyle.Normal.OutlineSettings.Color = OutlineColor;
    ButtonStyle.Hovered.OutlineSettings.Color = OutlineColor;
    ButtonStyle.Pressed.OutlineSettings.Color = OutlineColor;
    ButtonStyle.Normal.TintColor = FSlateColor(BackgroundColor);
    ButtonStyle.Hovered.TintColor = FSlateColor(BackgroundColor);
    ButtonStyle.Pressed.TintColor = FSlateColor(BackgroundColor);
    MainButton->SetStyle(ButtonStyle);
}
