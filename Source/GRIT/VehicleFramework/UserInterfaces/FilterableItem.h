//FilterableItem.h
#pragma once

#include "CoreMinimal.h"
#include "DropdownBase.h"  // Get EDropdownEasingType enum from here
#include "DropdownItemBase.h"
#include "FilterableItem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFilterItemCollapseComplete, UFilterableItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFilterItemExpandComplete, UFilterableItem*, Item);

/*====================================================================================================================================
                                                         FILTERABLE ITEM
======================================================================================================================================*/

UCLASS(Blueprintable, BlueprintType)
class GRIT_API UFilterableItem : public UDropdownItemBase
{
    GENERATED_BODY()

public:
    UFilterableItem(const FObjectInitializer& ObjectInitializer);

    /** Start collapse animation (when selected) */
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void StartCollapseAnimation();

    /** Start expand animation (when restored) */
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void StartExpandAnimation();

    /** Get original index */
    UFUNCTION(BlueprintPure, Category = "Filter")
    int32 GetOriginalIndex() const { return OriginalIndex; }

    /** Set original index */
    UFUNCTION(BlueprintCallable, Category = "Filter")
    void SetOriginalIndex(int32 Index) { OriginalIndex = Index; }

    /** Get default item data */
    UFUNCTION(BlueprintPure, Category = "Item Data")
    FString GetDefaultItemData() const { return DefaultItemData; }

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnFilterItemCollapseComplete OnCollapseComplete;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnFilterItemExpandComplete OnExpandComplete;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Widget bindings (bind in Blueprint)
    UPROPERTY(meta = (BindWidgetOptional))
    class UBorder* ItemBorder;

    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* AddFilterButton;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* ButtonLabel;

    // Button click handler
    UFUNCTION()
    void OnAddFilterButtonClicked();

    // Item data - set this in Blueprint class defaults
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item Data")
    FString DefaultItemData = TEXT("");

    // Animation config
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float AnimationDuration = 0.25f;  // [s]

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    EDropdownEasingType EasingType = EDropdownEasingType::EaseOut;

private:
    int32 OriginalIndex = -1;

    // Animation state
    FTimerHandle AnimTimer;
    float ElapsedTime = 0.0f;
    bool bIsAnimating = false;
    bool bIsCollapsing = false;

    void StopAnimation();
    void AnimateTick();
    void UpdateTransform(float Alpha);

    // Easing functions - use enum from DropdownBase.h
    float ApplyEasing(float t) const;
    float EaseInOutQuad(float t) const;
    float EaseInQuad(float t) const;
    float EaseOutQuad(float t) const;
    float EaseInOutCubic(float t) const;
};
