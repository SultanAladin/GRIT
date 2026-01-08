#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/HorizontalBox.h"
#include "Card.h"
#include "CardInfo.h"
#include "CardStream.generated.h"

UCLASS()
class GRIT_API UCardStream : public UUserWidget
{
    GENERATED_BODY()

public:
    // Array of card data that can be populated from outside
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FCardInfo> CardDataArray;

    UFUNCTION(BlueprintCallable)
    void CreateCards();

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    UHorizontalBox* CardContainer;

    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UCard> CardWidgetClass;

private:
    UPROPERTY()
    TArray<UCard*> CardWidgets;

    UCard* CurrentActiveCard = nullptr;

    UFUNCTION()
    void OnCardHovered(UCard* HoveredCard);

    void SetActiveCard(UCard* NewActiveCard);
};