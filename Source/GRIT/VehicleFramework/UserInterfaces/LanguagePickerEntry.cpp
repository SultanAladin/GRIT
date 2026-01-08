#include "LanguagePickerEntry.h"

//------------------------------------------------------------------------------
//                                  LIFECYCLE
//------------------------------------------------------------------------------

void ULanguagePickerEntry::NativeConstruct()
{
    Super::NativeConstruct();

    // Reason: Bind button events
    if (EntryButton)
    {
        EntryButton->OnClicked.AddDynamic(this, &ULanguagePickerEntry::ProcessClicked);
        EntryButton->OnHovered.AddDynamic(this, &ULanguagePickerEntry::ProcessHovered);
        EntryButton->OnUnhovered.AddDynamic(this, &ULanguagePickerEntry::ProcessUnhovered);
    } // End if (EntryButton exists)

    RefreshVisualState();
}

//------------------------------------------------------------------------------
//                               PUBLIC API
//------------------------------------------------------------------------------

void ULanguagePickerEntry::SetLanguageData(const FString& DisplayName, const FString& Code)
{
    LanguageCode = Code;
    
    // Reason: Update display text
    if (LanguageText)
    {
        LanguageText->SetText(FText::FromString(DisplayName));
    } // End if (LanguageText exists)
}

void ULanguagePickerEntry::SetSelected(bool bIsSelected_In)
{
    bIsSelected = bIsSelected_In;
    RefreshVisualState();
}

//------------------------------------------------------------------------------
//                             EVENT HANDLERS
//------------------------------------------------------------------------------

void ULanguagePickerEntry::ProcessClicked()
{
    // Reason: Broadcast language selection
    OnLanguageSelected.Broadcast(LanguageCode);
}

void ULanguagePickerEntry::ProcessHovered()
{
    // Reason: Apply hover color if not selected
    if (!bIsSelected && LanguageText)
    {
        LanguageText->SetColorAndOpacity(HoverColor);
    } // End if (not selected check)
}

void ULanguagePickerEntry::ProcessUnhovered()
{
    // Reason: Restore default state
    RefreshVisualState();
}

void ULanguagePickerEntry::RefreshVisualState()
{
    // Reason: Update text color based on selection state
    if (LanguageText)
    {
        FLinearColor TargetColor = bIsSelected ? SelectedColor : DefaultColor;  // [RGBA]
        LanguageText->SetColorAndOpacity(TargetColor);
    } // End if (LanguageText exists)
}
