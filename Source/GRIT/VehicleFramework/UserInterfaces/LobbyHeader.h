#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/BorderSlot.h"
#include "Components/SizeBoxSlot.h"
#include "Engine/Font.h"
#include "Styling/SlateColor.h"
#include "LobbyHeader.generated.h"

USTRUCT(BlueprintType)
struct GRIT_API FLobbyHeaderTheme
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor Primary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor Text;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor HeaderText;

    FLobbyHeaderTheme()
    {
        Primary = FLinearColor(0.010f, 0.010f, 0.010f, 1.0f);        // Slightly lighter than entry background
        Text = FLinearColor(0.8f, 0.8f, 0.8f, 1.0f);                // Muted white text
        HeaderText = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);          // Bright white for headers
    }
};

UCLASS()
class GRIT_API ULobbyHeader : public UUserWidget
{
    GENERATED_BODY()

public:
    ULobbyHeader(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;

private:
    // Theme Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme", meta = (AllowPrivateAccess = "true"))
    FLobbyHeaderTheme ThemeColors;

    // Main Components - using exact blueprint naming pattern
    UPROPERTY(meta = (BindWidget))
    class UBorder* EntryBorder;

    UPROPERTY(meta = (BindWidget))
    class UHorizontalBox* ContentContainer;

    // Index Section (empty container)
    UPROPERTY(meta = (BindWidget))
    class USizeBox* IndexContainer;

    // Title Section
    UPROPERTY(meta = (BindWidget))
    class UVerticalBox* VerticalBox_0;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* TitleText;

    // Header Text Blocks
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* EntryText;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* TimeText;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* SpotsText;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* TrackText;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* StatusText;

    // Button Section (empty container)
    UPROPERTY(meta = (BindWidget))
    class USizeBox* ButtonContainer;

    // Spacers - using exact blueprint names
    UPROPERTY(meta = (BindWidget))
    class USpacer* PostIndexSpacer;

    UPROPERTY(meta = (BindWidget))
    class USpacer* TitleBottomSpacer;

    UPROPERTY(meta = (BindWidget))
    class USpacer* PostTitleSpacer;

    UPROPERTY(meta = (BindWidget))
    class USpacer* PostEntrySpacer;

    UPROPERTY(meta = (BindWidget))
    class USpacer* PostTimeSpacer;

    UPROPERTY(meta = (BindWidget))
    class USpacer* PreSpotsSpacer;

    UPROPERTY(meta = (BindWidget))
    class USpacer* PostTrackSpacer;

    UPROPERTY(meta = (BindWidget))
    class USpacer* PostStatusSpacer;

    // Setup Functions
    void SetupLayout();
    void SetupHeaderText();
};