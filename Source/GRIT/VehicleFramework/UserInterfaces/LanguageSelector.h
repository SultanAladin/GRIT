#pragma once

#include "CoreMinimal.h"
#include "GenericRadioButtonDropdown.h"
#include "LanguageTypes.h"
#include "LanguageSelector.generated.h"

/*====================================================================================================================================
                                                         LANGUAGE SELECTOR
======================================================================================================================================*/

UCLASS(BlueprintType, Blueprintable)
class GRIT_API ULanguageSelector : public UGenericRadioButtonDropdown
{
    GENERATED_BODY()

public:
    ULanguageSelector(const FObjectInitializer& ObjectInitializer);

    /** Set current language */
    UFUNCTION(BlueprintCallable, Category = "Language")
    void SetCurrentLanguage(ELanguage Language);

    /** Get current language */
    UFUNCTION(BlueprintPure, Category = "Language")
    ELanguage GetCurrentLanguage() const;

    /** Refresh language options (useful if languages are added/removed) */
    UFUNCTION(BlueprintCallable, Category = "Language")
    void RefreshLanguageOptions();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    //------------------------------------------------------------------------------
    // Configuration
    //------------------------------------------------------------------------------

    /** Languages to include in selector (empty = all languages) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Languages")
    TArray<ELanguage> IncludedLanguages;

    /** Auto-sync with global language manager */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Behavior")
    bool bSyncWithGlobalManager = true;

    /** Apply language change to global manager when selected */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Behavior")
    bool bUpdateGlobalManager = true;

    //------------------------------------------------------------------------------
    // Runtime state
    //------------------------------------------------------------------------------

    UPROPERTY()
    TArray<ELanguage> ActiveLanguages;

    UPROPERTY()
    ULanguageManager* LanguageManager;

    //------------------------------------------------------------------------------
    // Internal methods
    //------------------------------------------------------------------------------

    void PopulateLanguageOptions();
    void SyncWithLanguageManager();
    ELanguage GetLanguageAtIndex(int32 Index) const;
    int32 GetIndexForLanguage(ELanguage Language) const;

    UFUNCTION()
    void OnLanguageSelectionChanged(int32 SelectedIndex, UGenericRadioButton* SelectedButton);

    UFUNCTION()
    void OnGlobalLanguageChanged(ELanguage NewLanguage);
};