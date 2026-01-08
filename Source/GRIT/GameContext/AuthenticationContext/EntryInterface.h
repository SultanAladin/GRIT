//EntryInterface.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"
#include "EntryInterface.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEntryAnimationCompleteDelegate);

/**
 * Entry screen widget that displays after successful login
 * Shows username and optional description text
 */
UCLASS(BlueprintType, Blueprintable)
class GRIT_API UEntryInterface : public UUserWidget
{
    GENERATED_BODY()

public:
    UEntryInterface(const FObjectInitializer& ObjectInitializer);

    //------------------------------------------------------------------------------
    // Public API
    //------------------------------------------------------------------------------

    /** Set username to display */
    UFUNCTION(BlueprintCallable, Category = "Entry Interface")
    void SetUsername(const FString& Username);

    /** Set display text */
    UFUNCTION(BlueprintCallable, Category = "Entry Interface")
    void SetDisplayText(const FText& Text);

    /** Set description text */
    UFUNCTION(BlueprintCallable, Category = "Entry Interface")
    void SetDescriptionText(const FText& Text);

    /** Apply theme styling */
    UFUNCTION(BlueprintCallable, Category = "Entry Interface")
    void ApplyThemeStyle();

    /** Trigger entry animation sequence */
    UFUNCTION(BlueprintCallable, Category = "Entry Interface")
    void TriggerEntrySequence();

    //------------------------------------------------------------------------------
    // Delegates
    //------------------------------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEntryAnimationCompleteDelegate OnAnimationComplete;

    //------------------------------------------------------------------------------
    // Events
    //------------------------------------------------------------------------------

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnEntryInitialized();

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnEntryAnimationComplete();

protected:
    virtual void NativeConstruct() override;

    //------------------------------------------------------------------------------
    // Widget bindings
    //------------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UOverlay* EntryOverlay;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* UsernameText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* DisplayText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* DescriptionText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetAnim), Transient)
    class UWidgetAnimation* EntrySlideIn;

    //------------------------------------------------------------------------------
    // Configuration
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry Config")
    FText DefaultDisplayText = FText::FromString("Welcome back");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry Config")
    FText DefaultDescriptionText = FText::FromString("Ready to play");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry Config")
    bool bUseHeaderFontForUsername = true;

private:
    //------------------------------------------------------------------------------
    // Runtime state
    //------------------------------------------------------------------------------

    UPROPERTY()
    class USessionAdapter* CachedSessionAdapter;

    FString CurrentUsername;
    bool bAnimationComplete = false;

    //------------------------------------------------------------------------------
    // Internal helpers
    //------------------------------------------------------------------------------

    class USessionAdapter* GetSessionAdapter();

    UFUNCTION()
    void ProcessAnimationComplete();
};
