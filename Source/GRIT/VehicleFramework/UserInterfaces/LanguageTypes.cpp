#include "LanguageTypes.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "../../GameContext/SessionAdapter.h"

/*====================================================================================================================================
                                                         LANGUAGE MANAGER IMPLEMENTATION
======================================================================================================================================*/

ULanguageManager* ULanguageManager::Instance = nullptr;

ULanguageManager::ULanguageManager()
{
    CurrentLanguage = ELanguage::English;
}

ULanguageManager* ULanguageManager::GetInstance()
{
    if (!Instance)
    {
        Instance = NewObject<ULanguageManager>();
        Instance->AddToRoot(); // Prevent garbage collection
        UE_LOG(LogTemp, Warning, TEXT("LanguageManager: Created singleton instance"));
    }
    return Instance;
}

void ULanguageManager::SetCurrentLanguage(ELanguage NewLanguage)
{
    if (CurrentLanguage == NewLanguage) return;

    ELanguage OldLanguage = CurrentLanguage;
    CurrentLanguage = NewLanguage;

    UE_LOG(LogTemp, Warning, TEXT("LanguageManager: Language changed from %s to %s"), 
           *GetLanguageDisplayName(OldLanguage).ToString(),
           *GetLanguageDisplayName(NewLanguage).ToString());

    // Broadcast to all listeners
    OnLanguageChanged.Broadcast(CurrentLanguage);

    // Note: Saving is now handled by the UI components that have proper world context
    // This prevents the "No world was found" error
}

FText ULanguageManager::GetLanguageDisplayName(ELanguage Language)
{
    switch (Language)
    {
        case ELanguage::English:    return FText::FromString(TEXT("English"));
        case ELanguage::Spanish:    return FText::FromString(TEXT("Español"));
        case ELanguage::French:     return FText::FromString(TEXT("Français"));
        case ELanguage::German:     return FText::FromString(TEXT("Deutsch"));
        case ELanguage::Italian:    return FText::FromString(TEXT("Italiano"));
        case ELanguage::Portuguese: return FText::FromString(TEXT("Português"));
        case ELanguage::Russian:    return FText::FromString(TEXT("Русский"));
        case ELanguage::Japanese:   return FText::FromString(TEXT("日本語"));
        case ELanguage::Korean:     return FText::FromString(TEXT("한국어"));
        case ELanguage::Chinese:    return FText::FromString(TEXT("中文"));
        default:                    return FText::FromString(TEXT("Unknown"));
    }
}

TArray<ELanguage> ULanguageManager::GetAllLanguages()
{
    return {
        ELanguage::English,
        ELanguage::Spanish,
        ELanguage::French,
        ELanguage::German,
        ELanguage::Italian,
        ELanguage::Portuguese,
        ELanguage::Russian,
        ELanguage::Japanese,
        ELanguage::Korean,
        ELanguage::Chinese
    };
}