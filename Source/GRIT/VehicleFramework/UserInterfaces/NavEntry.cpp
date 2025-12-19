//NavEntry.cpp
#include "NavEntry.h"

UNavEntry::UNavEntry(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , EntryRoot(nullptr)
    , EntryLabel(nullptr)
    , EntryIndex(-1)
    , bIsSelected(false)
    , bIsHovered(false)
    , bIsPressed(false)
{
}

void UNavEntry::NativeConstruct()
{
    Super::NativeConstruct();

    if (EntryLabel)
    {
        EntryLabel->SetColorAndOpacity(FSlateColor(TextColor));

        FSlateFontInfo FontInfo = EntryLabel->GetFont();
        FontInfo.Size = FontSize;
        EntryLabel->SetFont(FontInfo);
    }

    SyncChrome();
}

void UNavEntry::InitEntry(const FText& Label, const FString& Data, int32 Index)
{
    EntryData = Data;
    EntryIndex = Index;

    if (EntryLabel)
    {
        EntryLabel->SetText(Label);
    }
}

void UNavEntry::ToggleSelection(bool bState)
{
    bIsSelected = bState;
    SyncChrome();
} // End if (Selection Toggle)

void UNavEntry::SyncChrome()
{
    if (!EntryLabel) { return; }

    // Reason: Selection (Orange) takes precedence over Hover (White)
    FLinearColor TargetColor = bIsSelected ? ActiveTextColor : (bIsHovered ? FLinearColor::White : TextColor); // [RGBA]
    EntryLabel->SetColorAndOpacity(FSlateColor(TargetColor));

    if (EntryRoot)
    {
        float Alpha = (bIsSelected || bIsHovered) ? 0.05f : 0.0f; // [-]
        FLinearColor BgColor = HoverBackgroundColor;
        BgColor.A = Alpha;
        EntryRoot->SetBrushColor(BgColor);
    } // End if (EntryRoot exists)
} // End if (SyncChrome)

//------------------------------------------------------------------------------
//                                    INPUT HANDLING
//------------------------------------------------------------------------------

void UNavEntry::NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    Super::NativeOnMouseEnter(MyGeometry, MouseEvent);
    bIsHovered = true;
    SyncChrome();

    OnEntryHovered.Broadcast(this);
}

void UNavEntry::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
    Super::NativeOnMouseLeave(MouseEvent);
    bIsHovered = false;
    bIsPressed = false;
    SyncChrome();

    OnEntryUnhovered.Broadcast(this);
}

FReply UNavEntry::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        bIsPressed = true;
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UNavEntry::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsPressed)
    {
        bIsPressed = false;
        OnEntryClicked.Broadcast(this);
        OnEntryClickedBP();
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}