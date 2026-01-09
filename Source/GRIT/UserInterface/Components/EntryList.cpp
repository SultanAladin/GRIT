#include "EntryList.h"
#include "EntryData.h"
#include "EntryWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogEntryList, Log, All);

void UEntryList::InjectEntry(UEntryData* Data)
{
    if (Data) // Reason: Valid data check
    {
        UE_LOG(LogEntryList, Log, TEXT("InjectEntry: Adding entry ID=%d Type=%s"), Data->EntryID, *Data->RetrieveEntryType().ToString());
        AddItem(Data);
    } // End if (Data check)
    else
    {
        UE_LOG(LogEntryList, Warning, TEXT("InjectEntry: Attempted to add null entry"));
    }
}

void UEntryList::PurgeAll()
{
    UE_LOG(LogEntryList, Log, TEXT("PurgeAll: Clearing %d entries"), GetNumItems());
    ClearListItems();
}