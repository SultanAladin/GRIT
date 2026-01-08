#include "LoginMethodSelector.h"
#include "../../GameContext/SessionAdapter.h"
#include "../../GameContext/UserPreferences.h"

/*====================================================================================================================================
                                                         INITIALIZATION
======================================================================================================================================*/

ULoginMethodSelector::ULoginMethodSelector(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Configure for login method selection behavior
    bAutoCloseOnSelection = true;
    bUpdateTriggerText = true;
    TriggerText = FText::FromString("Login Method");
}

void ULoginMethodSelector::NativeConstruct()
{
    // Get login method manager instance
    LoginMethodManager = ULoginMethodManager::GetInstance();

    // Clear any default options from parent to prevent duplicates
    DefaultRadioOptions.Empty();

    // Populate login method options before parent construct
    PopulateLoginMethodOptions();

    // Bind to our own selection events BEFORE parent construct
    OnSelectionChanged.AddDynamic(this, &ULoginMethodSelector::OnLoginMethodSelectionChanged);

    // Bind to global login method changes if syncing enabled
    if (bSyncWithGlobalManager && LoginMethodManager)
    {
        LoginMethodManager->OnLoginMethodChanged.AddDynamic(this, &ULoginMethodSelector::OnGlobalLoginMethodChanged);
    }

    // Set default selection index based on current global login method if syncing
    if (bSyncWithGlobalManager && LoginMethodManager)
    {
        ELoginMethod CurrentGlobalLoginMethod = LoginMethodManager->GetCurrentLoginMethod();
        int32 GlobalLoginMethodIndex = GetIndexForLoginMethod(CurrentGlobalLoginMethod);
        if (GlobalLoginMethodIndex >= 0)
        {
            DefaultSelectionIndex = GlobalLoginMethodIndex;
            UE_LOG(LogTemp, Warning, TEXT("LoginMethodSelector: Set DefaultSelectionIndex to %d for global login method %s"), 
                   DefaultSelectionIndex, *ULoginMethodManager::GetLoginMethodDisplayName(CurrentGlobalLoginMethod).ToString());
        }
    }

    // Call parent construct (this will handle DefaultSelectionIndex)
    Super::NativeConstruct();

    UE_LOG(LogTemp, Warning, TEXT("LoginMethodSelector: Constructed with %d login methods, DefaultSelectionIndex=%d"), 
           ActiveLoginMethods.Num(), DefaultSelectionIndex);
}

void ULoginMethodSelector::NativeDestruct()
{
    // Unbind from global login method manager
    if (LoginMethodManager)
    {
        LoginMethodManager->OnLoginMethodChanged.RemoveDynamic(this, &ULoginMethodSelector::OnGlobalLoginMethodChanged);
    }

    Super::NativeDestruct();
}

/*====================================================================================================================================
                                                         LOGIN METHOD MANAGEMENT
======================================================================================================================================*/

void ULoginMethodSelector::SetCurrentLoginMethod(ELoginMethod LoginMethod)
{
    int32 LoginMethodIndex = GetIndexForLoginMethod(LoginMethod);
    if (LoginMethodIndex >= 0)
    {
        SetSelectedIndex(LoginMethodIndex);
        UE_LOG(LogTemp, Warning, TEXT("LoginMethodSelector: Set login method to %s (index %d)"), 
               *ULoginMethodManager::GetLoginMethodDisplayName(LoginMethod).ToString(), LoginMethodIndex);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("LoginMethodSelector: Login method %s not found in active login methods"), 
               *ULoginMethodManager::GetLoginMethodDisplayName(LoginMethod).ToString());
    }
}

ELoginMethod ULoginMethodSelector::GetCurrentLoginMethod() const
{
    return GetLoginMethodAtIndex(GetSelectedIndex());
}

void ULoginMethodSelector::RefreshLoginMethodOptions()
{
    UE_LOG(LogTemp, Warning, TEXT("LoginMethodSelector: Refreshing login method options"));

    // Remember current selection
    ELoginMethod CurrentLoginMethod = GetCurrentLoginMethod();

    // Clear and repopulate
    ClearRadioOptions();
    PopulateLoginMethodOptions();

    // Restore selection if possible
    if (CurrentLoginMethod != ELoginMethod::AccountPortal || GetIndexForLoginMethod(CurrentLoginMethod) >= 0)
    {
        SetCurrentLoginMethod(CurrentLoginMethod);
    }
}

/*====================================================================================================================================
                                                         INTERNAL METHODS
======================================================================================================================================*/

void ULoginMethodSelector::PopulateLoginMethodOptions()
{
    // Determine which login methods to include
    if (IncludedLoginMethods.Num() > 0)
    {
        ActiveLoginMethods = IncludedLoginMethods;
    }
    else
    {
        ActiveLoginMethods = ULoginMethodManager::GetAllLoginMethods();
    }

    UE_LOG(LogTemp, Warning, TEXT("LoginMethodSelector: Populating %d login method options"), ActiveLoginMethods.Num());

    // Create radio button for each login method
    for (ELoginMethod LoginMethod : ActiveLoginMethods)
    {
        FRadioButtonConfig Config;
        Config.Label = ULoginMethodManager::GetLoginMethodDisplayName(LoginMethod);
        Config.Data = FString::Printf(TEXT("%d"), (int32)LoginMethod);

        // Add tooltip with description if enabled
        if (bShowMethodDescriptions)
        {
            // Note: Tooltip functionality would need to be added to GenericRadioButton if desired
            // For now, we just store the description in Data field for potential future use
            Config.Data += TEXT("|") + ULoginMethodManager::GetLoginMethodDescription(LoginMethod).ToString();
        }

        AddRadioOption(Config);
    }
}

void ULoginMethodSelector::SyncWithLoginMethodManager()
{
    if (!LoginMethodManager) return;

    ELoginMethod GlobalLoginMethod = LoginMethodManager->GetCurrentLoginMethod();
    ELoginMethod CurrentLoginMethod = GetCurrentLoginMethod();

    if (GlobalLoginMethod != CurrentLoginMethod)
    {
        UE_LOG(LogTemp, Warning, TEXT("LoginMethodSelector: Syncing with global login method manager - %s"), 
               *ULoginMethodManager::GetLoginMethodDisplayName(GlobalLoginMethod).ToString());
        SetCurrentLoginMethod(GlobalLoginMethod);
    }
}

ELoginMethod ULoginMethodSelector::GetLoginMethodAtIndex(int32 Index) const
{
    if (Index < 0 || Index >= ActiveLoginMethods.Num())
    {
        return ELoginMethod::AccountPortal; // Default fallback
    }
    return ActiveLoginMethods[Index];
}

int32 ULoginMethodSelector::GetIndexForLoginMethod(ELoginMethod LoginMethod) const
{
    return ActiveLoginMethods.Find(LoginMethod);
}

void ULoginMethodSelector::OnLoginMethodSelectionChanged(int32 SelectedIndex, UGenericRadioButton* SelectedButton)
{
    ELoginMethod SelectedLoginMethod = GetLoginMethodAtIndex(SelectedIndex);
    
    UE_LOG(LogTemp, Warning, TEXT("LoginMethodSelector: Login method selection changed to %s (index %d)"), 
           *ULoginMethodManager::GetLoginMethodDisplayName(SelectedLoginMethod).ToString(), SelectedIndex);

    // Update global login method manager if enabled
    if (bUpdateGlobalManager && LoginMethodManager)
    {
        ELoginMethod CurrentGlobalLoginMethod = LoginMethodManager->GetCurrentLoginMethod();
        if (CurrentGlobalLoginMethod != SelectedLoginMethod)
        {
            UE_LOG(LogTemp, Warning, TEXT("LoginMethodSelector: Updating global login method manager from %s to %s"), 
                   *ULoginMethodManager::GetLoginMethodDisplayName(CurrentGlobalLoginMethod).ToString(),
                   *ULoginMethodManager::GetLoginMethodDisplayName(SelectedLoginMethod).ToString());
            LoginMethodManager->SetCurrentLoginMethod(SelectedLoginMethod);
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("LoginMethodSelector: Global login method already set to %s, no update needed"), 
                   *ULoginMethodManager::GetLoginMethodDisplayName(SelectedLoginMethod).ToString());
        }
    }

    // Save to user preferences (we have proper world context here)
    if (UWorld* World = GetWorld())
    {
        if (USessionAdapter* SessionAdapter = Cast<USessionAdapter>(World->GetGameInstance()))
        {
            if (UUserPreferencesManager* PrefsManager = SessionAdapter->GetUserPreferencesManager())
            {
                UE_LOG(LogTemp, Warning, TEXT("LoginMethodSelector: Saving login method preference to user settings..."));
                PrefsManager->SetPreferredLoginMethod(SelectedLoginMethod, true);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("LoginMethodSelector: UserPreferencesManager is null"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("LoginMethodSelector: SessionAdapter is null or wrong type"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("LoginMethodSelector: Could not get world context"));
    }
}

void ULoginMethodSelector::OnGlobalLoginMethodChanged(ELoginMethod NewLoginMethod)
{
    // Only sync if the change came from outside (avoid infinite loops)
    ELoginMethod CurrentLoginMethod = GetCurrentLoginMethod();
    if (NewLoginMethod != CurrentLoginMethod)
    {
        UE_LOG(LogTemp, Warning, TEXT("LoginMethodSelector: Global login method changed to %s, syncing selector"), 
               *ULoginMethodManager::GetLoginMethodDisplayName(NewLoginMethod).ToString());
        SetCurrentLoginMethod(NewLoginMethod);
    }
}