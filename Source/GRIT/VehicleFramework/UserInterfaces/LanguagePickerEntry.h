#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "LanguagePickerEntry.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLanguageSelected, const FString&, LanguageCode);

//------------------------------------------------------------------------------
//                           LANGUAGE PICKER ENTRY
//------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable)
class GRIT_API ULanguagePickerEntry : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Delegate fired when language is selected */
    UPROPERTY(BlueprintAssignable, Category = "Language")
    FOnLanguageSelected OnLanguageSelected;

    /** Set language display name and code */
    UFUNCTION(BlueprintCallable, Category = "Language")
    void SetLanguageData(const FString& DisplayName, const FString& Code);

    /** Set selection state visual feedback */
    UFUNCTION(BlueprintCallable, Category = "Language")
    void SetSelected(bool bIsSelected);

protected:
    virtual void NativeConstruct() override;

    //------------------------------------------------------------------------------
    // Widget bindings
    //------------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* EntryButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* LanguageText;

    //------------------------------------------------------------------------------
    // Configuration
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
    FLinearColor DefaultColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);  // [RGBA] - Default text color

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
    FLinearColor HoverColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);  // [RGBA] - Hover text color

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
    FLinearColor SelectedColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);  // [RGBA] - Selected text color

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    int32 FontSize = 14;  // [pt] - Text size

private:
    FString LanguageCode;
    bool bIsSelected = false;

    //------------------------------------------------------------------------------
    // Event handlers
    //------------------------------------------------------------------------------

    UFUNCTION()
    void ProcessClicked();

    UFUNCTION()
    void ProcessHovered();

    UFUNCTION()
    void ProcessUnhovered();

    /** Update visual state based on hover/selection */
    void RefreshVisualState();
};
