#include "PulseButtonWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Styling/SlateStyle.h"
#include "Framework/Text/SlateTextLayout.h"

UPulseButtonWidget::UPulseButtonWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , bIsHovered(false)
    , bIsPressed(false)
    , CurrentAnimationTime(0.0f)
{
    // Initialize with default theme
    ThemeData = GetDefaultUITheme();
}

void UPulseButtonWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    // Apply theme and styling in design time too
    ApplyThemeToComponents();
    ApplyButtonStyle();
    UpdateEnabledState();
}

void UPulseButtonWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    if (MainButton)
    {
        // Store original scale
        OriginalScale = MainButton->GetRenderTransform().Scale;
        StartScale = OriginalScale;
        TargetScale = OriginalScale;
        
        // Bind button events
        MainButton->OnHovered.AddDynamic(this, &UPulseButtonWidget::OnButtonHovered);
        MainButton->OnUnhovered.AddDynamic(this, &UPulseButtonWidget::OnButtonUnhovered);
        MainButton->OnClicked.AddDynamic(this, &UPulseButtonWidget::OnButtonClicked);
    }

    // Setup text component
    if (ButtonText)
    {
        ButtonText->SetText(ButtonTextContent);
    }

    // Apply initial styling
    ApplyThemeToComponents();
    ApplyButtonStyle();
    UpdateEnabledState();
}

void UPulseButtonWidget::NativeDestruct()
{
    // Clean up delegates
    if (MainButton)
    {
        MainButton->OnHovered.RemoveAll(this);
        MainButton->OnUnhovered.RemoveAll(this);
        MainButton->OnClicked.RemoveAll(this);
    }
    Super::NativeDestruct();
}

void UPulseButtonWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    
    // Handle scale animation
    if (CurrentAnimationTime < ThemeData.AnimationSpeed && MainButton)
    {
        CurrentAnimationTime = FMath::Min(CurrentAnimationTime + InDeltaTime, ThemeData.AnimationSpeed);
        float Alpha = CurrentAnimationTime / ThemeData.AnimationSpeed;
        
        // Smooth cubic ease-out curve
        Alpha = 1.0f - FMath::Pow(1.0f - Alpha, 3.0f);
        
        FVector2D CurrentScale = FMath::Lerp(StartScale, TargetScale, Alpha);
        
        FWidgetTransform Transform = MainButton->GetRenderTransform();
        Transform.Scale = CurrentScale;
        MainButton->SetRenderTransform(Transform);
    }
}

void UPulseButtonWidget::ApplyThemeToComponents()
{
    if (ButtonText)
    {
        // Apply default font size
        FSlateFontInfo FontInfo = ButtonText->GetFont();
        FontInfo.Size = 13.0f;
        ButtonText->SetFont(FontInfo);
        ButtonText->SetColorAndOpacity(ThemeData.TextPrimary);
    }
}

void UPulseButtonWidget::ApplyButtonStyle()
{
    if (!MainButton) return;
    
    FButtonStyle ButtonStyle = MainButton->GetStyle();
    
    // Set all states to rounded box
    ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::RoundedBox;
    ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::RoundedBox;
    ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::RoundedBox;
    ButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::RoundedBox;
    
    // Use HalfHeightRadius for all states - no math needed
    ButtonStyle.Normal.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
    ButtonStyle.Hovered.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
    ButtonStyle.Pressed.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
    ButtonStyle.Disabled.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;

    // Simple button style - card background
    ButtonStyle.Normal.TintColor = ThemeData.BackgroundCard;
    ButtonStyle.Hovered.TintColor = ThemeData.BackgroundHover;
    ButtonStyle.Pressed.TintColor = ThemeData.BackgroundCard * 0.8f;
    ButtonStyle.Disabled.TintColor = ThemeData.BackgroundCard * 0.5f;
    
    // No outline
    ButtonStyle.Normal.OutlineSettings.Width = 0.0f;
    ButtonStyle.Hovered.OutlineSettings.Width = 0.0f;
    ButtonStyle.Pressed.OutlineSettings.Width = 0.0f;
    ButtonStyle.Disabled.OutlineSettings.Width = 0.0f;
    
    // Enable brush transparency for all states
    ButtonStyle.Normal.OutlineSettings.bUseBrushTransparency = true;
    ButtonStyle.Hovered.OutlineSettings.bUseBrushTransparency = true;
    ButtonStyle.Pressed.OutlineSettings.bUseBrushTransparency = true;
    ButtonStyle.Disabled.OutlineSettings.bUseBrushTransparency = true;
    
    MainButton->SetStyle(ButtonStyle);
}

void UPulseButtonWidget::UpdateHoverState(bool bHovered)
{
    bIsHovered = bHovered;
    
    if (MainButton)
    {
        StartScale = MainButton->GetRenderTransform().Scale;
        TargetScale = bHovered ? (OriginalScale * ThemeData.HoverScale) : OriginalScale;
        CurrentAnimationTime = 0.0f;
    }
}

void UPulseButtonWidget::UpdateEnabledState()
{
    if (MainButton)
    {
        MainButton->SetIsEnabled(GetIsEnabled());
    }
    
    if (ButtonText)
    {
        FLinearColor TextColor = GetIsEnabled() ? ThemeData.TextPrimary : ThemeData.TextMuted;
        ButtonText->SetColorAndOpacity(TextColor);
    }
}

void UPulseButtonWidget::OnButtonHovered()
{
    UpdateHoverState(true);
    
    // Broadcast events
    OnPulseButtonHovered();
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1, 0.5f, FColor::Cyan,
            FString::Printf(TEXT("PulseButton HOVERED: %s"), *ButtonTextContent.ToString())
        );
    }
}

void UPulseButtonWidget::OnButtonUnhovered()
{
    UpdateHoverState(false);
    
    // Broadcast events
    OnPulseButtonUnhovered();
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1, 0.5f, FColor::Blue,
            FString::Printf(TEXT("PulseButton UNHOVERED: %s"), *ButtonTextContent.ToString())
        );
    }
}

void UPulseButtonWidget::OnButtonClicked()
{
    if (!GetIsEnabled()) return;
    
    // Click pulse effect
    if (MainButton)
    {
        bIsPressed = true;
        
        // Quick pulse animation - scale down then back up
        StartScale = MainButton->GetRenderTransform().Scale;
        TargetScale = OriginalScale * 0.9f; // Scale down on click
        CurrentAnimationTime = 0.0f;
        
        // Reset to hover scale (if hovered) or original scale after brief pulse
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
        {
            if (MainButton)
            {
                StartScale = MainButton->GetRenderTransform().Scale;
                TargetScale = bIsHovered ? (OriginalScale * ThemeData.HoverScale) : OriginalScale;
                CurrentAnimationTime = 0.0f;
                bIsPressed = false;
            }
        }, 0.1f, false);
    }
    
    // Broadcast events
    OnPulseButtonClickedDelegate.Broadcast();
    OnPulseButtonClicked();
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1, 1.0f, FColor::Yellow,
            FString::Printf(TEXT("PulseButton CLICKED: %s"), *ButtonTextContent.ToString())
        );
    }
}

// Public interface implementations
void UPulseButtonWidget::SetButtonText(const FText& NewText)
{
    ButtonTextContent = NewText;
    if (ButtonText)
    {
        ButtonText->SetText(ButtonTextContent);
    }
}

void UPulseButtonWidget::SetEnabled(bool bNewEnabled)
{
    if (GetIsEnabled() != bNewEnabled)
    {
        SetIsEnabled(bNewEnabled);
        UpdateEnabledState();
    }
}

void UPulseButtonWidget::SetTheme(const FUIThemeData& NewTheme)
{
    ThemeData = NewTheme;
    ApplyThemeToComponents();
    ApplyButtonStyle();
    UpdateEnabledState();
}

FUIThemeData UPulseButtonWidget::GetDefaultUITheme()
{
    FUIThemeData DefaultTheme;
    
    // Colors matching modern UI design
    DefaultTheme.AccentPrimary = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f); // White
    DefaultTheme.AccentSecondary = FLinearColor(0.8f, 0.8f, 0.8f, 1.0f); // Light grey
    
    DefaultTheme.BackgroundDark = FLinearColor(0.067f, 0.067f, 0.067f, 1.0f); // #111111
    DefaultTheme.BackgroundCard = FLinearColor(0.102f, 0.102f, 0.102f, 1.0f); // #1a1a1a
    DefaultTheme.BackgroundHover = FLinearColor(0.145f, 0.145f, 0.145f, 1.0f); // #252525
    
    DefaultTheme.TextPrimary = FLinearColor(0.867f, 0.867f, 0.867f, 1.0f); // #dddddd
    DefaultTheme.TextMuted = FLinearColor(0.533f, 0.533f, 0.533f, 1.0f); // #888888
    DefaultTheme.TextOnAccent = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f); // Dark text on light backgrounds
    
    DefaultTheme.BorderNormal = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f); // #333333
    DefaultTheme.BorderHover = FLinearColor(0.333f, 0.333f, 0.333f, 1.0f); // #555555
    
    DefaultTheme.BorderRadius = 12.0f;
    DefaultTheme.AnimationSpeed = 0.25f;
    DefaultTheme.HoverScale = 1.05f; // Subtle scale on hover
    
    return DefaultTheme;
}