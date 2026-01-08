#include "HoverButtonV2.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"

UHoverButtonV2::UHoverButtonV2(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , bIsHovered(false)
    , CurrentAnimationTime(0.0f)
{
}

void UHoverButtonV2::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Store original scale and setup button styling
    if (MainButton)
    {
        OriginalScale = MainButton->GetRenderTransform().Scale;
        StartScale = OriginalScale;
        TargetScale = OriginalScale;
        
        // Setup initial transparent style
        UpdateButtonStyle(false);
        
        // Bind button events
        MainButton->OnHovered.AddDynamic(this, &UHoverButtonV2::OnButtonHovered);
        MainButton->OnUnhovered.AddDynamic(this, &UHoverButtonV2::OnButtonUnhovered);
        MainButton->OnClicked.AddDynamic(this, &UHoverButtonV2::OnButtonClicked);
    }

    // Setup text component
    if (ButtonText)
    {
        ButtonText->SetText(ButtonTextContent);
        UpdateTextStyle(false);
    }
}

void UHoverButtonV2::NativeDestruct()
{
    // Clean up delegates to prevent memory leaks
    if (MainButton)
    {
        MainButton->OnHovered.RemoveAll(this);
        MainButton->OnUnhovered.RemoveAll(this);
        MainButton->OnClicked.RemoveAll(this);
    }
    Super::NativeDestruct();
}

void UHoverButtonV2::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    
    // Only tick when animating
    if (CurrentAnimationTime < AnimationDuration && MainButton)
    {
        CurrentAnimationTime = FMath::Min(CurrentAnimationTime + InDeltaTime, AnimationDuration);
        float Alpha = CurrentAnimationTime / AnimationDuration;
        
        // Smooth ease out animation curve
        Alpha = 1.0f - FMath::Pow(1.0f - Alpha, 3.0f); // Cubic ease out
        
        FVector2D CurrentScale = FMath::Lerp(StartScale, TargetScale, Alpha);
        
        FWidgetTransform Transform = MainButton->GetRenderTransform();
        Transform.Scale = CurrentScale;
        MainButton->SetRenderTransform(Transform);
    }
}

void UHoverButtonV2::UpdateButtonStyle(bool bHovered)
{
    if (!MainButton) return;
    
    FButtonStyle ButtonStyle = MainButton->GetStyle();
    
    // Set rounded corners for all button states
    ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::RoundedBox;
    ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::RoundedBox;
    ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::RoundedBox;
    
    // Set corner radius
    ButtonStyle.Normal.OutlineSettings.CornerRadii = FVector4(CornerRadius, CornerRadius, CornerRadius, CornerRadius);
    ButtonStyle.Hovered.OutlineSettings.CornerRadii = FVector4(CornerRadius, CornerRadius, CornerRadius, CornerRadius);
    ButtonStyle.Pressed.OutlineSettings.CornerRadii = FVector4(CornerRadius, CornerRadius, CornerRadius, CornerRadius);
    
    if (bHovered)
    {
        // Hovered state: show grey background, white outline, and border
        ButtonStyle.Normal.TintColor = HoveredBackgroundColor;
        ButtonStyle.Normal.OutlineSettings.Width = OutlineWidth;
        ButtonStyle.Normal.OutlineSettings.Color = HoveredOutlineColor;
        
        // Set border for hovered state
        ButtonStyle.Normal.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        ButtonStyle.Normal.OutlineSettings.bUseBrushTransparency = true;
        
        ButtonStyle.Hovered.TintColor = HoveredBackgroundColor;
        ButtonStyle.Hovered.OutlineSettings.Width = OutlineWidth;
        ButtonStyle.Hovered.OutlineSettings.Color = HoveredOutlineColor;
        ButtonStyle.Hovered.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        ButtonStyle.Hovered.OutlineSettings.bUseBrushTransparency = true;
        
        ButtonStyle.Pressed.TintColor = HoveredBackgroundColor * 0.8f; // Slightly darker when pressed
        ButtonStyle.Pressed.OutlineSettings.Width = OutlineWidth;
        ButtonStyle.Pressed.OutlineSettings.Color = HoveredOutlineColor;
        ButtonStyle.Pressed.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        ButtonStyle.Pressed.OutlineSettings.bUseBrushTransparency = true;
        
        // Apply border settings for hovered state
        if (BorderThickness > 0.0f)
        {
            ButtonStyle.Normal.OutlineSettings.Width = FMath::Max(OutlineWidth, BorderThickness);
            ButtonStyle.Normal.OutlineSettings.Color = HoveredBorderColor;
            
            ButtonStyle.Hovered.OutlineSettings.Width = FMath::Max(OutlineWidth, BorderThickness);
            ButtonStyle.Hovered.OutlineSettings.Color = HoveredBorderColor;
            
            ButtonStyle.Pressed.OutlineSettings.Width = FMath::Max(OutlineWidth, BorderThickness);
            ButtonStyle.Pressed.OutlineSettings.Color = HoveredBorderColor;
        }
    }
    else
    {
        // Normal state: transparent background and border
        ButtonStyle.Normal.TintColor = NormalBackgroundColor;
        ButtonStyle.Normal.OutlineSettings.Color = NormalOutlineColor;
        ButtonStyle.Normal.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        ButtonStyle.Normal.OutlineSettings.bUseBrushTransparency = true;
        
        ButtonStyle.Hovered.TintColor = NormalBackgroundColor;
        ButtonStyle.Hovered.OutlineSettings.Color = NormalOutlineColor;
        ButtonStyle.Hovered.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        ButtonStyle.Hovered.OutlineSettings.bUseBrushTransparency = true;
        
        ButtonStyle.Pressed.TintColor = NormalBackgroundColor;
        ButtonStyle.Pressed.OutlineSettings.Color = NormalOutlineColor;
        ButtonStyle.Pressed.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        ButtonStyle.Pressed.OutlineSettings.bUseBrushTransparency = true;
        
        // Apply border settings for normal state
        if (BorderThickness > 0.0f && NormalBorderColor.A > 0.0f)
        {
            ButtonStyle.Normal.OutlineSettings.Width = BorderThickness;
            ButtonStyle.Normal.OutlineSettings.Color = NormalBorderColor;
            
            ButtonStyle.Hovered.OutlineSettings.Width = BorderThickness;
            ButtonStyle.Hovered.OutlineSettings.Color = NormalBorderColor;
            
            ButtonStyle.Pressed.OutlineSettings.Width = BorderThickness;
            ButtonStyle.Pressed.OutlineSettings.Color = NormalBorderColor;
        }
        else
        {
            // No border in normal state
            ButtonStyle.Normal.OutlineSettings.Width = 0.0f;
            ButtonStyle.Hovered.OutlineSettings.Width = 0.0f;
            ButtonStyle.Pressed.OutlineSettings.Width = 0.0f;
        }
    }
    
    MainButton->SetStyle(ButtonStyle);
}

void UHoverButtonV2::UpdateTextStyle(bool bHovered)
{
    if (!ButtonText) return;
    
    FLinearColor TextColor = bHovered ? HoveredTextColor : NormalTextColor;
    ButtonText->SetColorAndOpacity(TextColor);
}

void UHoverButtonV2::OnButtonHovered()
{
    if (MainButton)
    {
        StartScale = MainButton->GetRenderTransform().Scale;
        TargetScale = OriginalScale * HoverScale;
        CurrentAnimationTime = 0.0f;
        bIsHovered = true;
        
        // Show outline and grey background
        UpdateButtonStyle(true);
        UpdateTextStyle(true);
    }
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            0.5f,
            FColor::Green,
            TEXT("NavButton HOVERED")
        );
    }
}

void UHoverButtonV2::OnButtonUnhovered()
{
    if (MainButton)
    {
        StartScale = MainButton->GetRenderTransform().Scale;
        TargetScale = OriginalScale;
        CurrentAnimationTime = 0.0f;
        bIsHovered = false;
        
        // Hide outline and background (make transparent)
        UpdateButtonStyle(false);
        UpdateTextStyle(false);
    }
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            0.5f,
            FColor::Red,
            TEXT("NavButton UNHOVERED")
        );
    }
}

void UHoverButtonV2::OnButtonClicked()
{
    // Broadcast C++ delegate
    OnButtonClickedDelegate.Broadcast();
    
    // Call Blueprint event
    OnButtonClickedBP();
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            1.0f,
            FColor::Yellow,
            TEXT("NavButton CLICKED")
        );
    }
}

void UHoverButtonV2::SetButtonText(const FText& NewText)
{
    ButtonTextContent = NewText;
    if (ButtonText)
    {
        ButtonText->SetText(ButtonTextContent);
    }
}