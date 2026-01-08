#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "HoverButtonV2.generated.h"

UCLASS(BlueprintType, Blueprintable)
class GRIT_API UHoverButtonV2 : public UUserWidget
{
    GENERATED_BODY()

public:
    UHoverButtonV2(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // Button component
    UPROPERTY(meta = (BindWidget))
    class UButton* MainButton;

    // Text component for button label
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* ButtonText;

    // Hover scale settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Settings")
    float HoverScale = 1.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Settings")
    float AnimationDuration = 0.25f;

    // Text settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text Settings")
    FText ButtonTextContent = FText::FromString(TEXT("Button"));

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text Settings")
    FLinearColor NormalTextColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text Settings")
    FLinearColor HoveredTextColor = FLinearColor::White;

    // Theme color settings - optimized for navbar
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Settings")
    FLinearColor NormalBackgroundColor = FLinearColor::Transparent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Settings")
    FLinearColor HoveredBackgroundColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.2f); // Light grey with transparency

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Settings")
    FLinearColor NormalOutlineColor = FLinearColor::Transparent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Settings")
    FLinearColor HoveredOutlineColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Settings")
    float OutlineWidth = 3.0f; // Increased from 1.5f to 3.0f for thicker outline

    // Border settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border Settings")
    FLinearColor NormalBorderColor = FLinearColor::Transparent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border Settings")
    FLinearColor HoveredBorderColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.8f); // White border with slight transparency

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border Settings")
    float BorderThickness = 2.0f;

    // Corner radius for rounded buttons
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Settings")
    float CornerRadius = 8.0f;

private:
    // Event handlers - using UFUNCTION for proper delegate binding
    UFUNCTION()
    void OnButtonHovered();

    UFUNCTION()
    void OnButtonUnhovered();

    UFUNCTION()
    void OnButtonClicked();

    // Store original scale for restoration
    FVector2D OriginalScale;

    // Animation state
    bool bIsHovered;
    float CurrentAnimationTime;
    FVector2D StartScale;
    FVector2D TargetScale;

    // Helper functions
    void UpdateButtonStyle(bool bHovered);
    void UpdateTextStyle(bool bHovered);

public:
    // Blueprint event for button click
    UFUNCTION(BlueprintImplementableEvent, Category = "Button Events")
    void OnButtonClickedBP();

    // C++ delegate for button click
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnButtonClicked);
    
    UPROPERTY(BlueprintAssignable, Category = "Button Events")
    FOnButtonClicked OnButtonClickedDelegate;

    // Function to set button text from Blueprint
    UFUNCTION(BlueprintCallable, Category = "Text Settings")
    void SetButtonText(const FText& NewText);
};