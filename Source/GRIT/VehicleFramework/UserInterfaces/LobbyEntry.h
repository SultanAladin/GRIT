#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/BorderSlot.h"
#include "Components/SizeBoxSlot.h"
#include "Components/ButtonSlot.h"
#include "Engine/Font.h"
#include "Styling/SlateColor.h"
#include "LobbyDescriptor.h"
#include "LobbyEntry.generated.h"

USTRUCT(BlueprintType)
struct GRIT_API FLobbyEntryTheme
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor Primary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor Accent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor Text;

    FLobbyEntryTheme()
    {
        Primary = FLinearColor(0.005208f, 0.005208f, 0.005208f, 1.0f);  // Dark background
        Accent = FLinearColor(1.0f, 0.0f, 0.524113f, 1.0f);            // Pink/Red accent
        Text = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);                   // White text
    }
};

// Delegate for lobby action clicks
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLobbyActionClicked, int32, LobbyID, ELobbyActionType, ActionType);

UCLASS()
class GRIT_API ULobbyEntry : public UUserWidget
{
    GENERATED_BODY()

public:
    ULobbyEntry(const FObjectInitializer& ObjectInitializer);

    // Public methods
    UFUNCTION(BlueprintCallable, Category = "Lobby Entry")
    void SetLobbyData(const FLobbyData& InLobbyData, int32 InIndex, bool bInPlayerWasInLobby = false);

    // Action clicked delegate
    UPROPERTY(BlueprintAssignable, Category = "Lobby Entry")
    FOnLobbyActionClicked OnActionClicked;

protected:
    virtual void NativeConstruct() override;

private:
    // Theme Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme", meta = (AllowPrivateAccess = "true"))
    FLobbyEntryTheme ThemeColors;

    // Lobby data
    UPROPERTY()
    FLobbyData LobbyData;

    UPROPERTY()
    int32 LobbyIndex = 0;

    UPROPERTY()
    bool bPlayerWasInLobby = false;

    // Main Components - using exact blueprint names
    UPROPERTY(meta = (BindWidget))
    class UBorder* EntryBorder;

    UPROPERTY(meta = (BindWidget))
    class UHorizontalBox* ContentContainer;

    // Index Section
    UPROPERTY(meta = (BindWidget))
    class USizeBox* IndexContainer;

    UPROPERTY(meta = (BindWidget))
    class UBorder* IndexCircle;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* IndexNumber;

    // Title Section
    UPROPERTY(meta = (BindWidget))
    class UVerticalBox* VerticalBox_0;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* TitleText;

    UPROPERTY(meta = (BindWidget))
    class USizeBox* SizeBox_2;

    UPROPERTY(meta = (BindWidget))
    class UBorder* Border_55;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* ClassTrackText;

    // Data Text Blocks
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

    // Button Section
    UPROPERTY(meta = (BindWidget))
    class USizeBox* ButtonContainer;

    UPROPERTY(meta = (BindWidget))
    class UButton* Btn_Action;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* ActionButtonText;

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
    void UpdateLobbyDisplay();
    void UpdateActionButton();
    
    // Button callbacks
    UFUNCTION()
    void OnActionButtonClicked();
    
    // Helper functions
    ELobbyActionType DetermineActionType() const;
    FString GetActionButtonText(ELobbyActionType ActionType) const;
    FLinearColor GetStatusColor(ELobbyStatus Status) const;
};