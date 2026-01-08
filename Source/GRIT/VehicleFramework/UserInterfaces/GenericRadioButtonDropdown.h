#pragma once

#include "CoreMinimal.h"
#include "GenericDropdownMenu.h"
#include "GenericRadioButton.h"
#include "GenericRadioButtonDropdown.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRadioDropdownSelectionChanged, int32, SelectedIndex, UGenericRadioButton*, SelectedButton);

/*====================================================================================================================================
                                                         RADIO BUTTON CONFIG
======================================================================================================================================*/

USTRUCT(BlueprintType)
struct FRadioButtonConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    FText Label = FText::FromString("Option");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    UTexture2D* Icon = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    FString Data = "";

    FRadioButtonConfig() {}
};

/*====================================================================================================================================
                                                         GENERIC RADIO BUTTON DROPDOWN
======================================================================================================================================*/

UCLASS(BlueprintType, Blueprintable)
class GRIT_API UGenericRadioButtonDropdown : public UGenericDropdownMenu
{
    GENERATED_BODY()

public:
    UGenericRadioButtonDropdown(const FObjectInitializer& ObjectInitializer);

    /** Add radio button option */
    UFUNCTION(BlueprintCallable, Category = "RadioDropdown")
    void AddRadioOption(const FRadioButtonConfig& Config);

    /** Clear all radio options */
    UFUNCTION(BlueprintCallable, Category = "RadioDropdown")
    void ClearRadioOptions();

    /** Set selected option by index */
    UFUNCTION(BlueprintCallable, Category = "RadioDropdown")
    void SetSelectedIndex(int32 Index);

    /** Get currently selected index */
    UFUNCTION(BlueprintPure, Category = "RadioDropdown")
    int32 GetSelectedIndex() const { return CurrentSelectionIndex; }

    /** Get currently selected button */
    UFUNCTION(BlueprintPure, Category = "RadioDropdown")
    UGenericRadioButton* GetSelectedButton() const;

    /** Get radio button by index */
    UFUNCTION(BlueprintPure, Category = "RadioDropdown")
    UGenericRadioButton* GetRadioButtonAt(int32 Index) const;

    /** Get number of radio options */
    UFUNCTION(BlueprintPure, Category = "RadioDropdown")
    int32 GetRadioOptionCount() const { return RadioButtons.Num(); }

    /** Debug method to validate radio button states */
    UFUNCTION(BlueprintCallable, Category = "RadioDropdown|Debug")
    void ValidateRadioButtonStates();

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnRadioDropdownSelectionChanged OnSelectionChanged;

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnSelectionChangedBP(int32 SelectedIndex, UGenericRadioButton* SelectedButton);

protected:
    virtual void NativeConstruct() override;

    //------------------------------------------------------------------------------
    // Configuration
    //------------------------------------------------------------------------------

    /** Default radio button options to populate on construction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|RadioOptions")
    TArray<FRadioButtonConfig> DefaultRadioOptions;

    /** Radio button widget class to use */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|RadioOptions")
    TSubclassOf<UGenericRadioButton> RadioButtonClass;

    /** Default selected index */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|RadioOptions")
    int32 DefaultSelectionIndex = 0;

    /** Auto-close dropdown when option is selected */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Behavior")
    bool bAutoCloseOnSelection = true;

    /** Update trigger text with selected option */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Behavior")
    bool bUpdateTriggerText = true;

    //------------------------------------------------------------------------------
    // Runtime state
    //------------------------------------------------------------------------------

    UPROPERTY()
    TArray<UGenericRadioButton*> RadioButtons;

    int32 CurrentSelectionIndex = -1;

    //------------------------------------------------------------------------------
    // Internal methods
    //------------------------------------------------------------------------------

    void PopulateDefaultOptions();
    void UpdateTriggerDisplay();

    UFUNCTION()
    void OnRadioButtonToggled(bool bSelected);
};