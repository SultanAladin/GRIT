#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "UIThemeData.h"
#include "PulseButtonWidget.generated.h"



UCLASS(BlueprintType, Blueprintable)
class GRIT_API UPulseButtonWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPulseButtonWidget(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual void NativePreConstruct() override;

    // Core UI components
    UPROPERTY(meta = (BindWidget))
    class UButton* MainButton;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* ButtonText;

public:
    // Theme and styling
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Theme")
    FUIThemeData ThemeData;

    // Content settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    FText ButtonTextContent = FText::FromString(TEXT("Button"));

private:
    // Event handlers
    UFUNCTION()
    void OnButtonHovered();

    UFUNCTION()
    void OnButtonUnhovered();

    UFUNCTION()
    void OnButtonClicked();

    // Animation state
    bool bIsHovered;
    bool bIsPressed;
    float CurrentAnimationTime;
    FVector2D OriginalScale;
    FVector2D StartScale;
    FVector2D TargetScale;

    // Helper functions
    void ApplyButtonStyle();
    void UpdateHoverState(bool bHovered);
    void UpdateEnabledState();
    void ApplyThemeToComponents();

public:
    // Blueprint events
    UFUNCTION(BlueprintImplementableEvent, Category = "Pulse Button Events")
    void OnPulseButtonClicked();

    UFUNCTION(BlueprintImplementableEvent, Category = "Pulse Button Events")
    void OnPulseButtonHovered();

    UFUNCTION(BlueprintImplementableEvent, Category = "Pulse Button Events")
    void OnPulseButtonUnhovered();

    // C++ delegates
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPulseButtonClicked);
    
    UPROPERTY(BlueprintAssignable, Category = "Pulse Button Events")
    FOnPulseButtonClicked OnPulseButtonClickedDelegate;

    // Public interface functions
    UFUNCTION(BlueprintCallable, Category = "Pulse Button")
    void SetButtonText(const FText& NewText);

    UFUNCTION(BlueprintCallable, Category = "Pulse Button")
    void SetEnabled(bool bNewEnabled);

    UFUNCTION(BlueprintCallable, Category = "Pulse Button")
    void SetTheme(const FUIThemeData& NewTheme);

    UFUNCTION(BlueprintCallable, Category = "Pulse Button")
    FText GetButtonText() const { return ButtonTextContent; }

    // Static function to create consistent theme across app
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "UI Theme")
    static FUIThemeData GetDefaultUITheme();
};