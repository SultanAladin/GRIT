#pragma once

#include "CoreMinimal.h"
#include "GenericRadioButtonDropdown.h"
#include "LoginMethodTypes.h"
#include "LoginMethodSelector.generated.h"

/*====================================================================================================================================
                                                         LOGIN METHOD SELECTOR
======================================================================================================================================*/

UCLASS(BlueprintType, Blueprintable)
class GRIT_API ULoginMethodSelector : public UGenericRadioButtonDropdown
{
    GENERATED_BODY()

public:
    ULoginMethodSelector(const FObjectInitializer& ObjectInitializer);

    /** Set current login method */
    UFUNCTION(BlueprintCallable, Category = "LoginMethod")
    void SetCurrentLoginMethod(ELoginMethod LoginMethod);

    /** Get current login method */
    UFUNCTION(BlueprintPure, Category = "LoginMethod")
    ELoginMethod GetCurrentLoginMethod() const;

    /** Refresh login method options (useful if methods are added/removed) */
    UFUNCTION(BlueprintCallable, Category = "LoginMethod")
    void RefreshLoginMethodOptions();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    //------------------------------------------------------------------------------
    // Configuration
    //------------------------------------------------------------------------------

    /** Login methods to include in selector (empty = all methods) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|LoginMethods")
    TArray<ELoginMethod> IncludedLoginMethods;

    /** Auto-sync with global login method manager */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Behavior")
    bool bSyncWithGlobalManager = true;

    /** Apply login method change to global manager when selected */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Behavior")
    bool bUpdateGlobalManager = true;

    /** Show method descriptions as tooltips */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Display")
    bool bShowMethodDescriptions = true;

    //------------------------------------------------------------------------------
    // Runtime state
    //------------------------------------------------------------------------------

    UPROPERTY()
    TArray<ELoginMethod> ActiveLoginMethods;

    UPROPERTY()
    ULoginMethodManager* LoginMethodManager;

    //------------------------------------------------------------------------------
    // Internal methods
    //------------------------------------------------------------------------------

    void PopulateLoginMethodOptions();
    void SyncWithLoginMethodManager();
    ELoginMethod GetLoginMethodAtIndex(int32 Index) const;
    int32 GetIndexForLoginMethod(ELoginMethod LoginMethod) const;

    UFUNCTION()
    void OnLoginMethodSelectionChanged(int32 SelectedIndex, UGenericRadioButton* SelectedButton);

    UFUNCTION()
    void OnGlobalLoginMethodChanged(ELoginMethod NewLoginMethod);
};