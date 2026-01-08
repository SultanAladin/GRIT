//CarouselWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Overlay.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "CarouselWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCarouselPageChanged, int32, NewPageIndex);

/*====================================================================================================================================
                                                    CAROUSEL WIDGET
======================================================================================================================================*/

UCLASS(BlueprintType, Blueprintable)
class GRIT_API UCarouselWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UCarouselWidget(const FObjectInitializer& ObjectInitializer);

    //------------------------------------------------------------------------------
    // Page management
    //------------------------------------------------------------------------------

    /** Add a page widget to the carousel */
    UFUNCTION(BlueprintCallable, Category = "Carousel")
    void AddPage(UUserWidget* PageWidget);

    /** Remove page at index */
    UFUNCTION(BlueprintCallable, Category = "Carousel")
    void RemovePage(int32 PageIndex);

    /** Clear all pages */
    UFUNCTION(BlueprintCallable, Category = "Carousel")
    void ClearPages();

    //------------------------------------------------------------------------------
    // Navigation
    //------------------------------------------------------------------------------

    /** Navigate to specific page index */
    UFUNCTION(BlueprintCallable, Category = "Carousel")
    void NavigateToPage(int32 PageIndex);

    /** Navigate to previous page */
    UFUNCTION(BlueprintCallable, Category = "Carousel")
    void NavigateLeft();

    /** Navigate to next page */
    UFUNCTION(BlueprintCallable, Category = "Carousel")
    void NavigateRight();

    //------------------------------------------------------------------------------
    // Getters
    //------------------------------------------------------------------------------

    /** Get current page index */
    UFUNCTION(BlueprintCallable, Category = "Carousel")
    int32 GetCurrentPageIndex() const { return CurrentPageIndex; }

    /** Get total page count */
    UFUNCTION(BlueprintCallable, Category = "Carousel")
    int32 GetPageCount() const { return PageWidgets.Num(); }

    /** Get current page widget */
    UFUNCTION(BlueprintCallable, Category = "Carousel")
    UUserWidget* GetCurrentPage() const;

    //------------------------------------------------------------------------------
    // Events
    //------------------------------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnCarouselPageChanged OnPageChanged;

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnPageChangedBP(int32 NewPageIndex);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    //------------------------------------------------------------------------------
    // Widget bindings
    //------------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UOverlay* RootOverlay;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UHorizontalBox* PageTrack;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* LeftButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* RightButton;

    //------------------------------------------------------------------------------
    // Configuration
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carousel")
    int32 InitialPageIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float TransitionDuration = 0.4f;  // [s] - Animation duration

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float PageWidth = 1920.0f;  // [px] - Width of each page

private:
    //------------------------------------------------------------------------------
    // Runtime state
    //------------------------------------------------------------------------------

    int32 CurrentPageIndex = 0;
    int32 TargetPageIndex = 0;
    float CurrentOffset = 0.0f;  // [px] - Current X translation
    float TargetOffset = 0.0f;   // [px] - Target X translation
    bool bIsAnimating = false;

    UPROPERTY()
    TArray<UUserWidget*> PageWidgets;

    //------------------------------------------------------------------------------
    // Internal pipeline
    //------------------------------------------------------------------------------

    void BindButtonEvents();
    void UpdateButtonVisibility();
    void UpdatePageTrackPosition();

    UFUNCTION()
    void HandleLeftButtonClicked();

    UFUNCTION()
    void HandleRightButtonClicked();
};
