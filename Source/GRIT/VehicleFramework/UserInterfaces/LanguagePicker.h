#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "LanguagePickerPanel.h"
#include "LanguagePicker.generated.h"

//------------------------------------------------------------------------------
//                            LANGUAGE PICKER
//------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable)
class GRIT_API ULanguagePicker : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Toggle language container visibility */
    UFUNCTION(BlueprintCallable, Category = "Language")
    void ToggleContainer();

    /** Close container if open */
    UFUNCTION(BlueprintCallable, Category = "Language")
    void CloseContainer();

    /** Set display language text */
    UFUNCTION(BlueprintCallable, Category = "Language")
    void SetDisplayLanguage(const FString& LanguageName);

    /** Add available language to dropdown */
    UFUNCTION(BlueprintCallable, Category = "Language")
    void AddLanguage(const FString& DisplayName, const FString& LanguageCode);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    //------------------------------------------------------------------------------
    // Widget bindings
    //------------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* SelectorButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* CurrentLanguageText;

    //------------------------------------------------------------------------------
    // Configuration
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    TSubclassOf<ULanguagePickerPanel> ContainerClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    FString DefaultLanguage = TEXT("English");

private:
    UPROPERTY()
    ULanguagePickerPanel* ActiveContainer;

    bool bContainerOpen = false;

    //------------------------------------------------------------------------------
    // Container management
    //------------------------------------------------------------------------------

    /** Spawn language container at button position */
    void SpawnContainer();

    /** Destroy active container */
    void DestroyContainer();

    //------------------------------------------------------------------------------
    // Event handlers
    //------------------------------------------------------------------------------

    UFUNCTION()
    void ProcessButtonClicked();

    UFUNCTION()
    void ProcessLanguageChanged(const FString& SelectedLanguage);
};
