#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/VerticalBox.h"
#include "Components/Border.h"
#include "LanguagePickerEntry.h"
#include "LanguagePickerPanel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLanguagePickerChanged, const FString&, SelectedLanguage);

//------------------------------------------------------------------------------
//                        LANGUAGE PICKER PANEL
//------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable)
class GRIT_API ULanguagePickerPanel : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Delegate fired when language selection changes */
    UPROPERTY(BlueprintAssignable, Category = "Language")
    FOnLanguagePickerChanged OnLanguageChanged;

    /** Add language option to container */
    UFUNCTION(BlueprintCallable, Category = "Language")
    void AddLanguageOption(const FString& DisplayName, const FString& LanguageCode);

    /** Set currently selected language */
    UFUNCTION(BlueprintCallable, Category = "Language")
    void SetSelectedLanguage(const FString& LanguageCode);

    /** Position container relative to spawn button with smart bounds checking */
    UFUNCTION(BlueprintCallable, Category = "Language")
    void PositionRelativeToButton(UWidget* SpawnButton);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    //------------------------------------------------------------------------------
    // Widget bindings
    //------------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UBorder* ContainerBorder;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox* LanguageList;

    //------------------------------------------------------------------------------
    // Configuration
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    TSubclassOf<ULanguagePickerEntry> LanguageEntryClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    FVector2D SpacingFromButton = FVector2D(0.0f, 10.0f);  // [px] - Offset from spawn button (X, Y)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    float EdgeMargin = 20.0f;  // [px] - Minimum distance from screen edges

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    FLinearColor BorderColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.2f);  // [RGBA] - Container border color

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    FLinearColor BackgroundColor = FLinearColor(0.05f, 0.05f, 0.05f, 0.95f);  // [RGBA] - Container background

private:
    UPROPERTY()
    TArray<ULanguagePickerEntry*> LanguageEntries;

    FString CurrentLanguageCode;

    //------------------------------------------------------------------------------
    // Positioning logic
    //------------------------------------------------------------------------------

    /** Calculate optimal position avoiding screen clipping */
    FVector2D CalculateOptimalPosition(const FVector2D& ButtonPos, const FVector2D& ButtonSize, const FVector2D& ContainerSize) const;

    /** Clamp position within viewport bounds */
    FVector2D ClampToViewport(const FVector2D& Position, const FVector2D& ContainerSize) const;

    //------------------------------------------------------------------------------
    // Event handlers
    //------------------------------------------------------------------------------

    UFUNCTION()
    void ProcessLanguageSelected(const FString& LanguageCode);
};
