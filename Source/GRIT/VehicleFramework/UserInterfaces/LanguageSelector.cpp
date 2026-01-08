#include "LanguageSelector.h"
#include "../../GameContext/SessionAdapter.h"
#include "../../GameContext/UserPreferences.h"

/*====================================================================================================================================
                                                         INITIALIZATION
======================================================================================================================================*/

ULanguageSelector::ULanguageSelector(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Configure for language selection behavior
    bAutoCloseOnSelection = true;
    bUpdateTriggerText = true;
    TriggerText = FText::FromString("Language");
}

void ULanguageSelector::NativeConstruct()
{
    // Get language manager instance
    LanguageManager = ULanguageManager::GetInstance();

    // Clear any default options from parent to prevent duplicates
    DefaultRadioOptions.Empty();

    // Populate language options before parent construct
    PopulateLanguageOptions();

    // Bind to our own selection events BEFORE parent construct
    OnSelectionChanged.AddDynamic(this, &ULanguageSelector::OnLanguageSelectionChanged);

    // Bind to global language changes if syncing enabled
    if (bSyncWithGlobalManager && LanguageManager)
    {
        LanguageManager->OnLanguageChanged.AddDynamic(this, &ULanguageSelector::OnGlobalLanguageChanged);
    }

    // Set default selection index based on current global language if syncing
    if (bSyncWithGlobalManager && LanguageManager)
    {
        ELanguage CurrentGlobalLanguage = LanguageManager->GetCurrentLanguage();
        int32 GlobalLanguageIndex = GetIndexForLanguage(CurrentGlobalLanguage);
        if (GlobalLanguageIndex >= 0)
        {
            DefaultSelectionIndex = GlobalLanguageIndex;
            UE_LOG(LogTemp, Warning, TEXT("LanguageSelector: Set DefaultSelectionIndex to %d for global language %s"), 
                   DefaultSelectionIndex, *ULanguageManager::GetLanguageDisplayName(CurrentGlobalLanguage).ToString());
        }
    }

    // Call parent construct (this will handle DefaultSelectionIndex)
    Super::NativeConstruct();

    UE_LOG(LogTemp, Warning, TEXT("LanguageSelector: Constructed with %d languages, DefaultSelectionIndex=%d"), 
           ActiveLanguages.Num(), DefaultSelectionIndex);
}

void ULanguageSelector::NativeDestruct()
{
    // Unbind from global language manager
    if (LanguageManager)
    {
        LanguageManager->OnLanguageChanged.RemoveDynamic(this, &ULanguageSelector::OnGlobalLanguageChanged);
    }

    Super::NativeDestruct();
}

/*====================================================================================================================================
                                                         LANGUAGE MANAGEMENT
======================================================================================================================================*/

void ULanguageSelector::SetCurrentLanguage(ELanguage Language)
{
    int32 LanguageIndex = GetIndexForLanguage(Language);
    if (LanguageIndex >= 0)
    {
        SetSelectedIndex(LanguageIndex);
        UE_LOG(LogTemp, Warning, TEXT("LanguageSelector: Set language to %s (index %d)"), 
               *ULanguageManager::GetLanguageDisplayName(Language).ToString(), LanguageIndex);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("LanguageSelector: Language %s not found in active languages"), 
               *ULanguageManager::GetLanguageDisplayName(Language).ToString());
    }
}

ELanguage ULanguageSelector::GetCurrentLanguage() const
{
    return GetLanguageAtIndex(GetSelectedIndex());
}

void ULanguageSelector::RefreshLanguageOptions()
{
    UE_LOG(LogTemp, Warning, TEXT("LanguageSelector: Refreshing language options"));

    // Remember current selection
    ELanguage CurrentLang = GetCurrentLanguage();

    // Clear and repopulate
    ClearRadioOptions();
    PopulateLanguageOptions();

    // Restore selection if possible
    if (CurrentLang != ELanguage::English || GetIndexForLanguage(CurrentLang) >= 0)
    {
        SetCurrentLanguage(CurrentLang);
    }
}

/*====================================================================================================================================
                                                         INTERNAL METHODS
======================================================================================================================================*/

void ULanguageSelector::PopulateLanguageOptions()
{
    // Determine which languages to include
    if (IncludedLanguages.Num() > 0)
    {
        ActiveLanguages = IncludedLanguages;
    }
    else
    {
        ActiveLanguages = ULanguageManager::GetAllLanguages();
    }

    UE_LOG(LogTemp, Warning, TEXT("LanguageSelector: Populating %d language options"), ActiveLanguages.Num());

    // Create radio button for each language
    for (ELanguage Language : ActiveLanguages)
    {
        FRadioButtonConfig Config;
        Config.Label = ULanguageManager::GetLanguageDisplayName(Language);
        Config.Data = FString::Printf(TEXT("%d"), (int32)Language);

        AddRadioOption(Config);
    }
}

void ULanguageSelector::SyncWithLanguageManager()
{
    if (!LanguageManager) return;

    ELanguage GlobalLanguage = LanguageManager->GetCurrentLanguage();
    ELanguage CurrentLanguage = GetCurrentLanguage();

    if (GlobalLanguage != CurrentLanguage)
    {
        UE_LOG(LogTemp, Warning, TEXT("LanguageSelector: Syncing with global language manager - %s"), 
               *ULanguageManager::GetLanguageDisplayName(GlobalLanguage).ToString());
        SetCurrentLanguage(GlobalLanguage);
    }
}

ELanguage ULanguageSelector::GetLanguageAtIndex(int32 Index) const
{
    if (Index < 0 || Index >= ActiveLanguages.Num())
    {
        return ELanguage::English; // Default fallback
    }
    return ActiveLanguages[Index];
}

int32 ULanguageSelector::GetIndexForLanguage(ELanguage Language) const
{
    return ActiveLanguages.Find(Language);
}

void ULanguageSelector::OnLanguageSelectionChanged(int32 SelectedIndex, UGenericRadioButton* SelectedButton)
{
    ELanguage SelectedLanguage = GetLanguageAtIndex(SelectedIndex);
    
    UE_LOG(LogTemp, Warning, TEXT("LanguageSelector: Language selection changed to %s (index %d)"), 
           *ULanguageManager::GetLanguageDisplayName(SelectedLanguage).ToString(), SelectedIndex);

    // Update global language manager if enabled
    if (bUpdateGlobalManager && LanguageManager)
    {
        ELanguage CurrentGlobalLanguage = LanguageManager->GetCurrentLanguage();
        if (CurrentGlobalLanguage != SelectedLanguage)
        {
            UE_LOG(LogTemp, Warning, TEXT("LanguageSelector: Updating global language manager from %s to %s"), 
                   *ULanguageManager::GetLanguageDisplayName(CurrentGlobalLanguage).ToString(),
                   *ULanguageManager::GetLanguageDisplayName(SelectedLanguage).ToString());
            LanguageManager->SetCurrentLanguage(SelectedLanguage);
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("LanguageSelector: Global language already set to %s, no update needed"), 
                   *ULanguageManager::GetLanguageDisplayName(SelectedLanguage).ToString());
        }
    }

    // Save to user preferences (we have proper world context here)
    if (UWorld* World = GetWorld())
    {
        if (USessionAdapter* SessionAdapter = Cast<USessionAdapter>(World->GetGameInstance()))
        {
            if (UUserPreferencesManager* PrefsManager = SessionAdapter->GetUserPreferencesManager())
            {
                UE_LOG(LogTemp, Warning, TEXT("LanguageSelector: Saving language preference to user settings..."));
                PrefsManager->SetPreferredLanguage(SelectedLanguage, true);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("LanguageSelector: UserPreferencesManager is null"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("LanguageSelector: SessionAdapter is null or wrong type"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("LanguageSelector: Could not get world context"));
    }
}

void ULanguageSelector::OnGlobalLanguageChanged(ELanguage NewLanguage)
{
    // Only sync if the change came from outside (avoid infinite loops)
    ELanguage CurrentLanguage = GetCurrentLanguage();
    if (NewLanguage != CurrentLanguage)
    {
        UE_LOG(LogTemp, Warning, TEXT("LanguageSelector: Global language changed to %s, syncing selector"), 
               *ULanguageManager::GetLanguageDisplayName(NewLanguage).ToString());
        SetCurrentLanguage(NewLanguage);
    }
}