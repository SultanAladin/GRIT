#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "EntryList.h"
#include "EntryData.h"
#include "AnimUtil.h"
#include "DropList.generated.h"

/*====================================================================================================================================
                                                         ANIMATED DROPDOWN LIST
======================================================================================================================================*/

/** Animated dropdown widget using ListView for content */
UCLASS()
class GRIT_API UDropList : public UUserWidget
{
    GENERATED_BODY()

public:
    UDropList(const FObjectInitializer& ObjectInitializer);

    //------------------------------------------------------------------------------
    // widget bindings
    //------------------------------------------------------------------------------
    
    UPROPERTY(meta = (BindWidget))
    UBorder* HeaderBorder; // [UBorder*] - Interactive header container

    UPROPERTY(meta = (BindWidget))
    UTextBlock* HeaderLabel; // [UTextBlock*] - Header text display

    UPROPERTY(meta = (BindWidget))
    UImage* ChevronIcon; // [UImage*] - Dropdown indicator

    UPROPERTY(meta = (BindWidget))
    UOverlay* ContentOverlay; // [UOverlay*] - Content container

    UPROPERTY(meta = (BindWidget))
    USizeBox* ContentSizeBox; // [USizeBox*] - Animated size container

    UPROPERTY(meta = (BindWidget))
    UBorder* ContentBorder; // [UBorder*] - Content background

    UPROPERTY(meta = (BindWidget))
    UEntryList* ContentList; // [UEntryList*] - List view for entries

    //------------------------------------------------------------------------------
    // configuration
    //------------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropList")
    FText HeaderText; // [FText] - Header display text

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropList|Content")
    TArray<FSimpleEntryConfig> EntryConfigs; // [TArray] - Auto-populate entry data

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropList|Animation")
    float AnimDuration; // [s] - Animation duration

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropList|Animation")
    EMotionCurve AnimCurve; // [Enum] - Animation curve type

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropList|Animation")
    bool bAnimChevron; // [bool] - Rotate chevron on expand

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropList|Animation")
    float ChevronAnimDuration; // [s] - Chevron rotation duration

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropList|Appearance")
    float MaxContentHeight; // [px] - Maximum content height

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropList|Appearance")
    float ItemHeight; // [px] - Estimated height per item

    //------------------------------------------------------------------------------
    // runtime operations
    //------------------------------------------------------------------------------
    
    /** Toggle dropdown state */
    UFUNCTION(BlueprintCallable, Category = "DropList")
    void Toggle();

    /** Set dropdown state explicitly */
    UFUNCTION(BlueprintCallable, Category = "DropList")
    void SetExpanded(bool bExpanded);

    /** Check if dropdown is expanded */
    UFUNCTION(BlueprintPure, Category = "DropList")
    bool IsExpanded() const { return bIsExpanded; }

    /** Add entry to list */
    UFUNCTION(BlueprintCallable, Category = "DropList")
    void AddEntry(UEntryData* Data);

    /** Clear all entries */
    UFUNCTION(BlueprintCallable, Category = "DropList")
    void ClearEntries();

    /** Get entry count */
    UFUNCTION(BlueprintPure, Category = "DropList")
    int32 GetEntryCount() const;

    /** Build list from EntryConfigs array */
    UFUNCTION(BlueprintCallable, Category = "DropList")
    void BuildFromConfigs();

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

    //------------------------------------------------------------------------------
    // animation state - protected for derived classes
    //------------------------------------------------------------------------------
    
    bool bIsExpanded; // [bool] - Current expansion state
    bool bIsAnimating; // [bool] - Active animation flag
    bool bIsPressed; // [bool] - Header press state
    bool bHasBuilt; // [bool] - Config auto-build flag
    
    float AnimTime; // [s] - Current animation time
    float StartHeight; // [px] - Animation start height
    float TargetHeight; // [px] - Animation target height
    
    float ChevronAnimTime; // [s] - Chevron animation time
    float StartChevronAngle; // [deg] - Chevron start angle
    float TargetChevronAngle; // [deg] - Chevron target angle
    bool bChevronAnimating; // [bool] - Chevron animation flag
    
    FLinearColor BaseHeaderColor; // [RGBA] - Cached base header color
    FLinearColor HoverHeaderColor; // [RGBA] - Cached hover header color
    
    FTimerHandle AnimTimer; // [Handle] - Animation timer handle
    FTimerHandle ChevronAnimTimer; // [Handle] - Chevron animation timer

    //------------------------------------------------------------------------------
    // internal operations - protected for derived classes
    //------------------------------------------------------------------------------
    
    /** Initialize theme styling */
    void InitTheme();

    /** Configure content overlay slot for width matching */
    void ConfigureContentSlot();

    /** Apply header hover state */
    void ApplyHeaderHover();

    /** Restore header default state */
    void RestoreHeaderDefault();

    /** Start expand animation */
    virtual void StartExpand();

    /** Start collapse animation */
    virtual void StartCollapse();

    /** Tick animation update */
    virtual void TickAnim();

    /** Tick chevron animation */
    void TickChevronAnim();

    /** Compute target content height */
    float ComputeTargetHeight() const;

    /** Animate chevron to angle */
    void AnimateChevronTo(float TargetAngle);

    /** Set chevron angle immediately */
    void SetChevronAngle(float Angle);
};
