#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/Spacer.h"
#include "Components/Image.h"
#include "Components/BackgroundBlur.h"
#include "Components/ScrollBox.h"
#include "LobbyInterface.generated.h"

USTRUCT(BlueprintType)
struct FLobbyTheme
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor AccentColor = FLinearColor(1.0f, 0.0f, 0.291303f, 1.0f); // Pink

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor AccentLightColor = FLinearColor(1.0f, 0.234375f, 0.457404f, 1.0f); // Lighter Pink

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor BackgroundColor = FLinearColor(0.281250f, 0.281250f, 0.281250f, 1.0f); // Grey

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor DarkBackgroundColor = FLinearColor(0.031250f, 0.031250f, 0.031250f, 1.0f); // Darker Grey

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor PrimaryTextColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f); // White

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor SecondaryTextColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor ShadowColor = FLinearColor(0.692708f, 0.0f, 0.201787f, 1.0f); // Pink Shadow

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor InputBackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black Input

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor InputHoverColor = FLinearColor(0.0625f, 0.0625f, 0.0625f, 1.0f); // Input Hover

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor ButtonOutlineColor = FLinearColor(1.0f, 0.84375f, 0.889266f, 1.0f); // Button Outline
};

UCLASS(BlueprintType, Blueprintable)
class GRIT_API ULobbyInterface : public UUserWidget
{
    GENERATED_BODY()

public:
    ULobbyInterface(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme", meta = (ExposeOnSpawn = "true"))
    FLobbyTheme Theme;

    // Root Canvas
    UPROPERTY(meta = (BindWidget))
    class UCanvasPanel* Root_BackgroundCanvas;

    // Background Elements
    UPROPERTY(meta = (BindWidget))
    class UImage* BackgroundImage;

    UPROPERTY(meta = (BindWidget))
    class UImage* DropShadow;

    UPROPERTY(meta = (BindWidget))
    class UBackgroundBlur* ShadowBlur;

    // Main Stack
    UPROPERTY(meta = (BindWidget))
    class UVerticalBox* Container_MainStack;

    // Header Section
    UPROPERTY(meta = (BindWidget))
    class USizeBox* SizeBox_Header;

    UPROPERTY(meta = (BindWidget))
    class UBorder* Section_Header;

    UPROPERTY(meta = (BindWidget))
    class UVerticalBox* Stack_TitleBlock;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* Txt_Title;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* Txt_Description;

    UPROPERTY(meta = (BindWidget))
    class USpacer* Spacer_TitleBlock;

    UPROPERTY(meta = (BindWidget))
    class USpacer* Spacer_DescriptionBlock;

    UPROPERTY(meta = (BindWidget))
    class USizeBox* SizeBox_CreateButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* Btn_CreateLobby;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* Txt_CreateLobby;

    // Table Section
    UPROPERTY(meta = (BindWidget))
    class USizeBox* SizeBox_Table;

    UPROPERTY(meta = (BindWidget))
    class UBorder* Section_LobbyTable;

    // Main Vertical Container inside Table Section
    UPROPERTY(meta = (BindWidget))
    class UVerticalBox* VerticalBox_2;

    // Header Spacers
    UPROPERTY(meta = (BindWidget))
    class USpacer* Spacer_HeaderFlex;

    // Toolbar Container
    UPROPERTY(meta = (BindWidget))
    class USizeBox* SizeBox_7;

    // Toolbar
    UPROPERTY(meta = (BindWidget))
    class UBorder* Section_Toolbar;

    // Toolbar Content Container
    UPROPERTY(meta = (BindWidget))
    class UVerticalBox* VBox_ToolbarContent;

    UPROPERTY(meta = (BindWidget))
    class UHorizontalBox* HorizontalBox_0;

    UPROPERTY(meta = (BindWidget))
    class UEditableTextBox* Input_Search;

    UPROPERTY(meta = (BindWidget))
    class USpacer* Spacer_558;

    UPROPERTY(meta = (BindWidget))
    class USizeBox* SizeBox_8;

    UPROPERTY(meta = (BindWidget))
    class UButton* Button_47;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* Filters;

    UPROPERTY(meta = (BindWidget))
    class USpacer* Spacer_599;

    // Filter Buttons Container
    UPROPERTY(meta = (BindWidget))
    class UHorizontalBox* HBox_FilterButtons;

    UPROPERTY(meta = (BindWidget))
    class UButton* FilterButton_01;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* FilterDescription_01;

    UPROPERTY(meta = (BindWidget))
    class USpacer* Spacer_FilterSeperator_01;

    UPROPERTY(meta = (BindWidget))
    class UButton* FilterButton_02;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* FilterDescription_02;

    UPROPERTY(meta = (BindWidget))
    class USpacer* Spacer_FilterSeperator_02;

    UPROPERTY(meta = (BindWidget))
    class UButton* FilterButton_03;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* FilterDescription_03;

    UPROPERTY(meta = (BindWidget))
    class USpacer* Spacer_HeaderContentGap;

    // Lobby Entry Header (TODO: Replace with actual BP_LobbyEntryHeader class)
   // UPROPERTY(meta = (BindWidget))
   // class UUserWidget* BP_LobbyEntryHeader;


    UPROPERTY(meta = (BindWidget))
    class USpacer* Spacer_TableContents;

    // Table List Section
    UPROPERTY(meta = (BindWidget))
    class USizeBox* SizeBox_LobbyList;

    UPROPERTY(meta = (BindWidget))
    class UScrollBox* SessionEntries;

    UPROPERTY(meta = (BindWidget))
    class USpacer* Spacer_TableDetails;

    // Footer Section
    UPROPERTY(meta = (BindWidget))
    class UHorizontalBox* HBox_TableFooter;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* Txt_LobbyCount;

    UPROPERTY(meta = (BindWidget))
    class USpacer* Spacer_Seperator_01;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* Txt_RulesDisplay;

    UPROPERTY(meta = (BindWidget))
    class USpacer* Spacer_Seperator_02;

    UPROPERTY(meta = (BindWidget))
    class USizeBox* SizeBox_12;

    UPROPERTY(meta = (BindWidget))
    class UButton* Button_125;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* Txt_ActionButton;

	    /** Percentage of screen width the container should occupy (0.0 to 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Layout", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float ContainerWidthPercentage = 0.78125f;

	/** Percentage of screen height the container should occupy (0.0 to 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Layout", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float ContainerHeightPercentage = 0.92593f;



private:
    void ApplyTheme();
    void SetupLayout();
    void ApplyFilterButtonStyle(UButton* Button, UTextBlock* TextBlock);
};