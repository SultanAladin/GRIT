#include "LoginMethodTypes.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "../../GameContext/SessionAdapter.h"

/*====================================================================================================================================
                                                         LOGIN METHOD MANAGER IMPLEMENTATION
======================================================================================================================================*/

ULoginMethodManager* ULoginMethodManager::Instance = nullptr;

ULoginMethodManager::ULoginMethodManager()
{
    CurrentLoginMethod = ELoginMethod::AccountPortal;
}

ULoginMethodManager* ULoginMethodManager::GetInstance()
{
    if (!Instance)
    {
        Instance = NewObject<ULoginMethodManager>();
        Instance->AddToRoot(); // Prevent garbage collection
        UE_LOG(LogTemp, Warning, TEXT("LoginMethodManager: Created singleton instance"));
    }
    return Instance;
}

void ULoginMethodManager::SetCurrentLoginMethod(ELoginMethod NewLoginMethod)
{
    if (CurrentLoginMethod == NewLoginMethod) return;

    ELoginMethod OldLoginMethod = CurrentLoginMethod;
    CurrentLoginMethod = NewLoginMethod;

    UE_LOG(LogTemp, Warning, TEXT("LoginMethodManager: Login method changed from %s to %s"), 
           *GetLoginMethodDisplayName(OldLoginMethod).ToString(),
           *GetLoginMethodDisplayName(NewLoginMethod).ToString());

    // Broadcast to all listeners
    OnLoginMethodChanged.Broadcast(CurrentLoginMethod);

    // Note: Saving is now handled by the UI components that have proper world context
    // This prevents the "No world was found" error
}

FText ULoginMethodManager::GetLoginMethodDisplayName(ELoginMethod LoginMethod)
{
    switch (LoginMethod)
    {
        case ELoginMethod::AccountPortal:    return FText::FromString(TEXT("Account Portal"));
        case ELoginMethod::PersistentAuth:   return FText::FromString(TEXT("Persistent Auth"));
        case ELoginMethod::ConnectInterface: return FText::FromString(TEXT("Connect Interface"));
        case ELoginMethod::DevAuth:          return FText::FromString(TEXT("Developer Auth"));
        case ELoginMethod::ExchangeCode:     return FText::FromString(TEXT("Exchange Code"));
        default:                             return FText::FromString(TEXT("Unknown"));
    }
}

FText ULoginMethodManager::GetLoginMethodDescription(ELoginMethod LoginMethod)
{
    switch (LoginMethod)
    {
        case ELoginMethod::AccountPortal:
            return FText::FromString(TEXT("Login using Epic Games account portal (web browser) - Available"));
        case ELoginMethod::PersistentAuth:
            return FText::FromString(TEXT("Login using saved credentials (automatic) - Available"));
        case ELoginMethod::ConnectInterface:
            return FText::FromString(TEXT("Login using platform-specific authentication - Available"));
        case ELoginMethod::DevAuth:
            return FText::FromString(TEXT("Developer authentication for testing - Not implemented (uses PersistentAuth)"));
        case ELoginMethod::ExchangeCode:
            return FText::FromString(TEXT("Login using exchange code from launcher - Not implemented (uses PersistentAuth)"));
        default:
            return FText::FromString(TEXT("Unknown login method"));
    }
}

TArray<ELoginMethod> ULoginMethodManager::GetAllLoginMethods()
{
    return {
        ELoginMethod::AccountPortal,
        ELoginMethod::PersistentAuth,
        ELoginMethod::ConnectInterface,
        ELoginMethod::DevAuth,
        ELoginMethod::ExchangeCode
    };
}