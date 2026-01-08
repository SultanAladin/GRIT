#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Components/Spacer.h"
#include "ColourStackEntry.h"
#include "ColourStackInterface.generated.h"

// Color Category Enum
UENUM(BlueprintType)
enum class EColorCategory : uint8
{
    Fruit       UMETA(DisplayName = "Fruit"),
    Pastel      UMETA(DisplayName = "Pastel"),
    Matter      UMETA(DisplayName = "Matter"),
    Nature      UMETA(DisplayName = "Nature"),
    Metallic    UMETA(DisplayName = "Metallic"),
    Gemstone    UMETA(DisplayName = "Gemstone")
};

// Universal Color Data Structure
USTRUCT(BlueprintType)
struct GRIT_API FColorPaletteEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color")
    FString ColorName = "Default";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color")
    FLinearColor Color = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color")
    EColorCategory Category = EColorCategory::Fruit;

    FColorPaletteEntry()
    {
        ColorName = "Default";
        Color = FLinearColor::White;
        Category = EColorCategory::Fruit;
    }

    FColorPaletteEntry(const FString& InName, const FLinearColor& InColor, EColorCategory InCategory)
        : ColorName(InName), Color(InColor), Category(InCategory)
    {
    }
};

UCLASS()
class GRIT_API UColourStackInterface : public UUserWidget
{
    GENERATED_BODY()

public:
    UColourStackInterface(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativePreConstruct() override;

    // Blueprint class reference for the color entry widget
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget Classes")
    TSubclassOf<UColourStackEntry> ColorEntryWidgetClass;

    // Color Theme - 3 Colors
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Colors")
    FLinearColor BackgroundColor = FLinearColor(0.015625f, 0.015625f, 0.015625f, 1.0f); // Dark Grey

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Colors")
    FLinearColor ButtonColor = FLinearColor(0.062500f, 0.062500f, 0.062500f, 1.0f); // Medium Grey

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Colors")
    FLinearColor HighlightColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f); // White

    // Font Colors
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Colors")
    FLinearColor FontColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f); // White

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Colors")
    FLinearColor ButtonTextColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f); // White (normal)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Colors")
    FLinearColor ButtonTextHoverColor = FLinearColor(0.062500f, 0.062500f, 0.062500f, 1.0f); // Grey (hover)

    // Main UI Components
    UPROPERTY(meta = (BindWidget))
    class UBorder* ColorStackInterface;

    UPROPERTY(meta = (BindWidget))
    class UVerticalBox* MainStackContainer;

    UPROPERTY(meta = (BindWidget))
    class UHorizontalBox* HeaderBox;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* HeaderTitle;

    UPROPERTY(meta = (BindWidget))
    class USpacer* PostHeaderSpacer;

    UPROPERTY(meta = (BindWidget))
    class USpacer* HeaderSpacer;

    UPROPERTY(meta = (BindWidget))
    class USizeBox* FilterContainer;

    UPROPERTY(meta = (BindWidget))
    class USizeBox* ColourStreamEntryContainer;

    UPROPERTY(meta = (BindWidget))
    class UScrollBox* ColorStreamContainer;

    UPROPERTY(meta = (BindWidget))
    class USpacer* Post_ButtonSpacer;

    UPROPERTY(meta = (BindWidget))
    class UButton* Btn_Apply;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* Txt_Apply;

    // Button Event Handlers
    UFUNCTION()
    void OnApplyButtonClicked();

    UFUNCTION()
    void OnApplyButtonHovered();

    UFUNCTION()
    void OnApplyButtonUnhovered();

    // Color Entry Event Handler
    UFUNCTION()
    void OnColorEntrySelected(UColourStackEntry* SelectedEntry, const FString& ColorID);

        // Editable color list in editor
       // NEW - This prevents Blueprint from overriding the C++ defaults:
    UPROPERTY(BlueprintReadOnly, Category = "Color Configuration", meta = (TitleProperty = "ColorName"))
    TArray<FColorPaletteEntry> ColorPalette;

    // DEBUG FUNCTIONS
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void ForceInitializeColors();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void ForcePopulateColorList();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    int32 GetColorPaletteCount() const { return ColorPalette.Num(); }

    UFUNCTION(BlueprintCallable, Category = "Debug")
    int32 GetScrollBoxChildCount() const { return ColorStreamContainer ? ColorStreamContainer->GetChildrenCount() : -1; }

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void DebugWidgetHierarchy();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void TestWidgetCreation();

private:
    void InitializeTheme();
    void SetupButtonStyles();
    void PopulateColorList();
    void InitializeDefaultColors();
    void DelayedPopulateColorList();

    // Track the currently selected color entry
    UPROPERTY()
    UColourStackEntry* CurrentlySelectedEntry;

    // Store all color entries for management
    UPROPERTY()
    TArray<UColourStackEntry*> ColorEntries;
};