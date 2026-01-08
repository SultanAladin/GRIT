//FilterEntry.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "FilterEntry.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFilterRemoved, UFilterEntry*, Entry);

/*====================================================================================================================================
                                                         FILTER ENTRY (Chip/Tag)
======================================================================================================================================*/

UCLASS(Blueprintable, BlueprintType)
class GRIT_API UFilterEntry : public UUserWidget
{
    GENERATED_BODY()

public:
    UFilterEntry(const FObjectInitializer& ObjectInitializer);

    /** Initialize filter with data */
    UFUNCTION(BlueprintCallable, Category = "Filter")
    void InitFilter(const FString& FilterData, int32 FilterOriginalIndex);

    /** Get filter data */
    UFUNCTION(BlueprintPure, Category = "Filter")
    FString GetFilterData() const { return DataPayload; }

    /** Get original index */
    UFUNCTION(BlueprintPure, Category = "Filter")
    int32 GetOriginalIndex() const { return OriginalIndex; }

    /** Play pop-in animation */
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void PlayPopInAnimation();

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnFilterRemoved OnFilterRemoved;

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnFilterRemovedBP();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Widget components
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UBorder* ChipBorder;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* FilterLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* RemoveButton;

    // Styling
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor ChipColor = FLinearColor(1.0f, 0.194658f, 0.041635f, 1.0f);  // Orange

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor TextColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    int32 FontSize = 14;

    // Animation config
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float PopInDuration = 0.2f;  // [s]

private:
    FString DataPayload;
    int32 OriginalIndex = -1;

    // Animation state
    FTimerHandle AnimTimer;
    float ElapsedTime = 0.0f;
    bool bIsAnimating = false;

    UFUNCTION()
    void OnRemoveButtonClicked();

    void AnimatePopIn();
    void UpdatePopInTransform(float Alpha);
};
