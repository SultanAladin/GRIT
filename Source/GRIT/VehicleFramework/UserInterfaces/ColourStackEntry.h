#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "ColourStackEntry.generated.h"

UCLASS()
class GRIT_API UColourStackEntry : public UUserWidget
{
    GENERATED_BODY()

public:
    UColourStackEntry(const FObjectInitializer& ObjectInitializer);

    /* ---------- Public Blueprint API ---------- */
    UFUNCTION(BlueprintCallable, Category = "Color Entry")
    void SetColorData(const FString& InColorName, const FLinearColor& InColorPreview, const FString& InColorID = "");

    UFUNCTION(BlueprintCallable, Category = "Color Entry")
    void UpdateColorPreview(const FLinearColor& NewColor);

    UFUNCTION(BlueprintCallable, Category = "Color Entry")
    void SetSelected(bool bIsSelected);

    UFUNCTION(BlueprintPure, Category = "Color Entry")
    FString GetColorID() const { return ColorID; }

    UFUNCTION(BlueprintPure, Category = "Color Entry")
    FLinearColor GetColorPreview() const { return ColorPreview; }

    /* ---------- Delegates ---------- */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorEntrySelected, UColourStackEntry*, SelectedEntry, const FString&, ColorID);
    UPROPERTY(BlueprintAssignable)
    FOnColorEntrySelected OnColorEntrySelected;

protected:
    virtual void NativeConstruct() override;
    virtual void NativePreConstruct() override;

    /* ---------- Theme Colours (fully exposed to Blueprint) ---------- */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Colours")
    FLinearColor EntryBackgroundColor = FLinearColor(0.026042f, 0.026042f, 0.026042f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Colours")
    FLinearColor EntryHighlightColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Colours")
    FLinearColor EntryTextColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Colours")
    FLinearColor EntryTextHoverColor = FLinearColor(0.026042f, 0.026042f, 0.026042f, 1.0f);

    /* ---------- Corner Style Toggle ---------- */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Corners")
    bool bUseRoundedCorners = true;

    /* ---------- Colour Data ---------- */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Data")
    FString ColorDisplayName = "Default";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Data")
    FLinearColor ColorPreview = FLinearColor(1.0f, 0.326969f, 0.902786f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Data")
    FString ColorID = "";

    /* ---------- UI Widgets ---------- */
    UPROPERTY(meta = (BindWidget))
    class UButton* ChromaSelector;

    UPROPERTY(meta = (BindWidget))
    class UHorizontalBox* PigmentLayoutContainer;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* HueIdentifier;

    UPROPERTY(meta = (BindWidget))
    class USizeBox* SpectrumPreviewContainer;

    UPROPERTY(meta = (BindWidget))
    class UImage* TintSample;

    /* ---------- Event Handlers ---------- */
    UFUNCTION()
    void OnChromaSelectorClicked();

    UFUNCTION()
    void OnChromaSelectorHovered();

    UFUNCTION()
    void OnChromaSelectorUnhovered();

private:
    void InitializeEntryTheme();
    void ConfigureChromaSelectorStyles();
    void RefreshVisualElements();

    bool bIsCurrentlySelected = false;
};