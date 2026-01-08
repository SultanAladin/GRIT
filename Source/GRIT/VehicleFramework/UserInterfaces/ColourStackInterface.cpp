#include "ColourStackInterface.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Components/Spacer.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateBrush.h"
#include "Engine/Engine.h"
#include "Blueprint/WidgetTree.h"
#include "TimerManager.h"

UColourStackInterface::UColourStackInterface(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , CurrentlySelectedEntry(nullptr)
{
    // Set default widget class if not specified
    if (!ColorEntryWidgetClass)
    {
        ColorEntryWidgetClass = UColourStackEntry::StaticClass();
    }
}

void UColourStackInterface::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    // ALWAYS initialize default colors in PreConstruct to override Blueprint values
    InitializeDefaultColors();
    UE_LOG(LogTemp, Warning, TEXT("NativePreConstruct: Initialized %d colors"), ColorPalette.Num());
    
    InitializeTheme();
}

void UColourStackInterface::NativeConstruct()
{
    Super::NativeConstruct();

    // FORCE initialize colors to ensure we have all 20 colors
    InitializeDefaultColors();
    UE_LOG(LogTemp, Warning, TEXT("NativeConstruct: Forced initialization - %d colors"), ColorPalette.Num());

    SetupButtonStyles();

    // Bind button events first
    if (Btn_Apply)
    {
        Btn_Apply->OnClicked.AddDynamic(this, &UColourStackInterface::OnApplyButtonClicked);
        Btn_Apply->OnHovered.AddDynamic(this, &UColourStackInterface::OnApplyButtonHovered);
        Btn_Apply->OnUnhovered.AddDynamic(this, &UColourStackInterface::OnApplyButtonUnhovered);
    }

    // Delay the population to ensure all widgets are fully constructed
    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
    {
        DelayedPopulateColorList();
    });
}

void UColourStackInterface::DelayedPopulateColorList()
{
    // FORCE initialize colors again to make sure we have all entries
    InitializeDefaultColors();
    UE_LOG(LogTemp, Warning, TEXT("DelayedPopulateColorList: ColorPalette now has %d entries"), ColorPalette.Num());
    
    if (ColorStreamContainer && ColorPalette.Num() > 0 && ColorEntryWidgetClass)
    {
        PopulateColorList();
        UE_LOG(LogTemp, Warning, TEXT("DelayedPopulateColorList: PopulateColorList completed"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DelayedPopulateColorList: Prerequisites not met!"));
        UE_LOG(LogTemp, Error, TEXT("  ColorStreamContainer: %s"), ColorStreamContainer ? TEXT("Valid") : TEXT("NULL"));
        UE_LOG(LogTemp, Error, TEXT("  ColorPalette.Num(): %d"), ColorPalette.Num());
        UE_LOG(LogTemp, Error, TEXT("  ColorEntryWidgetClass: %s"), ColorEntryWidgetClass ? TEXT("Valid") : TEXT("NULL"));
        
        // Try again with a slight delay if prerequisites aren't met
        if (GetWorld())
        {
            FTimerHandle RetryTimer;
            GetWorld()->GetTimerManager().SetTimer(RetryTimer, [this]()
            {
                InitializeDefaultColors();
                if (ColorStreamContainer && ColorPalette.Num() > 0 && ColorEntryWidgetClass)
                {
                    PopulateColorList();
                }
            }, 0.1f, false);
        }
    }
}

void UColourStackInterface::InitializeTheme()
{
    // Set main interface background color
    if (ColorStackInterface)
    {
        ColorStackInterface->SetBrushColor(BackgroundColor);
    }

    // Set header title text color to white
    if (HeaderTitle)
    {
        FSlateColor TextColor = FSlateColor(FontColor);
        HeaderTitle->SetColorAndOpacity(TextColor);
    }

    // Set apply button text color to white
    if (Txt_Apply)
    {
        FSlateColor TextColor = FSlateColor(ButtonTextColor);
        Txt_Apply->SetColorAndOpacity(TextColor);
    }
}

void UColourStackInterface::SetupButtonStyles()
{
    if (!Btn_Apply)
        return;

    // Create button style with grey background
    FButtonStyle ButtonStyle = Btn_Apply->GetStyle();

    // Normal state: Grey button, white text
    FSlateColor NormalTint = FSlateColor(ButtonColor);
    ButtonStyle.Normal.TintColor = NormalTint;
    ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::RoundedBox;

    // Hovered state: White button (will change text to grey in event handler)
    FSlateColor HoverTint = FSlateColor(HighlightColor);
    ButtonStyle.Hovered.TintColor = HoverTint;
    ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::RoundedBox;

    // Pressed state: White button
    ButtonStyle.Pressed.TintColor = HoverTint;
    ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::RoundedBox;

    // Apply the style
    Btn_Apply->SetStyle(ButtonStyle);
}

void UColourStackInterface::InitializeDefaultColors()
{
    // Initialize with 20 colors from different categories
    ColorPalette.Empty();
    
    // Fruit Colors (10)
    ColorPalette.Add(FColorPaletteEntry(TEXT("Lime"), FLinearColor(0.196f, 0.804f, 0.196f, 1.0f), EColorCategory::Fruit));
    ColorPalette.Add(FColorPaletteEntry(TEXT("Banana"), FLinearColor(1.0f, 1.0f, 0.0f, 1.0f), EColorCategory::Fruit));
    ColorPalette.Add(FColorPaletteEntry(TEXT("Watermelon"), FLinearColor(1.0f, 0.412f, 0.706f, 1.0f), EColorCategory::Fruit));
    ColorPalette.Add(FColorPaletteEntry(TEXT("Grape"), FLinearColor(0.502f, 0.0f, 0.502f, 1.0f), EColorCategory::Fruit));
    ColorPalette.Add(FColorPaletteEntry(TEXT("Cherry"), FLinearColor(0.863f, 0.078f, 0.235f, 1.0f), EColorCategory::Fruit));
    ColorPalette.Add(FColorPaletteEntry(TEXT("Orange"), FLinearColor(1.0f, 0.647f, 0.0f, 1.0f), EColorCategory::Fruit));
    ColorPalette.Add(FColorPaletteEntry(TEXT("Blueberry"), FLinearColor(0.255f, 0.412f, 0.882f, 1.0f), EColorCategory::Fruit));
    ColorPalette.Add(FColorPaletteEntry(TEXT("Peach"), FLinearColor(1.0f, 0.855f, 0.725f, 1.0f), EColorCategory::Fruit));
    ColorPalette.Add(FColorPaletteEntry(TEXT("Strawberry"), FLinearColor(1.0f, 0.271f, 0.0f, 1.0f), EColorCategory::Fruit));
    ColorPalette.Add(FColorPaletteEntry(TEXT("Mango"), FLinearColor(1.0f, 0.773f, 0.165f, 1.0f), EColorCategory::Fruit));
    
    // Pastel Colors (3)
    ColorPalette.Add(FColorPaletteEntry(TEXT("Soft Pink"), FLinearColor(1.0f, 0.714f, 0.757f, 1.0f), EColorCategory::Pastel));
    ColorPalette.Add(FColorPaletteEntry(TEXT("Baby Blue"), FLinearColor(0.678f, 0.847f, 0.902f, 1.0f), EColorCategory::Pastel));
    ColorPalette.Add(FColorPaletteEntry(TEXT("Mint Green"), FLinearColor(0.596f, 0.984f, 0.596f, 1.0f), EColorCategory::Pastel));
    
    // Matter Colors (4)
    ColorPalette.Add(FColorPaletteEntry(TEXT("Steel Gray"), FLinearColor(0.439f, 0.502f, 0.565f, 1.0f), EColorCategory::Matter));
    ColorPalette.Add(FColorPaletteEntry(TEXT("Concrete"), FLinearColor(0.663f, 0.663f, 0.663f, 1.0f), EColorCategory::Matter));
    ColorPalette.Add(FColorPaletteEntry(TEXT("Copper"), FLinearColor(0.722f, 0.451f, 0.200f, 1.0f), EColorCategory::Matter));
    ColorPalette.Add(FColorPaletteEntry(TEXT("Charcoal"), FLinearColor(0.212f, 0.271f, 0.310f, 1.0f), EColorCategory::Matter));
    
    // Nature Colors (3)
    ColorPalette.Add(FColorPaletteEntry(TEXT("Forest Green"), FLinearColor(0.133f, 0.545f, 0.133f, 1.0f), EColorCategory::Nature));
    ColorPalette.Add(FColorPaletteEntry(TEXT("Sky Blue"), FLinearColor(0.529f, 0.808f, 0.922f, 1.0f), EColorCategory::Nature));
    ColorPalette.Add(FColorPaletteEntry(TEXT("Earth Brown"), FLinearColor(0.627f, 0.322f, 0.176f, 1.0f), EColorCategory::Nature));
}

void UColourStackInterface::PopulateColorList()
{
    if (!ColorStreamContainer || ColorPalette.Num() == 0 || !ColorEntryWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("PopulateColorList: Prerequisites failed!"));
        return;
    }

    // Check if ColorEntryWidgetClass is valid and is a subclass of UColourStackEntry
    if (!ColorEntryWidgetClass->IsChildOf(UColourStackEntry::StaticClass()))
    {
        UE_LOG(LogTemp, Error, TEXT("PopulateColorList: ColorEntryWidgetClass is not a subclass of UColourStackEntry"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("PopulateColorList: Starting with %d colors"), ColorPalette.Num());

    // Clear any existing entries
    ColorStreamContainer->ClearChildren();
    ColorEntries.Empty();

    // Create and add color entries to the scroll box
    for (int32 i = 0; i < ColorPalette.Num(); ++i)
    {
        const FColorPaletteEntry& ColorData = ColorPalette[i];
        
        UE_LOG(LogTemp, Warning, TEXT("Creating color entry %d: %s"), i, *ColorData.ColorName);
        
        // Try creating the widget
        UColourStackEntry* ColorEntry = CreateWidget<UColourStackEntry>(this, ColorEntryWidgetClass);
        
        if (!ColorEntry)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create widget for %s"), *ColorData.ColorName);
            continue;
        }

        // Set the color data
        ColorEntry->SetColorData(ColorData.ColorName, ColorData.Color, ColorData.ColorName);
        
        // Bind to the selection event
        ColorEntry->OnColorEntrySelected.AddDynamic(this, &UColourStackInterface::OnColorEntrySelected);
        
        // Add to scroll box
        ColorStreamContainer->AddChild(ColorEntry);
        
        // Add spacer after each item (except the last one)
        if (i < ColorPalette.Num() - 1)
        {
            USpacer* ItemSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
            if (ItemSpacer)
            {
                ItemSpacer->SetSize(FVector2D(1.0f, 10.0f)); // 10px height spacer
                ColorStreamContainer->AddChild(ItemSpacer);
            }
        }
        
        // Store reference for management
        ColorEntries.Add(ColorEntry);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("PopulateColorList: Created %d color entries, ScrollBox has %d children"), 
           ColorEntries.Num(), ColorStreamContainer->GetChildrenCount());
    
    // Force a layout update
    if (ColorStreamContainer->GetParent())
    {
        ColorStreamContainer->GetParent()->InvalidateLayoutAndVolatility();
    }
}

void UColourStackInterface::OnColorEntrySelected(UColourStackEntry* SelectedEntry, const FString& ColorID)
{
    // Deselect the previously selected entry
    if (CurrentlySelectedEntry && CurrentlySelectedEntry != SelectedEntry)
    {
        CurrentlySelectedEntry->SetSelected(false);
    }

    // Set the new selected entry
    CurrentlySelectedEntry = SelectedEntry;
}

void UColourStackInterface::OnApplyButtonClicked()
{
    if (CurrentlySelectedEntry)
    {
        FString SelectedColorID = CurrentlySelectedEntry->GetColorID();
        FLinearColor SelectedColor = CurrentlySelectedEntry->GetColorPreview();

        // Reset the selected state after applying
        CurrentlySelectedEntry->SetSelected(false);
        CurrentlySelectedEntry = nullptr;

        // Add your color application logic here
        // Example: ApplySelectedColors(SelectedColorID, SelectedColor);
    }
}

void UColourStackInterface::OnApplyButtonHovered()
{
    // Change text color to grey when hovering (button becomes white)
    if (Txt_Apply)
    {
        FSlateColor HoverTextColor = FSlateColor(ButtonTextHoverColor);
        Txt_Apply->SetColorAndOpacity(HoverTextColor);
    }
}

void UColourStackInterface::OnApplyButtonUnhovered()
{
    // Change text color back to white when not hovering (button becomes grey)
    if (Txt_Apply)
    {
        FSlateColor NormalTextColor = FSlateColor(ButtonTextColor);
        Txt_Apply->SetColorAndOpacity(NormalTextColor);
    }
}

// DEBUG FUNCTIONS
void UColourStackInterface::ForceInitializeColors()
{
    InitializeDefaultColors();
}

void UColourStackInterface::ForcePopulateColorList()
{
    if (ColorPalette.Num() == 0)
    {
        InitializeDefaultColors();
    }
    
    PopulateColorList();
}

void UColourStackInterface::DebugWidgetHierarchy()
{
    UE_LOG(LogTemp, Warning, TEXT("=== WIDGET HIERARCHY DEBUG ==="));
    
    UE_LOG(LogTemp, Warning, TEXT("ColorStackInterface: %s"), ColorStackInterface ? TEXT("Valid") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("MainStackContainer: %s"), MainStackContainer ? TEXT("Valid") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("ColourStreamEntryContainer: %s"), ColourStreamEntryContainer ? TEXT("Valid") : TEXT("NULL"));
    
    if (ColorStreamContainer)
    {
        UE_LOG(LogTemp, Warning, TEXT("ColorStreamContainer: Valid, Children: %d"), ColorStreamContainer->GetChildrenCount());
        UE_LOG(LogTemp, Warning, TEXT("ColorStreamContainer Visibility: %s"), 
               ColorStreamContainer->GetVisibility() == ESlateVisibility::Visible ? TEXT("Visible") : TEXT("Not Visible"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ColorStreamContainer: NULL"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("ColorPalette size: %d"), ColorPalette.Num());
    UE_LOG(LogTemp, Warning, TEXT("ColorEntries size: %d"), ColorEntries.Num());
    UE_LOG(LogTemp, Warning, TEXT("ColorEntryWidgetClass: %s"), 
           ColorEntryWidgetClass ? *ColorEntryWidgetClass->GetName() : TEXT("NULL"));
}

void UColourStackInterface::TestWidgetCreation()
{
    if (!ColorEntryWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("TEST FAILED: ColorEntryWidgetClass is NULL"));
        return;
    }

    if (!ColorStreamContainer)
    {
        UE_LOG(LogTemp, Warning, TEXT("TEST FAILED: ColorStreamContainer is NULL"));
        return;
    }

    // Try to create a single test widget
    UColourStackEntry* TestWidget = CreateWidget<UColourStackEntry>(this, ColorEntryWidgetClass);
    if (TestWidget)
    {
        TestWidget->SetColorData(TEXT("Test Color"), FLinearColor::Red, TEXT("test"));
        
        UPanelSlot* AddedSlot = ColorStreamContainer->AddChild(TestWidget);
        
        if (AddedSlot)
        {
            UE_LOG(LogTemp, Warning, TEXT("TEST SUCCESS: Widget created and added successfully"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("TEST FAILED: Widget created but failed to add to ScrollBox"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("TEST FAILED: Could not create test widget"));
    }
}