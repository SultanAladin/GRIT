#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "ButtonEntryWidget.generated.h"

/*====================================================================================================================================
                                                         HIGH-PERFORMANCE BUTTON WIDGET
======================================================================================================================================*/

/** High-performance button entry using UBorder (~0.1ms overhead) */
UCLASS()
class GRIT_API UButtonEntryWidget : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    //------------------------------------------------------------------------------
    // widget bindings - optional components
    //------------------------------------------------------------------------------
    
    UPROPERTY(meta = (BindWidgetOptional))
    UBorder* ButtonBorder; // [UBorder*] - Required input container

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* LabelText; // [UTextBlock*] - Optional text display

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* IconImage; // [UImage*] - Optional image display

protected:
    virtual void NativeConstruct() override;
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
    //------------------------------------------------------------------------------
    // internal state management
    //------------------------------------------------------------------------------
    
    /** Sync display with entry data */
    void SyncDisplay(class UEntryData* Data);

    /** Apply theme styling to button */
    void ApplyThemeStyling();

    /** Apply hover state visual */
    void ApplyHoverState();

    /** Restore default state visual */
    void RestoreDefaultState();

    UPROPERTY()
    class UButtonEntryBase* CurrentEntry; // [UButtonEntryBase*] - Active entry reference

    bool bIsPressed; // [bool] - Tracks active press state
    
    FLinearColor BaseColor; // [RGBA] - Cached base color
    FLinearColor HoverColor; // [RGBA] - Cached hover color
};