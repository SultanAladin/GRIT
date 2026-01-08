// GenericEntry.cpp
#include "GenericEntry.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

/*============================================================================
                              INITIALIZATION
============================================================================*/

UGenericEntry::UGenericEntry(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UGenericEntry::NativePreConstruct()
{
    Super::NativePreConstruct();

    // Initialize state
    bIsExpanded = bStartExpanded;
    CurrentHeight = bIsExpanded ? ExpandedHeight : CollapsedHeight;

    // Initialize colors
    GetRootTargetColors(CurrentRootColor, CurrentRootOutline);
    GetMetadataTargetColors(CurrentMetadataColor, CurrentMetadataOutline);

    // Apply initial styles
    ApplyRootBorderStyle();
    ApplyMetadataBorderStyle();
    ApplyContent();
    UpdateHeightOverride();

    // SizeBox clipping for animation
    if (EntryMetadata) { EntryMetadata->SetClipping(EWidgetClipping::ClipToBounds); }
}

void UGenericEntry::NativeConstruct()
{
    Super::NativeConstruct();

    // Initialize state
    bIsExpanded = bStartExpanded;
    CurrentHeight = bIsExpanded ? ExpandedHeight : CollapsedHeight;
    TargetHeight = CurrentHeight;
    StartHeight = CurrentHeight;

    // Initialize root colors
    GetRootTargetColors(CurrentRootColor, CurrentRootOutline);
    TargetRootColor = CurrentRootColor;
    StartRootColor = CurrentRootColor;
    TargetRootOutline = CurrentRootOutline;
    StartRootOutline = CurrentRootOutline;

    // Initialize metadata colors
    GetMetadataTargetColors(CurrentMetadataColor, CurrentMetadataOutline);
    TargetMetadataColor = CurrentMetadataColor;
    StartMetadataColor = CurrentMetadataColor;
    TargetMetadataOutline = CurrentMetadataOutline;
    StartMetadataOutline = CurrentMetadataOutline;

    // Apply styles
    ApplyRootBorderStyle();
    ApplyMetadataBorderStyle();
    ApplyContent();
    UpdateHeightOverride();

    // SizeBox clipping
    if (EntryMetadata) { EntryMetadata->SetClipping(EWidgetClipping::ClipToBounds); }

    // Bind dropdown button
    if (DropDownButton)
    {
        DropDownButton->OnClicked.AddDynamic(this, &UGenericEntry::OnDropDownClicked);
    }

    // Bind confirm button
    if (ConfirmButton)
    {
        ConfirmButton->OnClicked.AddDynamic(this, &UGenericEntry::OnConfirmClicked);
        ConfirmButton->OnPressed.AddDynamic(this, &UGenericEntry::OnConfirmPressed);
        ConfirmButton->OnReleased.AddDynamic(this, &UGenericEntry::OnConfirmReleased);
        ConfirmButton->OnHovered.AddDynamic(this, &UGenericEntry::OnConfirmHovered);
        ConfirmButton->OnUnhovered.AddDynamic(this, &UGenericEntry::OnConfirmUnhovered);
    }
}

void UGenericEntry::NativeDestruct()
{
    if (DropDownButton) { DropDownButton->OnClicked.RemoveAll(this); }

    if (ConfirmButton)
    {
        ConfirmButton->OnClicked.RemoveAll(this);
        ConfirmButton->OnPressed.RemoveAll(this);
        ConfirmButton->OnReleased.RemoveAll(this);
        ConfirmButton->OnHovered.RemoveAll(this);
        ConfirmButton->OnUnhovered.RemoveAll(this);
    }

    Super::NativeDestruct();
}

/*============================================================================
                              TICK - ANIMATION
============================================================================*/

void UGenericEntry::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    //------------------------------------------------------------------------------
    // Height animation
    //------------------------------------------------------------------------------

    if (bIsHeightAnimating && HeightAnimTime < AnimDuration)
    {
        HeightAnimTime = FMath::Min(HeightAnimTime + InDeltaTime, AnimDuration);
        float Alpha = ApplyEasing(HeightAnimTime / AnimDuration);

        CurrentHeight = FMath::Lerp(StartHeight, TargetHeight, Alpha);
        UpdateHeightOverride();

        if (HeightAnimTime >= AnimDuration) { bIsHeightAnimating = false; }
    }

    //------------------------------------------------------------------------------
    // Root border color transition
    //------------------------------------------------------------------------------

    if (RootColorAnimTime < ColorTransitionDuration)
    {
        RootColorAnimTime = FMath::Min(RootColorAnimTime + InDeltaTime, ColorTransitionDuration);
        float Alpha = ApplyEasing(RootColorAnimTime / ColorTransitionDuration);

        CurrentRootColor = FMath::Lerp(StartRootColor, TargetRootColor, Alpha);
        CurrentRootOutline = FMath::Lerp(StartRootOutline, TargetRootOutline, Alpha);
        ApplyRootBorderStyle();
    }

    //------------------------------------------------------------------------------
    // Metadata border color transition
    //------------------------------------------------------------------------------

    if (MetadataColorAnimTime < ColorTransitionDuration)
    {
        MetadataColorAnimTime = FMath::Min(MetadataColorAnimTime + InDeltaTime, ColorTransitionDuration);
        float Alpha = ApplyEasing(MetadataColorAnimTime / ColorTransitionDuration);

        CurrentMetadataColor = FMath::Lerp(StartMetadataColor, TargetMetadataColor, Alpha);
        CurrentMetadataOutline = FMath::Lerp(StartMetadataOutline, TargetMetadataOutline, Alpha);
        ApplyMetadataBorderStyle();
    }
}

/*============================================================================
                              EVENT HANDLERS
============================================================================*/

void UGenericEntry::OnDropDownClicked()
{
    Toggle();
}

void UGenericEntry::OnConfirmClicked()
{
    OnConfirmed.Broadcast();
    OnConfirmedBP();
}

void UGenericEntry::OnConfirmPressed()
{
    bIsMetadataPressed = true;
    StartMetadataColorTransition();
}

void UGenericEntry::OnConfirmReleased()
{
    bIsMetadataPressed = false;
    StartMetadataColorTransition();
}

void UGenericEntry::OnConfirmHovered()
{
    bIsMetadataHovered = true;
    StartMetadataColorTransition();
}

void UGenericEntry::OnConfirmUnhovered()
{
    bIsMetadataHovered = false;
    bIsMetadataPressed = false;
    StartMetadataColorTransition();
}

/*============================================================================
                              STYLING
============================================================================*/

void UGenericEntry::ApplyRootBorderStyle()
{
    if (!RootBorder) { return; }

    FSlateBrush Brush;
    Brush.DrawAs = (CornerStyle == ECornerStyle::None) ? ESlateBrushDrawType::Box : ESlateBrushDrawType::RoundedBox;
    Brush.TintColor = FSlateColor(CurrentRootColor);

    float Radius = GetCornerRadius(RootBorder);
    Brush.OutlineSettings.Width = RootOutlineWidth;
    Brush.OutlineSettings.Color = CurrentRootOutline;
    Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);

    RootBorder->SetBrush(Brush);
}

void UGenericEntry::ApplyMetadataBorderStyle()
{
    if (!MetadataBorder) { return; }

    FSlateBrush Brush;
    Brush.DrawAs = (CornerStyle == ECornerStyle::None) ? ESlateBrushDrawType::Box : ESlateBrushDrawType::RoundedBox;
    Brush.TintColor = FSlateColor(CurrentMetadataColor);

    float Radius = GetCornerRadius(MetadataBorder);
    Brush.OutlineSettings.Width = MetadataOutlineWidth;
    Brush.OutlineSettings.Color = CurrentMetadataOutline;
    Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);

    MetadataBorder->SetBrush(Brush);
}

void UGenericEntry::ApplyContent()
{
    if (HeaderText && !LabelText.IsEmpty()) { HeaderText->SetText(LabelText); }
    if (EntryIcon && IconTexture) { EntryIcon->SetBrushFromTexture(IconTexture); }
}

void UGenericEntry::UpdateHeightOverride()
{
    if (!EntryMetadata) { return; }
    EntryMetadata->SetHeightOverride(CurrentHeight);
}

/*============================================================================
                              ANIMATION STARTERS
============================================================================*/

void UGenericEntry::StartHeightAnimation(float NewTarget)
{
    StartHeight = CurrentHeight;
    TargetHeight = NewTarget;
    HeightAnimTime = 0.0f;
    bIsHeightAnimating = true;
}

void UGenericEntry::StartRootColorTransition()
{
    StartRootColor = CurrentRootColor;
    StartRootOutline = CurrentRootOutline;
    GetRootTargetColors(TargetRootColor, TargetRootOutline);
    RootColorAnimTime = 0.0f;
}

void UGenericEntry::StartMetadataColorTransition()
{
    StartMetadataColor = CurrentMetadataColor;
    StartMetadataOutline = CurrentMetadataOutline;
    GetMetadataTargetColors(TargetMetadataColor, TargetMetadataOutline);
    MetadataColorAnimTime = 0.0f;
}

/*============================================================================
                              COLOR SELECTION
============================================================================*/

void UGenericEntry::GetRootTargetColors(FLinearColor& OutColor, FLinearColor& OutOutline) const
{
    // Priority: Selected > Hovered > Normal
    if (bIsExpanded)
    {
        OutColor = RootColorSelected;
        OutOutline = RootOutlineSelected;
    }
    else if (bIsHovered)
    {
        OutColor = RootColorHovered;
        OutOutline = RootOutlineHovered;
    }
    else
    {
        OutColor = RootColorNormal;
        OutOutline = RootOutlineNormal;
    }
}

void UGenericEntry::GetMetadataTargetColors(FLinearColor& OutColor, FLinearColor& OutOutline) const
{
    // Priority: Pressed > Hovered > Normal
    if (bIsMetadataPressed)
    {
        OutColor = MetadataColorPressed;
        OutOutline = MetadataOutlinePressed;
    }
    else if (bIsMetadataHovered)
    {
        OutColor = MetadataColorHovered;
        OutOutline = MetadataOutlineHovered;
    }
    else
    {
        OutColor = MetadataColorNormal;
        OutOutline = MetadataOutlineNormal;
    }
}

/*============================================================================
                              HELPERS
============================================================================*/

float UGenericEntry::GetCornerRadius(UWidget* ForWidget) const
{
    float MinDimension = 32.0f;
    if (ForWidget)
    {
        FVector2D Size = ForWidget->GetCachedGeometry().GetLocalSize();
        if (!Size.IsZero()) { MinDimension = FMath::Min(Size.X, Size.Y); }
    }

    switch (CornerStyle)
    {
        case ECornerStyle::None:    return 0.0f;
        case ECornerStyle::Slight:  return MinDimension * 0.1f;
        case ECornerStyle::Medium:  return MinDimension * 0.2f;
        case ECornerStyle::Rounded: return MinDimension * 0.35f;
        case ECornerStyle::Pill:    return MinDimension * 0.5f;
        default:                    return MinDimension * 0.2f;
    }
}

float UGenericEntry::ApplyEasing(float Alpha) const
{
    // Ease out cubic
    return 1.0f - FMath::Pow(1.0f - Alpha, 3.0f);
}

/*============================================================================
                              PUBLIC API
============================================================================*/

void UGenericEntry::SetExpanded(bool bExpand)
{
    if (bIsExpanded == bExpand) { return; }

    bIsExpanded = bExpand;
    StartHeightAnimation(bIsExpanded ? ExpandedHeight : CollapsedHeight);

    // Update root colors for selected state
    StartRootColorTransition();

    OnToggled.Broadcast(bIsExpanded);
    OnToggledBP(bIsExpanded);
}

void UGenericEntry::Toggle()
{
    SetExpanded(!bIsExpanded);
}

void UGenericEntry::SetHeaderText(const FText& NewText)
{
    LabelText = NewText;
    if (HeaderText) { HeaderText->SetText(LabelText); }
}

void UGenericEntry::SetIcon(UTexture2D* NewIcon)
{
    IconTexture = NewIcon;
    if (EntryIcon && IconTexture) { EntryIcon->SetBrushFromTexture(IconTexture); }
}

void UGenericEntry::ShowMetadata(bool bShow)
{
    bShowConfirmButton = bShow;
    if (MetadataBorder) { MetadataBorder->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); }
}
