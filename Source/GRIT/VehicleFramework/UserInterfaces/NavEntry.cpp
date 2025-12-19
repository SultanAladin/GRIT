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
    UE_LOG(LogTemp, Log, TEXT("[NavEntry] ToggleSelection(%d) on Index=%d - WAS bIsSelected=%d"), bState, EntryIndex, bIsSelected);
    bIsSelected = bState;
    SyncChrome();
    UE_LOG(LogTemp, Log, TEXT("[NavEntry] ToggleSelection DONE - Index=%d, bIsSelected=%d"), EntryIndex, bIsSelected);
} // End if (Selection Toggle)

void UNavEntry::SyncChrome()
{
    if (!EntryLabel)
    {
        UE_LOG(LogTemp, Error, TEXT("[NavEntry] SyncChrome - EntryLabel is NULL! Index=%d"), EntryIndex);
        return;
    }

    // Reason: Selection (Orange) takes precedence over Hover (White)
    FLinearColor TargetColor = bIsSelected ? ActiveTextColor : (bIsHovered ? FLinearColor::White : TextColor); // [RGBA]

    UE_LOG(LogTemp, Log, TEXT("[NavEntry] SyncChrome Index=%d - bIsSelected=%d, bIsHovered=%d -> Color=(%.2f,%.2f,%.2f)"),
        EntryIndex, bIsSelected, bIsHovered, TargetColor.R, TargetColor.G, TargetColor.B);

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
    UE_LOG(LogTemp, Log, TEXT("[NavEntry] NativeOnMouseEnter - Index=%d"), EntryIndex);
    bIsHovered = true;
    SyncChrome();

    OnEntryHovered.Broadcast(this);
}

void UNavEntry::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
    Super::NativeOnMouseLeave(MouseEvent);
    UE_LOG(LogTemp, Log, TEXT("[NavEntry] NativeOnMouseLeave - Index=%d, bIsSelected=%d"), EntryIndex, bIsSelected);
    bIsHovered = false;
    bIsPressed = false;
    SyncChrome();

    OnEntryUnhovered.Broadcast(this);
}

FReply UNavEntry::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        UE_LOG(LogTemp, Log, TEXT("[NavEntry] NativeOnMouseButtonDown - Index=%d"), EntryIndex);
        bIsPressed = true;
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UNavEntry::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsPressed)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavEntry] NativeOnMouseButtonUp - CLICK! Index=%d, Broadcasting OnEntryClicked"), EntryIndex);
        bIsPressed = false;
        OnEntryClicked.Broadcast(this);
        OnEntryClickedBP();
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}