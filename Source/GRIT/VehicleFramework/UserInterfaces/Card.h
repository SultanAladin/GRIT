#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "CardInfo.h"
#include "Card.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardHovered, class UCard*, HoveredCard);

UCLASS()
class GRIT_API UCard : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnCardHovered OnCardHovered;

    UFUNCTION(BlueprintCallable)
    void SetupCard(const FCardInfo& CardInfo);

    UFUNCTION(BlueprintCallable)
    void SetCardActive(bool bActive);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(meta = (BindWidget))
    UButton* CardButton;

    UPROPERTY(meta = (BindWidget))
    UImage* CardImage;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TitleText;

    UPROPERTY(meta = (BindWidget))
    USizeBox* CardSizeBox;

    UFUNCTION()
    void OnCardButtonHovered();

    UFUNCTION()
    void OnCardButtonClicked();

private:
    bool bIsActive = false;
    FCardInfo CurrentCardInfo;
    
    // Animation properties
    float CurrentWidth = 120.0f;
    float TargetWidth = 120.0f;
    float AnimationSpeed = 800.0f; // pixels per second
    bool bIsAnimating = false;
};