// ColorSelectorWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "ColorDataObject.h"
#include "ColorSelectorWidget.generated.h"

USTRUCT(BlueprintType)
struct FColorOption
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ColorName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor ColorValue;

    FColorOption()
        : ColorName(TEXT("Default Color"))
        , ColorValue(FLinearColor::White)
    {}

    FColorOption(const FString& InName, const FLinearColor& InColor)
        : ColorName(InName)
        , ColorValue(InColor)
    {}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorSelected, const FString&, ColorName, const FLinearColor&, ColorValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorApplied, const FString&, ColorName, const FLinearColor&, ColorValue);

UCLASS(BlueprintType, Blueprintable)
class GRIT_API UColorSelectorWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UColorSelectorWidget(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;

    // Widget Components
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* TitleText;

    UPROPERTY(meta = (BindWidget))
    class UButton* FilterButton;

    UPROPERTY(meta = (BindWidget))
    class UListView* ColourListView;

    UPROPERTY(meta = (BindWidget))
    class UButton* ApplyButton;

    // Optional: Direct reference to Apply button's text (if named in Blueprint)
    UPROPERTY(meta = (BindWidget), meta = (OptionalWidget = true))
    class UTextBlock* ApplyButtonText;

    // Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Selector")
    FText WidgetTitle = FText::FromString("Exterior colors");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Selector")
    TArray<FColorOption> ColorOptions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Selector")
    int32 DefaultSelectedIndex = 0;

    // Styling
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Styling")
    FLinearColor BackgroundColor = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Styling")
    float ItemSpacing = 8.0f;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnColorSelected OnColorSelected;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnColorApplied OnColorApplied;

    // Blueprint Events
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnColorSelectedBP(const FString& ColorName, const FLinearColor& ColorValue);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnColorAppliedBP(const FString& ColorName, const FLinearColor& ColorValue);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnFilterButtonClickedBP();

public:
    // Public Functions
    UFUNCTION(BlueprintCallable, Category = "Color Selector")
    void PopulateColorList();

    UFUNCTION(BlueprintCallable, Category = "Color Selector")
    void InitializeWithTestData();

    UFUNCTION(BlueprintCallable, Category = "Color Selector")
    void SetSelectedColor(int32 Index);

    UFUNCTION(BlueprintCallable, Category = "Color Selector")
    void SetSelectedColorByName(const FString& ColorName);

    UFUNCTION(BlueprintCallable, Category = "Color Selector")
    FColorOption GetSelectedColor() const;

    // New getter functions for selected color data
    UFUNCTION(BlueprintCallable, Category = "Color Selector")
    FString GetSelectedColorName() const;

    UFUNCTION(BlueprintCallable, Category = "Color Selector")
    FLinearColor GetSelectedColorValue() const;

    UFUNCTION(BlueprintCallable, Category = "Color Selector")
    int32 GetSelectedColorIndex() const;

    UFUNCTION(BlueprintCallable, Category = "Color Selector")
    void AddColorOption(const FString& ColorName, const FLinearColor& ColorValue);

    UFUNCTION(BlueprintCallable, Category = "Color Selector")
    void ClearColorOptions();

    // Apply button state functions
    UFUNCTION(BlueprintCallable, Category = "Color Selector")
    bool HasAppliedSelection() const { return bHasAppliedSelection; }

    UFUNCTION(BlueprintCallable, Category = "Color Selector")
    void ResetAppliedState() { bHasAppliedSelection = false; UpdateApplyButtonText(); }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Color Selector")
    TSubclassOf<class UColorItemWidget> ColorItemWidgetClass;

    // Styling - made public so ColorItemWidget can access them
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Styling")
    FLinearColor ItemNormalColor = FLinearColor(0.25f, 0.25f, 0.25f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Styling")
    FLinearColor ItemHoveredColor = FLinearColor(0.29f, 0.29f, 0.29f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Styling")
    FLinearColor ItemSelectedColor = FLinearColor(0.04f, 0.35f, 0.16f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Styling")
    FLinearColor SelectedBorderColor = FLinearColor(0.0f, 1.0f, 0.4f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Styling")
    float BorderRadius = 8.0f;

private:
    int32 SelectedColorIndex;
    bool bHasAppliedSelection;
    
    // Data objects for ListView
    UPROPERTY()
    TArray<UColorDataObject*> ColorDataObjects;

    UFUNCTION()
    void OnApplyButtonClicked();

    UFUNCTION()
    void OnFilterButtonClicked();

    UFUNCTION()
    void OnColorItemSelected(int32 ItemIndex);

    // New callback for when list entry widgets are generated
    void OnListEntryWidgetGenerated(UUserWidget& EntryWidget);

    void UpdateSelection();
    void UpdateApplyButtonText();
    void SetupDefaultColors();
    void CreateColorDataObjects();
};