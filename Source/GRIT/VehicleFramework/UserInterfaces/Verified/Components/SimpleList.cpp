#include "SimpleList.h"

DEFINE_LOG_CATEGORY_STATIC(LogSimpleList, Log, All);

USimpleList::USimpleList(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , bHasBuilt(false)
{
    UE_LOG(LogSimpleList, Log, TEXT("SimpleList: Constructor called"));
}

void USimpleList::SynchronizeProperties()
{
    Super::SynchronizeProperties();
    
    UE_LOG(LogSimpleList, Log, TEXT("SynchronizeProperties: Called, bHasBuilt=%d, EntryConfigs=%d"), bHasBuilt, EntryConfigs.Num());
    
    if (!bHasBuilt && EntryConfigs.Num() > 0) // Reason: Auto-populate on first sync
    {
        UE_LOG(LogSimpleList, Log, TEXT("SynchronizeProperties: Auto-building from configs"));
        BuildFromConfigs();
        bHasBuilt = true;
    } // End if (First build check)
}

void USimpleList::InjectItem(UObject* Item)
{
    if (Item) // Reason: Valid item check
    {
        UE_LOG(LogSimpleList, Log, TEXT("InjectItem: Adding item %s"), *Item->GetName());
        AddItem(Item);
    } // End if (Item check)
    else
    {
        UE_LOG(LogSimpleList, Warning, TEXT("InjectItem: Attempted to add null item"));
    }
}

void USimpleList::EjectItem(UObject* Item)
{
    if (Item) // Reason: Valid item check
    {
        UE_LOG(LogSimpleList, Log, TEXT("EjectItem: Removing item %s"), *Item->GetName());
        RemoveItem(Item);
    } // End if (Item check)
}

void USimpleList::PurgeAll()
{
    UE_LOG(LogSimpleList, Log, TEXT("PurgeAll: Clearing %d items"), GetNumItems());
    ClearListItems();
}

int32 USimpleList::RetrieveItemCount() const
{
    return GetNumItems();
}

void USimpleList::BuildFromConfigs()
{
    UE_LOG(LogSimpleList, Log, TEXT("BuildFromConfigs: Starting with %d configs"), EntryConfigs.Num());
    
    if (EntryConfigs.Num() == 0) // Reason: No configs to process
    {
        UE_LOG(LogSimpleList, Warning, TEXT("BuildFromConfigs: EntryConfigs array is empty!"));
        return;
    } // End if (Empty configs check)

    PurgeAll();

    //------------------------------------------------------------------------------
    // config iteration and entry construction
    //------------------------------------------------------------------------------
    
    for (int32 i = 0; i < EntryConfigs.Num(); i++) // Reason: Iterate configs
    {
        const FSimpleEntryConfig& Config = EntryConfigs[i];
        
        bool bHasText = !Config.EntryText.IsEmpty();
        bool bHasTexture = Config.EntryTexture != nullptr;
        bool bIsButton = Config.bIsButton;

        UE_LOG(LogSimpleList, Log, TEXT("BuildFromConfigs [%d]: Text='%s' HasText=%d HasTexture=%d IsButton=%d"), i, *Config.EntryText.ToString(), bHasText, bHasTexture, bIsButton);

        if (bIsButton) // Reason: Create button entries
        {
            if (bHasText && bHasTexture) // Reason: Both text and texture button
            {
                UE_LOG(LogSimpleList, Log, TEXT("BuildFromConfigs [%d]: Creating LabeledImageButtonEntry"), i);
                ULabeledImageButtonEntry* Entry = NewObject<ULabeledImageButtonEntry>(this);
                Entry->BootstrapLabeledImageButton(i, Config.EntryText, Config.EntryTexture);
                InjectItem(Entry);
            } // End if (Text and Texture button)
            else if (bHasTexture) // Reason: Texture only button
            {
                UE_LOG(LogSimpleList, Log, TEXT("BuildFromConfigs [%d]: Creating ImageButtonEntry"), i);
                UImageButtonEntry* Entry = NewObject<UImageButtonEntry>(this);
                Entry->BootstrapImageButton(i, Config.EntryTexture);
                InjectItem(Entry);
            } // End if (Texture button)
            else if (bHasText) // Reason: Text only button
            {
                UE_LOG(LogSimpleList, Log, TEXT("BuildFromConfigs [%d]: Creating TextButtonEntry"), i);
                UTextButtonEntry* Entry = NewObject<UTextButtonEntry>(this);
                Entry->BootstrapTextButton(i, Config.EntryText);
                InjectItem(Entry);
            } // End if (Text button)
        } // End if (Button entry)
        else // Reason: Create static entries
        {
            if (bHasText && bHasTexture) // Reason: Both text and texture
            {
                UE_LOG(LogSimpleList, Log, TEXT("BuildFromConfigs [%d]: Creating LabeledImageEntry"), i);
                ULabeledImageEntry* Entry = NewObject<ULabeledImageEntry>(this);
                Entry->BootstrapLabeledImage(i, Config.EntryText, Config.EntryTexture);
                InjectItem(Entry);
            } // End if (Text and Texture)
            else if (bHasTexture) // Reason: Texture only
            {
                UE_LOG(LogSimpleList, Log, TEXT("BuildFromConfigs [%d]: Creating ImageEntry"), i);
                UImageEntry* Entry = NewObject<UImageEntry>(this);
                Entry->BootstrapImage(i, Config.EntryTexture);
                InjectItem(Entry);
            } // End if (Texture only)
            else if (bHasText) // Reason: Text only
            {
                UE_LOG(LogSimpleList, Log, TEXT("BuildFromConfigs [%d]: Creating TextEntry"), i);
                UTextEntry* Entry = NewObject<UTextEntry>(this);
                Entry->BootstrapText(i, Config.EntryText);
                InjectItem(Entry);
            } // End if (Text only)
            else
            {
                UE_LOG(LogSimpleList, Warning, TEXT("BuildFromConfigs [%d]: Skipping - no text or texture"), i);
            }
        } // End else (Static entry)
    } // End for (EntryConfigs loop)
    
    UE_LOG(LogSimpleList, Log, TEXT("BuildFromConfigs: Complete. Final item count: %d"), GetNumItems());
}