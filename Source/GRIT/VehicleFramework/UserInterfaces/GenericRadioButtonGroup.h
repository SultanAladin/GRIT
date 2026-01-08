#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GenericRadioButton.h"
#include "GenericRadioButtonGroup.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRadioGroupSelectionChanged, int32, SelectedIndex);

/*====================================================================================================================================
                                                         GENERIC RADIO BUTTON GROUP
======================================================================================================================================*/

UCLASS(BlueprintType, Blueprintable)
class GRIT_API UGenericRadioButtonGroup : public UUserWidget
{
	GENERATED_BODY()

public:
	UGenericRadioButtonGroup(const FObjectInitializer& ObjectInitializer);

	/** Register radio button to group */
	UFUNCTION(BlueprintCallable, Category = "RadioGroup") void BindRadioButton(UGenericRadioButton* RadioButton);
	
	/** Remove radio button from group */
	UFUNCTION(BlueprintCallable, Category = "RadioGroup") void UnbindRadioButton(UGenericRadioButton* RadioButton);
	
	/** Force selection of specific button */
	UFUNCTION(BlueprintCallable, Category = "RadioGroup") void ForceSelection(int32 Index);
	
	/** Get currently selected index */
	UFUNCTION(BlueprintPure, Category = "RadioGroup") int32 GetCurrentSelection() const { return CurrentSelectionIndex; }
	
	/** Get currently selected button */
	UFUNCTION(BlueprintPure, Category = "RadioGroup") UGenericRadioButton* GetCurrentButton() const;

	UPROPERTY(BlueprintAssignable, Category = "RadioGroup Events")
	FOnRadioGroupSelectionChanged OnSelectionChanged;

protected:
	virtual void NativeConstruct() override;

	//------------------------------------------------------------------------------
	// Configuration
	//------------------------------------------------------------------------------
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RadioGroup|Config")
	int32 DefaultSelectionIndex = 0; // [-] - Initial selection index

	//------------------------------------------------------------------------------
	// Runtime state
	//------------------------------------------------------------------------------
	
	UPROPERTY()
	TArray<UGenericRadioButton*> RadioButtons; // [-] - Managed button collection
	
	int32 CurrentSelectionIndex = -1; // [-] - Active button index

	//------------------------------------------------------------------------------
	// Internal methods
	//------------------------------------------------------------------------------
	
	UFUNCTION() void OnRadioButtonToggled(bool bSelected);
	void LockSelection(int32 Index);
};
