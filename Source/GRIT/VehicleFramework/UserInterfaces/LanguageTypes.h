#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "LanguageTypes.generated.h"

/*====================================================================================================================================
                                                         LANGUAGE SYSTEM TYPES
======================================================================================================================================*/

UENUM(BlueprintType)
enum class ELanguage : uint8
{
    English     UMETA(DisplayName = "English"),
    Spanish     UMETA(DisplayName = "Español"),
    French      UMETA(DisplayName = "Français"),
    German      UMETA(DisplayName = "Deutsch"),
    Italian     UMETA(DisplayName = "Italiano"),
    Portuguese  UMETA(DisplayName = "Português"),
    Russian     UMETA(DisplayName = "Русский"),
    Japanese    UMETA(DisplayName = "日本語"),
    Korean      UMETA(DisplayName = "한국어"),
    Chinese     UMETA(DisplayName = "中文")
};

// Global delegate for language changes - any class can bind to this
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLanguageChanged, ELanguage, NewLanguage);

/*====================================================================================================================================
                                                         LANGUAGE MANAGER (SINGLETON)
======================================================================================================================================*/

UCLASS(BlueprintType, Blueprintable)
class GRIT_API ULanguageManager : public UObject
{
    GENERATED_BODY()

public:
    /** Get the global language manager instance */
    UFUNCTION(BlueprintCallable, Category = "Language", CallInEditor)
    static ULanguageManager* GetInstance();

    /** Set current language and broadcast to all listeners */
    UFUNCTION(BlueprintCallable, Category = "Language")
    void SetCurrentLanguage(ELanguage NewLanguage);

    /** Get current language */
    UFUNCTION(BlueprintPure, Category = "Language")
    ELanguage GetCurrentLanguage() const { return CurrentLanguage; }

    /** Get language display name */
    UFUNCTION(BlueprintPure, Category = "Language")
    static FText GetLanguageDisplayName(ELanguage Language);

    /** Get all available languages */
    UFUNCTION(BlueprintPure, Category = "Language")
    static TArray<ELanguage> GetAllLanguages();

    /** Global delegate that any class can bind to for language changes */
    UPROPERTY(BlueprintAssignable, Category = "Language Events")
    FOnLanguageChanged OnLanguageChanged;

private:
    static ULanguageManager* Instance;

    UPROPERTY()
    ELanguage CurrentLanguage = ELanguage::English;

    ULanguageManager();
};