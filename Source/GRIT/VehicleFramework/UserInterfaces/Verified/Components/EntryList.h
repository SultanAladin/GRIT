#pragma once

#include "CoreMinimal.h"
#include "Components/ListView.h"
#include "EntryList.generated.h"

/*====================================================================================================================================
                                                         MANUAL ENTRY LIST VIEW
======================================================================================================================================*/

/** Manual list view for procedural entry creation */
UCLASS()
class GRIT_API UEntryList : public UListView
{
    GENERATED_BODY()

public:
    /** Inject entry into list */
    void InjectEntry(class UEntryData* Data);

    /** Purge all entries */
    void PurgeAll();
};