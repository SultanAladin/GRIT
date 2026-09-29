#include "StateIndicator.h"
#include "Components/BorderSlot.h"

UStateIndicator::UStateIndicator(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , bIsPressed(false)
    , bIsHovered(false)
{
}

void UStateIndicator::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Apply initial configuration
    ApplyStateIndicatorConfig(IndicatorConfig);
}

FReply UStateIndicator::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        bIsPressed = true;
        UpdateVisualState();
        return FReply::Handled().CaptureMouse(this->GetCachedWidget().ToSharedRef());
    }
    
    return FReply::Unhandled();
}

FReply UStateIndicator::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsPressed)
    {
        bIsPressed = false;
        
        if (InGeometry.IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
        {
            HandleIndicatorClick();
        }
        
        UpdateVisualState();
        return FReply::Handled().ReleaseMouseCapture();
    }
    
    return FReply::Unhandled();
}

void UStateIndicator::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    bIsHovered = true;
    UpdateVisualState();
}

void UStateIndicator::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    bIsHovered = false;
    UpdateVisualState();
}

void UStateIndicator::SetImageTexture(UTexture2D* NewTexture)
{
    IndicatorConfig.ImageTexture = NewTexture;
    
    if (Image_24 && NewTexture)
    {
        Image_24->SetBrushFromTexture(NewTexture);
    }
}

void UStateIndicator::SetImageTintColor(const FLinearColor& NewColor)
{
    IndicatorConfig.ImageTintColor = NewColor;
    UpdateVisualState();
}

void UStateIndicator::SetBackgroundColor(const FLinearColor& NewColor)
{
    IndicatorConfig.BackgroundColor = NewColor;
    UpdateVisualState();
}

void UStateIndicator::SetBorderColor(const FLinearColor& NewColor)
{
    IndicatorConfig.BorderColor = NewColor;
    UpdateVisualState();
}

void UStateIndicator::ApplyStateIndicatorConfig(const FStateIndicatorConfig& NewConfig)
{
    IndicatorConfig = NewConfig;
    
    // Apply image texture if set
    if (IndicatorConfig.ImageTexture)
    {
        SetImageTexture(IndicatorConfig.ImageTexture);
    }
    
    UpdateVisualState();
}

void UStateIndicator::UpdateVisualState()
{
    if (!Border_17 || !Image_24)
    {
        return;
    }
    
    // Create border brush with rounded box style
    FSlateBrush BorderBrush;
    BorderBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
    
    // Apply background color (with hover/press effects)
    FLinearColor CurrentBackgroundColor = IndicatorConfig.BackgroundColor;
    if (bIsPressed)
    {
        CurrentBackgroundColor = CurrentBackgroundColor * 0.7f; // Darken when pressed
    }
    else if (bIsHovered)
    {
        CurrentBackgroundColor = CurrentBackgroundColor * 1.2f; // Lighten when hovered
    }
    
    BorderBrush.TintColor = FSlateColor(CurrentBackgroundColor);
    
    // Apply border styling
    BorderBrush.OutlineSettings.Color = FSlateColor(IndicatorConfig.BorderColor);
    BorderBrush.OutlineSettings.Width = IndicatorConfig.BorderWidth;
    BorderBrush.OutlineSettings.CornerRadii = FVector4(IndicatorConfig.CornerRadius, 0.0f, 0.0f, 0.0f);
    
    Border_17->SetBrush(BorderBrush);
    
    // Update image styling
    FSlateBrush ImageBrush;
    ImageBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
    ImageBrush.TintColor = FSlateColor(IndicatorConfig.ImageTintColor);
    ImageBrush.ImageSize = IndicatorConfig.ImageSize;
    
    if (IndicatorConfig.ImageTexture)
    {
        ImageBrush.SetResourceObject(IndicatorConfig.ImageTexture);
    }
    
    Image_24->SetBrush(ImageBrush);
    
    // Set image size
    Image_24->SetDesiredSizeOverride(IndicatorConfig.ImageSize);
}

void UStateIndicator::HandleIndicatorClick()
{
    OnStateIndicatorClicked.Broadcast(this);
}