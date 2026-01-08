#include "GenericRadioButtonDropdown.h"
#include "Components/VerticalBoxSlot.h"

/*====================================================================================================================================
                                                         INITIALIZATION
======================================================================================================================================*/

UGenericRadioButtonDropdown::UGenericRadioButtonDropdown(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Set default radio button class if not specified
    if (!RadioButtonClass)
    {
        RadioButtonClass = UGenericRadioButton::StaticClass();
    }
}

void UGenericRadioButtonDropdown::NativeConstruct()
{
    Super::NativeConstruct();

    // Populate default options
    PopulateDefaultOptions();

    // Set initial selection
    if (DefaultSelectionIndex >= 0 && DefaultSelectionIndex < RadioButtons.Num())
    {
        SetSelectedIndex(DefaultSelectionIndex);
    }

    UE_LOG(LogTemp, Warning, TEXT("GenericRadioButtonDropdown: Constructed with %d radio options"), RadioButtons.Num());
}

/*====================================================================================================================================
                                                         RADIO OPTION MANAGEMENT
======================================================================================================================================*/

void UGenericRadioButtonDropdown::AddRadioOption(const FRadioButtonConfig& Config)
{
    if (!ContentContainer || !RadioButtonClass)
    {
        UE_LOG(LogTemp, Error, TEXT("GenericRadioButtonDropdown::AddRadioOption - Missing ContentContainer or RadioButtonClass"));
        return;
    }

    // Create radio button widget
    UGenericRadioButton* NewRadioButton = CreateWidget<UGenericRadioButton>(this, RadioButtonClass);
    if (!NewRadioButton)
    {
        UE_LOG(LogTemp, Error, TEXT("GenericRadioButtonDropdown::AddRadioOption - Failed to create radio button widget"));
        return;
    }

    // Configure radio button
    NewRadioButton->SetRadioLabelText(Config.Label);
    if (Config.Icon)
    {
        NewRadioButton->SetRadioIcon(Config.Icon);
    }

    // Bind to selection events
    NewRadioButton->OnRadioToggled.AddDynamic(this, &UGenericRadioButtonDropdown::OnRadioButtonToggled);

    // Add to content container
    UVerticalBoxSlot* RadioSlot = ContentContainer->AddChildToVerticalBox(NewRadioButton);
    if (RadioSlot)
    {
        RadioSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        RadioSlot->SetPadding(FMargin(0.0f, 2.0f));
        RadioSlot->SetHorizontalAlignment(HAlign_Fill);
        RadioSlot->SetVerticalAlignment(VAlign_Top);
    }

    // Add to our tracking array
    RadioButtons.Add(NewRadioButton);

    int32 NewIndex = RadioButtons.Num() - 1;
    UE_LOG(LogTemp, Warning, TEXT("GenericRadioButtonDropdown: Added radio option %d: %s"), NewIndex, *Config.Label.ToString());

    // Recalculate content size after adding option
    RecalculateContentSize();
}

void UGenericRadioButtonDropdown::ClearRadioOptions()
{
    // Clear selection
    CurrentSelectionIndex = -1;

    // Remove all radio buttons from container
    if (ContentContainer)
    {
        ContentContainer->ClearChildren();
    }

    // Clear tracking array
    RadioButtons.Empty();

    // Recalculate content size
    RecalculateContentSize();

    UE_LOG(LogTemp, Warning, TEXT("GenericRadioButtonDropdown: Cleared all radio options"));
}

void UGenericRadioButtonDropdown::SetSelectedIndex(int32 Index)
{
    UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown] ========== SetSelectedIndex(%d) CALLED =========="), Index);
    
    if (Index < 0 || Index >= RadioButtons.Num())
    {
        UE_LOG(LogTemp, Error, TEXT("[RadioDropdown] SetSelectedIndex - Invalid index %d (valid range: 0-%d)"), Index, RadioButtons.Num() - 1);
        return;
    }

    // Check if already selected to prevent unnecessary work
    if (CurrentSelectionIndex == Index)
    {
        UE_LOG(LogTemp, Log, TEXT("[RadioDropdown] SetSelectedIndex - Index %d already selected, skipping"), Index);
        return;
    }

    int32 PreviousSelection = CurrentSelectionIndex;
    
    UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown] BEFORE: CurrentSelectionIndex=%d, Target=%d"), PreviousSelection, Index);
    
    // Log current state before changes
    UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown] Button states BEFORE changes:"));
    for (int32 i = 0; i < RadioButtons.Num(); ++i)
    {
        if (RadioButtons[i])
        {
            bool bButtonSelected = RadioButtons[i]->IsSelected();
            UE_LOG(LogTemp, Warning, TEXT("  Button[%d] '%s': %s"), 
                   i, *RadioButtons[i]->GetRadioLabelText().ToString(), 
                   bButtonSelected ? TEXT("SELECTED") : TEXT("deselected"));
        }
    }

    // CRITICAL: Temporarily unbind all radio button events to prevent loops
    UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown] UNBINDING all radio button events..."));
    for (int32 i = 0; i < RadioButtons.Num(); ++i)
    {
        if (RadioButtons[i])
        {
            RadioButtons[i]->OnRadioToggled.RemoveDynamic(this, &UGenericRadioButtonDropdown::OnRadioButtonToggled);
        }
    }

    // STEP 1: First, deselect ALL buttons (including the current selection)
    UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown] STEP 1: Deselecting ALL buttons..."));
    for (int32 i = 0; i < RadioButtons.Num(); ++i)
    {
        if (RadioButtons[i] && RadioButtons[i]->IsSelected())
        {
            UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown]   DESELECTING button %d ('%s')"), i, *RadioButtons[i]->GetRadioLabelText().ToString());
            RadioButtons[i]->SetSelected(false);
        }
    }

    // STEP 2: Now select the target button
    UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown] STEP 2: Selecting target button..."));
    if (RadioButtons[Index])
    {
        UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown]   SELECTING button %d ('%s')"), Index, *RadioButtons[Index]->GetRadioLabelText().ToString());
        RadioButtons[Index]->SetSelected(true);
    }

    // STEP 3: Update our tracking variable AFTER the changes
    CurrentSelectionIndex = Index;
    UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown] STEP 3: Updated CurrentSelectionIndex to %d"), CurrentSelectionIndex);

    // Log state after changes
    UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown] Button states AFTER changes:"));
    for (int32 i = 0; i < RadioButtons.Num(); ++i)
    {
        if (RadioButtons[i])
        {
            bool bButtonSelected = RadioButtons[i]->IsSelected();
            UE_LOG(LogTemp, Warning, TEXT("  Button[%d] '%s': %s"), 
                   i, *RadioButtons[i]->GetRadioLabelText().ToString(), 
                   bButtonSelected ? TEXT("SELECTED") : TEXT("deselected"));
        }
    }

    // CRITICAL: Re-bind all radio button events after changes are complete
    UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown] RE-BINDING all radio button events..."));
    for (int32 i = 0; i < RadioButtons.Num(); ++i)
    {
        if (RadioButtons[i])
        {
            RadioButtons[i]->OnRadioToggled.AddDynamic(this, &UGenericRadioButtonDropdown::OnRadioButtonToggled);
        }
    }

    // Update trigger display
    if (bUpdateTriggerText)
    {
        UpdateTriggerDisplay();
    }

    // Auto-close if enabled
    if (bAutoCloseOnSelection && IsOpen())
    {
        SetOpen(false);
    }

    // Broadcast selection change
    UGenericRadioButton* SelectedButton = GetSelectedButton();
    OnSelectionChanged.Broadcast(CurrentSelectionIndex, SelectedButton);
    OnSelectionChangedBP(CurrentSelectionIndex, SelectedButton);

    UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown] ========== SetSelectedIndex COMPLETE: %d -> %d =========="), PreviousSelection, CurrentSelectionIndex);
    
    // Validate state for debugging
    ValidateRadioButtonStates();
}

UGenericRadioButton* UGenericRadioButtonDropdown::GetSelectedButton() const
{
    if (CurrentSelectionIndex < 0 || CurrentSelectionIndex >= RadioButtons.Num())
    {
        return nullptr;
    }
    return RadioButtons[CurrentSelectionIndex];
}

UGenericRadioButton* UGenericRadioButtonDropdown::GetRadioButtonAt(int32 Index) const
{
    if (Index < 0 || Index >= RadioButtons.Num())
    {
        return nullptr;
    }
    return RadioButtons[Index];
}

/*====================================================================================================================================
                                                         INTERNAL METHODS
======================================================================================================================================*/

void UGenericRadioButtonDropdown::PopulateDefaultOptions()
{
    UE_LOG(LogTemp, Warning, TEXT("GenericRadioButtonDropdown: Populating %d default options"), DefaultRadioOptions.Num());

    for (const FRadioButtonConfig& Config : DefaultRadioOptions)
    {
        AddRadioOption(Config);
    }
}

void UGenericRadioButtonDropdown::UpdateTriggerDisplay()
{
    if (!TriggerLabel) return;

    UGenericRadioButton* SelectedButton = GetSelectedButton();
    if (SelectedButton)
    {
        FText SelectedText = SelectedButton->GetRadioLabelText();
        TriggerLabel->SetText(SelectedText);
        UE_LOG(LogTemp, Log, TEXT("GenericRadioButtonDropdown: Updated trigger text to: %s"), *SelectedText.ToString());
    }
}

void UGenericRadioButtonDropdown::OnRadioButtonToggled(bool bSelected)
{
    UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown] OnRadioButtonToggled(%s) - CurrentSelectionIndex=%d"), 
           bSelected ? TEXT("TRUE") : TEXT("FALSE"), CurrentSelectionIndex);

    // Log current state of all buttons
    UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown] Current button states BEFORE processing:"));
    for (int32 i = 0; i < RadioButtons.Num(); ++i)
    {
        if (RadioButtons[i])
        {
            bool bButtonSelected = RadioButtons[i]->IsSelected();
            UE_LOG(LogTemp, Warning, TEXT("  Button[%d] '%s': %s"), 
                   i, *RadioButtons[i]->GetRadioLabelText().ToString(), 
                   bButtonSelected ? TEXT("SELECTED") : TEXT("deselected"));
        }
    }

    // Only process selection events, ignore deselection
    if (!bSelected) 
    {
        UE_LOG(LogTemp, Log, TEXT("[RadioDropdown] Ignoring deselection event"));
        return;
    }

    // CRITICAL FIX: Find which button just became selected by checking which one is different from our current selection
    int32 NewlySelectedButtonIndex = -1;
    
    for (int32 i = 0; i < RadioButtons.Num(); ++i)
    {
        if (RadioButtons[i] && RadioButtons[i]->IsSelected())
        {
            // If this button is selected but it's NOT our current selection, it must be the newly clicked one
            if (i != CurrentSelectionIndex)
            {
                NewlySelectedButtonIndex = i;
                UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown] Found NEWLY selected button at index %d ('%s') - different from current %d"), 
                       i, *RadioButtons[i]->GetRadioLabelText().ToString(), CurrentSelectionIndex);
                break;
            }
        }
    }

    // If we didn't find a newly selected button, it means the current selection was clicked again
    if (NewlySelectedButtonIndex == -1)
    {
        // Find any selected button (should be our current selection)
        for (int32 i = 0; i < RadioButtons.Num(); ++i)
        {
            if (RadioButtons[i] && RadioButtons[i]->IsSelected())
            {
                if (i == CurrentSelectionIndex)
                {
                    UE_LOG(LogTemp, Log, TEXT("[RadioDropdown] Current selection button %d clicked again, ignoring"), i);
                    return;
                }
            }
        }
        
        UE_LOG(LogTemp, Error, TEXT("[RadioDropdown] ERROR: No selected button found after selection event!"));
        return;
    }

    // Now we know which button was newly selected
    UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown] Radio button %d ('%s') toggled ON, calling SetSelectedIndex"), 
           NewlySelectedButtonIndex, *RadioButtons[NewlySelectedButtonIndex]->GetRadioLabelText().ToString());
    SetSelectedIndex(NewlySelectedButtonIndex);
}

void UGenericRadioButtonDropdown::ValidateRadioButtonStates()
{
    UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown] ========== VALIDATING BUTTON STATES =========="));
    UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown] CurrentSelectionIndex: %d"), CurrentSelectionIndex);
    
    int32 SelectedCount = 0;
    int32 ActualSelectedIndex = -1;
    
    for (int32 i = 0; i < RadioButtons.Num(); ++i)
    {
        if (RadioButtons[i])
        {
            bool bIsSelected = RadioButtons[i]->IsSelected();
            FString ButtonLabel = RadioButtons[i]->GetRadioLabelText().ToString();
            
            if (bIsSelected)
            {
                SelectedCount++;
                ActualSelectedIndex = i;
                UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown]   Button[%d] '%s': ✅ SELECTED"), i, *ButtonLabel);
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("[RadioDropdown]   Button[%d] '%s': ❌ deselected"), i, *ButtonLabel);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[RadioDropdown]   Button[%d]: NULL POINTER!"), i);
        }
    }
    
    // Validation checks
    if (SelectedCount == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[RadioDropdown] ❌ INVALID STATE - NO radio buttons selected!"));
    }
    else if (SelectedCount > 1)
    {
        UE_LOG(LogTemp, Error, TEXT("[RadioDropdown] ❌ INVALID STATE - %d radio buttons selected (should be exactly 1)"), SelectedCount);
    }
    else if (ActualSelectedIndex != CurrentSelectionIndex)
    {
        UE_LOG(LogTemp, Error, TEXT("[RadioDropdown] ❌ INVALID STATE - CurrentSelectionIndex=%d but button %d is actually selected"), 
               CurrentSelectionIndex, ActualSelectedIndex);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown] ✅ Valid state - exactly 1 radio button selected at correct index"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[RadioDropdown] ========== VALIDATION COMPLETE =========="));
}