#include "CardStream.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

void UCardStream::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Auto-create cards when widget is constructed
    CreateCards();
}

void UCardStream::CreateCards()
{
    if (!CardContainer || !CardWidgetClass)
        return;

    // Clear existing cards
    CardContainer->ClearChildren();
    CardWidgets.Empty();
    CurrentActiveCard = nullptr;

    // Create cards from data array
    for (int32 i = 0; i < CardDataArray.Num(); i++)
    {
        UCard* NewCard = CreateWidget<UCard>(this, CardWidgetClass);
        if (NewCard)
        {
            // Setup card with data
            NewCard->SetupCard(CardDataArray[i]);
            
            // Bind hover event
            NewCard->OnCardHovered.AddDynamic(this, &UCardStream::OnCardHovered);
            
            // Add to container
            UHorizontalBoxSlot* CardSlot = CardContainer->AddChildToHorizontalBox(NewCard);
            if (CardSlot)
            {
                CardSlot->SetPadding(FMargin(2.0f));
                CardSlot->SetHorizontalAlignment(HAlign_Fill);
                CardSlot->SetVerticalAlignment(VAlign_Fill);
            }

            CardWidgets.Add(NewCard);

            // Set first card as active
            if (i == 0)
            {
                SetActiveCard(NewCard);
            }
        }
    }
}

void UCardStream::OnCardHovered(UCard* HoveredCard)
{
    if (HoveredCard && HoveredCard != CurrentActiveCard)
    {
        SetActiveCard(HoveredCard);
    }
}

void UCardStream::SetActiveCard(UCard* NewActiveCard)
{
    if (!NewActiveCard)
        return;

    // Deactivate all cards first
    for (UCard* Card : CardWidgets)
    {
        if (Card)
        {
            Card->SetCardActive(false);
        }
    }

    // Activate the selected card
    NewActiveCard->SetCardActive(true);
    CurrentActiveCard = NewActiveCard;
}