// ColorSelectorWidget.cpp
#include "ColorSelectorWidget.h"
#include "ColorItemWidget.h"
#include "ColorDataObject.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

UColorSelectorWidget::UColorSelectorWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , SelectedColorIndex(-1)
    , bHasAppliedSelection(false)
{
}

void UColorSelectorWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    if (ColorOptions.Num() == 0)
    {
        SetupDefaultColors();
    }
}

void UColorSelectorWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    if (TitleText)
    {
        TitleText->SetText(WidgetTitle);
    }
    
    if (ApplyButton)
    {
        ApplyButton->OnClicked.AddDynamic(this, &UColorSelectorWidget::OnApplyButtonClicked);
        // Set initial button text
        UpdateApplyButtonText();
    }
    
    if (FilterButton)
    {
        FilterButton->OnClicked.AddDynamic(this, &UColorSelectorWidget::OnFilterButtonClicked);
    }

    // Setup ListView with entry widget generation callback
    if (ColourListView)
    {
        ColourListView->OnEntryWidgetGenerated().AddUObject(this, &UColorSelectorWidget::OnListEntryWidgetGenerated);
    }
    
    PopulateColorList();
    SetSelectedColor(DefaultSelectedIndex);
}

void UColorSelectorWidget::OnListEntryWidgetGenerated(UUserWidget& EntryWidget)
{
    UColorItemWidget* ColorItemWidget = Cast<UColorItemWidget>(&EntryWidget);
    if (ColorItemWidget)
    {
        // Bind to the color item's selection event
        ColorItemWidget->OnColorItemSelected.AddDynamic(this, &UColorSelectorWidget::OnColorItemSelected);
        
        // Apply styling to the newly created widget
        ColorItemWidget->SetStyling(
            ItemNormalColor,
            ItemHoveredColor,
            ItemSelectedColor,
            SelectedBorderColor,
            BorderRadius
        );
        
        // Ensure proper selection state when widget is generated
        if (ColorItemWidget->GetColorDataObject())
        {
            int32 ItemIndex = ColorItemWidget->GetColorDataObject()->ItemIndex;
            ColorItemWidget->SetSelected(ItemIndex == SelectedColorIndex);
        }
    }
}

void UColorSelectorWidget::PopulateColorList()
{
    if (!ColourListView)
        return;

    // Clear existing data
    ColourListView->ClearListItems();
    ColorDataObjects.Empty();

    // Create data objects for ListView
    CreateColorDataObjects();

    // Set the data source for ListView
    for (UColorDataObject* DataObject : ColorDataObjects)
    {
        ColourListView->AddItem(DataObject);
    }
    
    // Force update selection after populating
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
    {
        UpdateSelection();
    }, 0.1f, false);
}

void UColorSelectorWidget::CreateColorDataObjects()
{
    for (int32 i = 0; i < ColorOptions.Num(); ++i)
    {
        UColorDataObject* DataObject = NewObject<UColorDataObject>(this);
        DataObject->SetColorData(ColorOptions[i].ColorName, ColorOptions[i].ColorValue, i);
        ColorDataObjects.Add(DataObject);
    }
}

void UColorSelectorWidget::InitializeWithTestData()
{
    // Force setup default colors
    SetupDefaultColors();
    
    // Force populate the list
    PopulateColorList();
    
    // Set default selection
    SetSelectedColor(1); // Select "Azul White" by default
    
    // Debug output
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, 
            FString::Printf(TEXT("Color Selector initialized with %d colors"), ColorOptions.Num()));
    }
}

void UColorSelectorWidget::SetSelectedColor(int32 Index)
{
    if (Index >= 0 && Index < ColorOptions.Num())
    {
        // Store the previous selection
        int32 PreviousSelection = SelectedColorIndex;
        SelectedColorIndex = Index;
        
        // Reset applied state since we have a new selection
        bHasAppliedSelection = false;
        
        // Update visual selection
        UpdateSelection();
        
        // Update Apply button text
        UpdateApplyButtonText();
        
        // Broadcast events only if selection actually changed
        if (PreviousSelection != SelectedColorIndex)
        {
            const FColorOption& SelectedColor = ColorOptions[Index];
            OnColorSelected.Broadcast(SelectedColor.ColorName, SelectedColor.ColorValue);
            OnColorSelectedBP(SelectedColor.ColorName, SelectedColor.ColorValue);
            
            // Debug output
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, 
                    FString::Printf(TEXT("Selected Color: %s"), *SelectedColor.ColorName));
            }
        }
    }
}

void UColorSelectorWidget::SetSelectedColorByName(const FString& ColorName)
{
    for (int32 i = 0; i < ColorOptions.Num(); ++i)
    {
        if (ColorOptions[i].ColorName == ColorName)
        {
            SetSelectedColor(i);
            break;
        }
    }
}

FColorOption UColorSelectorWidget::GetSelectedColor() const
{
    if (SelectedColorIndex >= 0 && SelectedColorIndex < ColorOptions.Num())
    {
        return ColorOptions[SelectedColorIndex];
    }
    return FColorOption();
}

FString UColorSelectorWidget::GetSelectedColorName() const
{
    if (SelectedColorIndex >= 0 && SelectedColorIndex < ColorOptions.Num())
    {
        return ColorOptions[SelectedColorIndex].ColorName;
    }
    return TEXT("");
}

FLinearColor UColorSelectorWidget::GetSelectedColorValue() const
{
    if (SelectedColorIndex >= 0 && SelectedColorIndex < ColorOptions.Num())
    {
        return ColorOptions[SelectedColorIndex].ColorValue;
    }
    return FLinearColor::White;
}

int32 UColorSelectorWidget::GetSelectedColorIndex() const
{
    return SelectedColorIndex;
}

void UColorSelectorWidget::AddColorOption(const FString& ColorName, const FLinearColor& ColorValue)
{
    ColorOptions.Add(FColorOption(ColorName, ColorValue));
    PopulateColorList();
}

void UColorSelectorWidget::ClearColorOptions()
{
    ColorOptions.Empty();
    SelectedColorIndex = -1;
    bHasAppliedSelection = false;
    PopulateColorList();
    UpdateApplyButtonText();
}

void UColorSelectorWidget::OnApplyButtonClicked()
{
    if (SelectedColorIndex >= 0 && SelectedColorIndex < ColorOptions.Num())
    {
        const FColorOption& SelectedColor = ColorOptions[SelectedColorIndex];
        bHasAppliedSelection = true;
        
        // Update button text
        UpdateApplyButtonText();
        
        // Broadcast events
        OnColorApplied.Broadcast(SelectedColor.ColorName, SelectedColor.ColorValue);
        OnColorAppliedBP(SelectedColor.ColorName, SelectedColor.ColorValue);
        
        // Debug output
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, 
                FString::Printf(TEXT("Applied Color: %s"), *SelectedColor.ColorName));
        }
    }
}

void UColorSelectorWidget::OnFilterButtonClicked()
{
    OnFilterButtonClickedBP();
}

void UColorSelectorWidget::OnColorItemSelected(int32 ItemIndex)
{
    SetSelectedColor(ItemIndex);
}

void UColorSelectorWidget::UpdateSelection()
{
    if (!ColourListView)
        return;

    // Clear all selections first - this ensures only one item is selected
    for (int32 i = 0; i < ColorDataObjects.Num(); ++i)
    {
        UUserWidget* EntryWidget = ColourListView->GetEntryWidgetFromItem(ColorDataObjects[i]);
        if (EntryWidget)
        {
            UColorItemWidget* ColorItemWidget = Cast<UColorItemWidget>(EntryWidget);
            if (ColorItemWidget)
            {
                ColorItemWidget->SetSelected(false);
            }
        }
    }
    
    // Now set the correct selection
    if (SelectedColorIndex >= 0 && SelectedColorIndex < ColorDataObjects.Num())
    {
        UUserWidget* SelectedEntryWidget = ColourListView->GetEntryWidgetFromItem(ColorDataObjects[SelectedColorIndex]);
        if (SelectedEntryWidget)
        {
            UColorItemWidget* SelectedColorItemWidget = Cast<UColorItemWidget>(SelectedEntryWidget);
            if (SelectedColorItemWidget)
            {
                SelectedColorItemWidget->SetSelected(true);
            }
        }
    }
    
    // Also update any displayed widgets (fallback)
    TArray<UUserWidget*> DisplayedWidgets = ColourListView->GetDisplayedEntryWidgets();
    for (UUserWidget* Widget : DisplayedWidgets)
    {
        UColorItemWidget* ColorItemWidget = Cast<UColorItemWidget>(Widget);
        if (ColorItemWidget && ColorItemWidget->GetColorDataObject())
        {
            int32 ItemIndex = ColorItemWidget->GetColorDataObject()->ItemIndex;
            ColorItemWidget->SetSelected(ItemIndex == SelectedColorIndex);
        }
    }
}

void UColorSelectorWidget::UpdateApplyButtonText()
{
    if (ApplyButton)
    {
        UTextBlock* ButtonTextBlock = nullptr;
        
        // Method 1: Use direct reference if available
        if (ApplyButtonText)
        {
            ButtonTextBlock = ApplyButtonText;
        }
        // Method 2: Try to find by name "ApplyButtonText" (common naming convention)
        else
        {
            ButtonTextBlock = Cast<UTextBlock>(GetWidgetFromName(TEXT("ApplyButtonText")));
        }
        
        // Method 3: If not found, look for any TextBlock child in the button
        if (!ButtonTextBlock)
        {
            UPanelWidget* ButtonPanel = Cast<UPanelWidget>(ApplyButton->GetChildAt(0));
            if (ButtonPanel)
            {
                for (int32 i = 0; i < ButtonPanel->GetChildrenCount(); ++i)
                {
                    ButtonTextBlock = Cast<UTextBlock>(ButtonPanel->GetChildAt(i));
                    if (ButtonTextBlock)
                        break;
                }
            }
            else
            {
                // Direct TextBlock child
                ButtonTextBlock = Cast<UTextBlock>(ApplyButton->GetChildAt(0));
            }
        }
        
        // Update the text if we found the TextBlock
        if (ButtonTextBlock)
        {
            if (bHasAppliedSelection && SelectedColorIndex >= 0)
            {
                // Show "Applied" when color has been applied
                ButtonTextBlock->SetText(FText::FromString("Applied"));
            }
            else if (SelectedColorIndex >= 0)
            {
                // Show "Apply" when color is selected but not applied
                ButtonTextBlock->SetText(FText::FromString("Apply"));
            }
            else
            {
                // Show default text when no selection
                ButtonTextBlock->SetText(FText::FromString("Select Color"));
            }
        }
        else
        {
            // Debug output if TextBlock not found
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, 
                    TEXT("Could not find TextBlock in ApplyButton. Please add ApplyButtonText widget or ensure button has TextBlock child."));
            }
        }
    }
}

void UColorSelectorWidget::SetupDefaultColors()
{
    ColorOptions.Empty();
    ColorOptions.Add(FColorOption(TEXT("Blanco Puro"), FLinearColor(0.97f, 0.98f, 0.98f, 1.0f)));
    ColorOptions.Add(FColorOption(TEXT("Azul White"), FLinearColor(0.89f, 0.95f, 0.99f, 1.0f)));
    ColorOptions.Add(FColorOption(TEXT("Rojo Carmesí"), FLinearColor(0.55f, 0.31f, 0.31f, 1.0f)));
    ColorOptions.Add(FColorOption(TEXT("Naranja Brillante"), FLinearColor(0.61f, 0.42f, 0.65f, 1.0f)));
    ColorOptions.Add(FColorOption(TEXT("Violeta Profundo"), FLinearColor(0.63f, 0.40f, 0.48f, 1.0f)));
    ColorOptions.Add(FColorOption(TEXT("Rosa Pálido"), FLinearColor(0.48f, 0.60f, 0.54f, 1.0f)));
    ColorOptions.Add(FColorOption(TEXT("Gris Azul"), FLinearColor(0.42f, 0.48f, 0.54f, 1.0f)));
    ColorOptions.Add(FColorOption(TEXT("Verde Esmeralda"), FLinearColor(0.18f, 0.49f, 0.35f, 1.0f)));
}