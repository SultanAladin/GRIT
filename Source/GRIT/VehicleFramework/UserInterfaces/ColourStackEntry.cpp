#include "ColourStackEntry.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateBrush.h"


#include "Styling/SlateTypes.h"


#include "Engine/Engine.h"

UColourStackEntry::UColourStackEntry(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UColourStackEntry::NativePreConstruct()
{
    Super::NativePreConstruct();
    InitializeEntryTheme();
}

void UColourStackEntry::NativeConstruct()
{
    Super::NativeConstruct();

    ConfigureChromaSelectorStyles();
    RefreshVisualElements();

    if (ChromaSelector)
    {
        ChromaSelector->OnClicked.AddDynamic(this, &UColourStackEntry::OnChromaSelectorClicked);
        ChromaSelector->OnHovered.AddDynamic(this, &UColourStackEntry::OnChromaSelectorHovered);
        ChromaSelector->OnUnhovered.AddDynamic(this, &UColourStackEntry::OnChromaSelectorUnhovered);
    }
}

void UColourStackEntry::InitializeEntryTheme()
{
    if (HueIdentifier)
    {
        HueIdentifier->SetColorAndOpacity(FSlateColor(EntryTextColor));
        HueIdentifier->SetText(FText::FromString(ColorDisplayName));
    }

    if (TintSample)
    {
        TintSample->SetColorAndOpacity(FSlateColor(ColorPreview).GetSpecifiedColor());
    }
}

void UColourStackEntry::ConfigureChromaSelectorStyles()
{
    if (!ChromaSelector)
        return;

    // Get a fresh copy of the button style
    FButtonStyle Style = ChromaSelector->GetStyle();

    // Determine the brush draw type based on corner preference
    ESlateBrushDrawType::Type BrushType = bUseRoundedCorners
        ? ESlateBrushDrawType::RoundedBox
        : ESlateBrushDrawType::Box;

    // Configure Normal state
    Style.Normal.TintColor = FSlateColor(EntryBackgroundColor);
    Style.Normal.DrawAs = BrushType;
    if (bUseRoundedCorners)
    {
        // Set a reasonable corner radius for rounded corners
        Style.Normal.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
        Style.Normal.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    }
    else
    {
        // Reset corner radius for square corners
        Style.Normal.OutlineSettings.CornerRadii = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
        Style.Normal.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    }

    // Configure Hovered state
    Style.Hovered.TintColor = FSlateColor(EntryHighlightColor);
    Style.Hovered.DrawAs = BrushType;
    if (bUseRoundedCorners)
    {
        Style.Hovered.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
        Style.Hovered.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    }
    else
    {
        Style.Hovered.OutlineSettings.CornerRadii = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
        Style.Hovered.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    }

    // Configure Pressed state
    Style.Pressed.TintColor = FSlateColor(EntryHighlightColor);
    Style.Pressed.DrawAs = BrushType;
    if (bUseRoundedCorners)
    {
        Style.Pressed.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
        Style.Pressed.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    }
    else
    {
        Style.Pressed.OutlineSettings.CornerRadii = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
        Style.Pressed.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    }

    // Remove pressed offset/scale effect
    Style.SetNormalPadding(FMargin(0));
    Style.SetPressedPadding(FMargin(0));

    // Optional: silence pressed sound
    Style.SetPressedSound(FSlateSound());

    // Apply the updated style back to the UButton
    ChromaSelector->SetStyle(Style);
}


void UColourStackEntry::RefreshVisualElements()
{
    if (HueIdentifier)
    {
        HueIdentifier->SetText(FText::FromString(ColorDisplayName));
    }

    if (TintSample)
    {
        TintSample->SetColorAndOpacity(FSlateColor(ColorPreview).GetSpecifiedColor());
    }
}

void UColourStackEntry::OnChromaSelectorClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("ColourStackEntry: Color selected - %s"), *ColorDisplayName);

    if (OnColorEntrySelected.IsBound())
    {
        OnColorEntrySelected.Broadcast(this, ColorID);
    }

    SetSelected(true);
}

void UColourStackEntry::OnChromaSelectorHovered()
{
    if (HueIdentifier)
    {
        HueIdentifier->SetColorAndOpacity(FSlateColor(EntryTextHoverColor));
    }
}

void UColourStackEntry::OnChromaSelectorUnhovered()
{
    if (HueIdentifier)
    {
        HueIdentifier->SetColorAndOpacity(FSlateColor(EntryTextColor));
    }
}

void UColourStackEntry::SetColorData(const FString& InColorName, const FLinearColor& InColorPreview, const FString& InColorID)
{
    ColorDisplayName = InColorName;
    ColorPreview     = InColorPreview;
    ColorID          = InColorID.IsEmpty() ? InColorName : InColorID;

    RefreshVisualElements();
}

void UColourStackEntry::UpdateColorPreview(const FLinearColor& NewColor)
{
    ColorPreview = NewColor;

    if (TintSample)
    {
        TintSample->SetColorAndOpacity(FSlateColor(ColorPreview).GetSpecifiedColor());
    }
}

void UColourStackEntry::SetSelected(bool bIsSelected)
{
    bIsCurrentlySelected = bIsSelected;

    if (ChromaSelector)
    {
        if (bIsCurrentlySelected)
        {
            FButtonStyle SelectedStyle = ChromaSelector->GetStyle();
            
            // Set the selected tint color
            SelectedStyle.Normal.TintColor = FSlateColor(EntryHighlightColor * 0.8f);
            
            // Preserve the corner style based on bUseRoundedCorners
            ESlateBrushDrawType::Type BrushType = bUseRoundedCorners
                ? ESlateBrushDrawType::RoundedBox
                : ESlateBrushDrawType::Box;
            
            SelectedStyle.Normal.DrawAs = BrushType;
            SelectedStyle.Hovered.DrawAs = BrushType;
            SelectedStyle.Pressed.DrawAs = BrushType;
            
            ChromaSelector->SetStyle(SelectedStyle);
        }
        else
        {
            // Reset to default styling
            ConfigureChromaSelectorStyles();
        }
    }
}