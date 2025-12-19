//NavBarBase.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Overlay.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/Image.h"
#include "NavEntry.h"
#include "NavBarBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNavSelectionChanged, int32, SelectedIndex, const FString&, SelectedData);

/*====================================================================================================================================
                                                         NAVBAR BASE (Sliding Indicator)
======================================================================================================================================*/

UCLASS(Blueprintable, BlueprintType)
class GRIT_API UNavBarBase : public UUserWidget
{
    GENERATED_BODY()

public:
    UNavBarBase(const FObjectInitializer& ObjectInitializer);

    /** Add entry from Blueprint class */
    UFUNCTION(BlueprintCallable, Category = "NavBar")
    void AddNavEntry(TSubclassOf<UNavEntry> EntryClass);

    /** Set selected entry by index */
    UFUNCTION(BlueprintCallable, Category = "NavBar")
    void SetSelectedIndex(int32 Index);

    /** Get selected index */
    UFUNCTION(BlueprintPure, Category = "NavBar")
    int32 GetSelectedIndex() const { return SelectedIndex; }

    /** Clear all entries */
    UFUNCTION(BlueprintCallable, Category = "NavBar")
    void ClearEntries();

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnNavSelectionChanged OnSelectionChanged;

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnSelectionChangedBP(int32 Index, const FString& Data);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeOnMouseLeave(const FPointerEvent& MouseEvent) override;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UOverlay* RootOverlay;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UHorizontalBox* NavContainer_H;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UVerticalBox* NavContainer_V;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage* SlidingIndicator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Items")
    TArray<TSubclassOf<UNavEntry>> DefaultEntries;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Selection")
    int32 DefaultSelectedIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    FLinearColor IndicatorColor = FLinearColor(1.0f, 0.194658f, 0.041635f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Appearance")
    float IndicatorThickness = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Animation")
    float IndicatorAnimDuration = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Spacing")
    FMargin EntryPadding = FMargin(0.0f, 0.0f);

private:
    bool bIsHorizontal = true;
    int32 SelectedIndex = -1;
    int32 HoveredIndex = -1; // Track which entry is currently hovered [-]
    TArray<UNavEntry*> NavEntries;
    FTimerHandle IndicatorAnimTimer;
    FTimerHandle ReturnToSelectedTimer; // Delayed return timer
    FTimerHandle InitRetryTimer; // Geometry readiness retry timer
    float IndicatorAnimElapsed = 0.0f;
    bool bSizesCalculated = false;
    bool bInitComplete = false; // Full initialization status [-]
    int32 InitRetryCount = 0; // Geometry check attempts [-]
    float CachedItemSize = 0.0f; // Cached item width or height [px]
    float IndicatorStartPos = 0.0f;
    float IndicatorTargetPos = 0.0f;
    bool bIsAnimatingIndicator = false;

    void DetermineOrientation();
    void ConfigureSlotProperties();
    void RecalcEntrySizes();
    void UpdateIndicatorSizeAndPosition();
    void AnimateIndicatorToIndex(int32 Index);
    void TickIndicatorAnimation();
    float CalculateIndicatorPosition(int32 Index);
    void UpdateIndicatorTransform(float Position);
    void TryCompleteInit(); // Attempt full setup with geometry check

    UFUNCTION()
    void OnEntryClicked(UNavEntry* Entry);

    UFUNCTION()
    void AnimateIndicatorToEntry(UNavEntry* Entry);

    UFUNCTION()
    void OnEntryUnhovered(UNavEntry* Entry);

    void ReturnIndicatorToSelected();
};
