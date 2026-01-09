#pragma once

#include "CoreMinimal.h"
#include "Components/ListView.h"
#include "EntryData.h"
#include "SimpleList.generated.h"

/*====================================================================================================================================
                                                         AUTO-POPULATING LIST VIEW
======================================================================================================================================*/

/** Auto-populating list view with config array */
UCLASS()
class GRIT_API USimpleList : public UListView
{
    GENERATED_BODY()

public:
    USimpleList(const FObjectInitializer& ObjectInitializer);

    //------------------------------------------------------------------------------
    // blueprint configuration array
    //------------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "List")
    TArray<FSimpleEntryConfig> EntryConfigs; // [TArray] - Auto-populate source data

    //------------------------------------------------------------------------------
    // runtime list operations
    //------------------------------------------------------------------------------
    
    /** Inject item into list */
    void InjectItem(UObject* Item);

    /** Eject item from list */
    void EjectItem(UObject* Item);

    /** Purge all items */
    void PurgeAll();

    /** Retrieve item count */
    int32 RetrieveItemCount() const;

    /** Build list from EntryConfigs array */
    UFUNCTION(BlueprintCallable, Category = "List")
    void BuildFromConfigs();

protected:
    virtual void SynchronizeProperties() override;

private:
    bool bHasBuilt; // [bool] - Tracks initial build state
};