#include "Card.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Animation/WidgetAnimation.h"

void UCard::NativeConstruct()
{
    Super::NativeConstruct();

    if (CardButton)
    {
        CardButton->OnHovered.AddDynamic(this, &UCard::OnCardButtonHovered);
        CardButton->OnClicked.AddDynamic(this, &UCard::OnCardButtonClicked);
        
        // Set rounded corners style
        FButtonStyle ButtonStyle = CardButton->GetStyle();
        ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::RoundedBox;
        ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::RoundedBox;
        ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::RoundedBox;
        CardButton->SetStyle(ButtonStyle);
    }
}

void UCard::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Smooth width animation
    if (bIsAnimating && CardSizeBox)
    {
        float Difference = TargetWidth - CurrentWidth;
        
        if (FMath::Abs(Difference) > 1.0f)
        {
            float AnimationStep = AnimationSpeed * InDeltaTime;
            if (Difference > 0)
            {
                CurrentWidth = FMath::Min(CurrentWidth + AnimationStep, TargetWidth);
            }
            else
            {
                CurrentWidth = FMath::Max(CurrentWidth - AnimationStep, TargetWidth);
            }
            
            CardSizeBox->SetWidthOverride(CurrentWidth);
        }
        else
        {
            CurrentWidth = TargetWidth;
            CardSizeBox->SetWidthOverride(CurrentWidth);
            bIsAnimating = false;
        }
    }
}

void UCard::SetupCard(const FCardInfo& CardInfo)
{
    CurrentCardInfo = CardInfo;

    if (CardImage && CardInfo.CardImage)
    {
        CardImage->SetBrushFromTexture(CardInfo.CardImage);
    }

    if (TitleText)
    {
        TitleText->SetText(CardInfo.CardTitle);
    }

    // Set initial size (collapsed)
    if (CardSizeBox)
    {
        CardSizeBox->SetWidthOverride(CurrentWidth); // Use CurrentWidth (120.0f)
        CardSizeBox->SetHeightOverride(400.0f);
    }
}

void UCard::SetCardActive(bool bActive)
{
    bIsActive = bActive;

    // Set target width and start animation
    TargetWidth = bActive ? 600.0f : 120.0f; // Expanded vs collapsed
    bIsAnimating = true;

    // Adjust opacity for inactive cards
    SetRenderOpacity(bActive ? 1.0f : 0.6f);
}

void UCard::OnCardButtonHovered()
{
    OnCardHovered.Broadcast(this);
}

void UCard::OnCardButtonClicked()
{
    OnCardHovered.Broadcast(this);
}