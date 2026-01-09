#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "EntryWidget.generated.h"

/*====================================================================================================================================
                                                         UNIVERSAL ENTRY WIDGET
======================================================================================================================================*/

/** Universal entry widget for all static data types */
UCLASS()
class GRIT_API UEntryWidget : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    //------------------------------------------------------------------------------
    // widget bindings - optional components
    //------------------------------------------------------------------------------
    
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* LabelText; // [UTextBlock*] - Optional text display

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* IconImage; // [UImage*] - Optional image display

protected:
    virtual void NativeConstruct() override;
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

private:
    /** Sync display with entry data */
    void SyncDisplay(class UEntryData* Data);

    /** Apply theme styling to entry */
    void ApplyThemeStyling();
};