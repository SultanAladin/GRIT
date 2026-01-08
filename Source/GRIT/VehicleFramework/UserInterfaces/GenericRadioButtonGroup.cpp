#include "GenericRadioButtonGroup.h"

/*====================================================================================================================================
                                                         INITIALIZATION
======================================================================================================================================*/

UGenericRadioButtonGroup::UGenericRadioButtonGroup(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGenericRadioButtonGroup::NativeConstruct()
{
	Super::NativeConstruct();

	// Reason: Apply default selection on first construct
	if (DefaultSelectionIndex >= 0 && DefaultSelectionIndex < RadioButtons.Num())
	{
		ForceSelection(DefaultSelectionIndex);
	} // End if (DefaultSelectionIndex valid)
}

/*====================================================================================================================================
                                                         RADIO BUTTON MANAGEMENT
======================================================================================================================================*/

void UGenericRadioButtonGroup::BindRadioButton(UGenericRadioButton* RadioButton)
{
	// Reason: Validate button before binding
	if (!RadioButton) { return; }
	
	// Reason: Prevent duplicate bindings
	if (RadioButtons.Contains(RadioButton)) { return; }
	
	RadioButtons.Add(RadioButton);
	RadioButton->OnRadioToggled.AddDynamic(this, &UGenericRadioButtonGroup::OnRadioButtonToggled);
	
	// Reason: Auto-select first button if none selected
	if (CurrentSelectionIndex < 0 && RadioButtons.Num() == 1)
	{
		ForceSelection(0);
	} // End if (First button auto-select)
}

void UGenericRadioButtonGroup::UnbindRadioButton(UGenericRadioButton* RadioButton)
{
	// Reason: Validate button before unbinding
	if (!RadioButton) { return; }
	
	int32 Index = RadioButtons.Find(RadioButton);
	
	// Reason: Only unbind if button exists in group
	if (Index != INDEX_NONE)
	{
		RadioButton->OnRadioToggled.RemoveDynamic(this, &UGenericRadioButtonGroup::OnRadioButtonToggled);
		RadioButtons.RemoveAt(Index);
		
		// Reason: Clear selection if removed button was selected
		if (CurrentSelectionIndex == Index)
		{
			CurrentSelectionIndex = -1;
		}
		// Reason: Adjust index if removed button was before current selection
		else if (CurrentSelectionIndex > Index)
		{
			CurrentSelectionIndex--;
		} // End if (Index adjustment)
	} // End if (Button found)
}

void UGenericRadioButtonGroup::ForceSelection(int32 Index)
{
	// Reason: Validate index bounds
	if (Index < 0 || Index >= RadioButtons.Num()) { return; }
	
	LockSelection(Index);
}

UGenericRadioButton* UGenericRadioButtonGroup::GetCurrentButton() const
{
	// Reason: Return nullptr if no valid selection
	if (CurrentSelectionIndex < 0 || CurrentSelectionIndex >= RadioButtons.Num()) { return nullptr; }
	
	return RadioButtons[CurrentSelectionIndex];
}

/*====================================================================================================================================
                                                         SELECTION LOGIC
======================================================================================================================================*/

void UGenericRadioButtonGroup::OnRadioButtonToggled(bool bSelected)
{
	// Reason: Only process selection events, ignore deselection
	if (!bSelected) { return; }
	
	// Reason: Find which button triggered the event
	for (int32 i = 0; i < RadioButtons.Num(); ++i)
	{
		if (RadioButtons[i] && RadioButtons[i]->IsSelected())
		{
			LockSelection(i);
			break;
		} // End if (Button selected)
	} // End for (RadioButtons)
}

void UGenericRadioButtonGroup::LockSelection(int32 Index)
{
	// Reason: No-op if already selected
	if (CurrentSelectionIndex == Index) { return; }
	
	// Reason: Deselect all other buttons
	for (int32 i = 0; i < RadioButtons.Num(); ++i)
	{
		if (RadioButtons[i])
		{
			RadioButtons[i]->SetSelected(i == Index);
		} // End if (Button valid)
	} // End for (RadioButtons)
	
	CurrentSelectionIndex = Index;
	OnSelectionChanged.Broadcast(CurrentSelectionIndex);
}
