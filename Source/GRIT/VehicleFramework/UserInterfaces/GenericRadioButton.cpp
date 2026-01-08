// GenericRadioButton.cpp
#include "GenericRadioButton.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

/*============================================================================
                              INITIALIZATION
============================================================================*/

UGenericRadioButton::UGenericRadioButton(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UGenericRadioButton::NativePreConstruct()
{
    // FIXED: Apply radio-specific content before parent styling
    ApplyRadioContent();
    
    Super::NativePreConstruct();

    bIsSelected = bStartSelected;
    CurrentRingColor = GetTargetRingColor();
    CurrentIndicatorScale = GetTargetIndicatorScale();

    ApplyRingStyle();
    ApplyIndicatorStyle();
    
    UE_LOG(LogTemp, Log, TEXT("[GenericRadioButton] NativePreConstruct - Label: '%s', Icon: %s"), 
           *RadioLabelText.ToString(), RadioIconTexture ? *RadioIconTexture->GetName() : TEXT("None"));
}

void UGenericRadioButton::NativeConstruct()
{
    // FIXED: Apply radio-specific content before parent construction
    ApplyRadioContent();
    
    Super::NativeConstruct();

    bIsSelected = bStartSelected;

    // Initialize animation state
    CurrentRingColor = GetTargetRingColor();
    TargetRingColor = CurrentRingColor;
    StartRingColor = CurrentRingColor;

    CurrentIndicatorScale = GetTargetIndicatorScale();
    TargetIndicatorScale = CurrentIndicatorScale;
    StartIndicatorScale = CurrentIndicatorScale;

    ApplyRingStyle();
    ApplyIndicatorStyle();

    // Bind to parent's click delegate
    OnButtonClickedDelegate.AddDynamic(this, &UGenericRadioButton::OnRadioClicked);
    
    UE_LOG(LogTemp, Log, TEXT("[GenericRadioButton] NativeConstruct - Label: '%s', Icon: %s, Selected: %s"), 
           *RadioLabelText.ToString(), RadioIconTexture ? *RadioIconTexture->GetName() : TEXT("None"),
           bIsSelected ? TEXT("True") : TEXT("False"));
}

void UGenericRadioButton::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Detect hover state from parent (GenericButton exposes bIsHovered as private, so we track via MainButton)
    bool bCurrentlyHovered = MainButton && MainButton->IsHovered();
    if (bCurrentlyHovered != bRadioHovered)
    {
        bRadioHovered = bCurrentlyHovered;

        // Only update visuals if NOT selected - selected state locks the indicator
        if (!bIsSelected)
        {
            UpdateRadioVisuals();
        }
        else
        {
            // Still update ring color when selected, just not indicator scale
            StartRingColor = CurrentRingColor;
            TargetRingColor = GetTargetRingColor();
            RingAnimTime = 0.0f;
        }
    }

    //------------------------------------------------------------------------------
    // Ring color animation
    //------------------------------------------------------------------------------

    if (RingAnimTime < IndicatorAnimDuration)
    {
        RingAnimTime = FMath::Min(RingAnimTime + InDeltaTime, IndicatorAnimDuration);
        float Alpha = RingAnimTime / IndicatorAnimDuration;
        Alpha = 1.0f - FMath::Pow(1.0f - Alpha, 3.0f); // Ease out cubic

        CurrentRingColor = FMath::Lerp(StartRingColor, TargetRingColor, Alpha);
        ApplyRingStyle();
    }

    //------------------------------------------------------------------------------
    // Indicator scale animation
    //------------------------------------------------------------------------------

    if (IndicatorAnimTime < IndicatorAnimDuration)
    {
        IndicatorAnimTime = FMath::Min(IndicatorAnimTime + InDeltaTime, IndicatorAnimDuration);
        float Alpha = IndicatorAnimTime / IndicatorAnimDuration;
        Alpha = 1.0f - FMath::Pow(1.0f - Alpha, 3.0f); // Ease out cubic

        CurrentIndicatorScale = FMath::Lerp(StartIndicatorScale, TargetIndicatorScale, Alpha);
        ApplyIndicatorStyle();
    }
}

/*============================================================================
                              STATE MANAGEMENT
============================================================================*/

void UGenericRadioButton::OnRadioClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("[RadioButton] OnRadioClicked() - '%s' was clicked, current state: %s"), 
           *RadioLabelText.ToString(), bIsSelected ? TEXT("SELECTED") : TEXT("DESELECTED"));
    Toggle();
}

void UGenericRadioButton::UpdateRadioVisuals()
{
    // Start ring color transition
    StartRingColor = CurrentRingColor;
    TargetRingColor = GetTargetRingColor();
    RingAnimTime = 0.0f;

    // Start indicator scale transition
    StartIndicatorScale = CurrentIndicatorScale;
    TargetIndicatorScale = GetTargetIndicatorScale();
    IndicatorAnimTime = 0.0f;
}

float UGenericRadioButton::GetTargetIndicatorScale() const
{
    // Selected state locks the scale - hover/unhover don't affect it
    if (bIsSelected) { return IndicatorScaleSelected; }
    if (bRadioHovered) { return IndicatorScaleHovered; }
    return IndicatorScaleIdle;
}

FLinearColor UGenericRadioButton::GetTargetRingColor() const
{
    // Priority: Selected > Hovered > Idle
    if (bIsSelected) { return RingColorSelected; }
    if (bRadioHovered) { return RingColorHovered; }
    return RingColorIdle;
}

/*============================================================================
                              VISUAL STYLING
============================================================================*/

void UGenericRadioButton::ApplyRadioContent()
{
    // FIXED: Apply radio-specific content to parent's properties
    // This ensures the parent GenericButton uses our radio-specific values
    LabelText = RadioLabelText;
    IconTexture = RadioIconTexture;
    
    UE_LOG(LogTemp, Log, TEXT("[GenericRadioButton] ApplyRadioContent - Setting parent LabelText: '%s', IconTexture: %s"), 
           *LabelText.ToString(), IconTexture ? *IconTexture->GetName() : TEXT("None"));
}

void UGenericRadioButton::ApplyRingStyle()
{
    if (!RingBorder) { return; }

    // Set border brush - outline only, transparent inside
    FSlateBrush Brush;
    Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
    Brush.TintColor = FSlateColor(FLinearColor::Transparent); // Transparent inside

    // Get ring size for pill radius
    FVector2D RingSize = RingBorder->GetCachedGeometry().GetLocalSize();
    float MinDim = FMath::Max(RingSize.GetMin(), 16.0f);
    float PillRadius = MinDim * 0.5f; // [px] - Full pill

    Brush.OutlineSettings.Width = RingBorderWidth;
    Brush.OutlineSettings.Color = CurrentRingColor;
    Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    Brush.OutlineSettings.CornerRadii = FVector4(PillRadius, PillRadius, PillRadius, PillRadius);

    RingBorder->SetBrush(Brush);
}

void UGenericRadioButton::ApplyIndicatorStyle()
{
    if (!IndicatorImage) { return; }

    IndicatorImage->SetColorAndOpacity(IndicatorColor);
    IndicatorImage->SetRenderScale(FVector2D(CurrentIndicatorScale, CurrentIndicatorScale));
}

/*============================================================================
                              PUBLIC API
============================================================================*/

void UGenericRadioButton::SetSelected(bool bNewSelected)
{
    UE_LOG(LogTemp, Warning, TEXT("[RadioButton] SetSelected(%s) called on '%s' - WAS bIsSelected=%s"), 
           bNewSelected ? TEXT("TRUE") : TEXT("FALSE"), 
           *RadioLabelText.ToString(),
           bIsSelected ? TEXT("TRUE") : TEXT("FALSE"));

    if (bIsSelected == bNewSelected) 
    { 
        UE_LOG(LogTemp, Log, TEXT("[RadioButton] '%s' - No change needed, already %s"), 
               *RadioLabelText.ToString(), bIsSelected ? TEXT("SELECTED") : TEXT("DESELECTED"));
        return; 
    }

    bool bOldSelected = bIsSelected;
    bIsSelected = bNewSelected;
    
    UE_LOG(LogTemp, Warning, TEXT("[RadioButton] '%s' - State changed: %s -> %s"), 
           *RadioLabelText.ToString(),
           bOldSelected ? TEXT("SELECTED") : TEXT("DESELECTED"),
           bIsSelected ? TEXT("SELECTED") : TEXT("DESELECTED"));

    UpdateRadioVisuals();

    UE_LOG(LogTemp, Warning, TEXT("[RadioButton] '%s' - Broadcasting OnRadioToggled(%s)"), 
           *RadioLabelText.ToString(), bIsSelected ? TEXT("TRUE") : TEXT("FALSE"));

    OnRadioToggled.Broadcast(bIsSelected);
    OnRadioToggledBP(bIsSelected);

    UE_LOG(LogTemp, Warning, TEXT("[RadioButton] '%s' - SetSelected COMPLETE, final state: %s"), 
           *RadioLabelText.ToString(), bIsSelected ? TEXT("SELECTED") : TEXT("DESELECTED"));
}

void UGenericRadioButton::Toggle()
{
    UE_LOG(LogTemp, Warning, TEXT("[RadioButton] Toggle() called on '%s' - Current state: %s"), 
           *RadioLabelText.ToString(), bIsSelected ? TEXT("SELECTED") : TEXT("DESELECTED"));
    SetSelected(!bIsSelected);
}

/*============================================================================
                              CONTENT API
============================================================================*/

void UGenericRadioButton::SetRadioLabelText(const FText& NewText)
{
    RadioLabelText = NewText;
    ApplyRadioContent();
    
    // Force parent to update content
    if (ButtonText)
    {
        ButtonText->SetText(RadioLabelText);
        UE_LOG(LogTemp, Log, TEXT("[GenericRadioButton] SetRadioLabelText - Updated to: '%s'"), *RadioLabelText.ToString());
    }
}

void UGenericRadioButton::SetRadioIcon(UTexture2D* NewTexture)
{
    RadioIconTexture = NewTexture;
    ApplyRadioContent();
    
    // Force parent to update content
    if (ButtonImage)
    {
        if (RadioIconTexture)
        {
            ButtonImage->SetBrushFromTexture(RadioIconTexture);
            UE_LOG(LogTemp, Log, TEXT("[GenericRadioButton] SetRadioIcon - Updated to: %s"), *RadioIconTexture->GetName());
        }
        else
        {
            ButtonImage->SetBrushFromTexture(nullptr);
            UE_LOG(LogTemp, Log, TEXT("[GenericRadioButton] SetRadioIcon - Cleared icon"));
        }
    }
}