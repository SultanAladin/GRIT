#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Engine/Engine.h"
#include "CyberButtonWidget.generated.h"

// Theme/Accent helper struct for consistent styling across widgets
USTRUCT(BlueprintType)
struct GRIT_API FCyberThemeData
{
    GENERATED_BODY()

    // Primary accent colors
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Primary Colors")
    FLinearColor AccentPrimary = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f); // White

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Primary Colors")
    FLinearColor AccentSecondary = FLinearColor(0.8f, 0.8f, 0.8f, 1.0f); // Light Grey

    // Background colors
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Background Colors")
    FLinearColor BackgroundDark = FLinearColor(0.067f, 0.067f, 0.067f, 1.0f); // #111111

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Background Colors")
    FLinearColor BackgroundCard = FLinearColor(0.102f, 0.102f, 0.102f, 1.0f); // #1a1a1a

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Background Colors")
    FLinearColor BackgroundHover = FLinearColor(0.145f, 0.145f, 0.145f, 1.0f); // #252525

    // Text colors
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text Colors")
    FLinearColor TextPrimary = FLinearColor(0.867f, 0.867f, 0.867f, 1.0f); // #dddddd

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text Colors")
    FLinearColor TextMuted = FLinearColor(0.533f, 0.533f, 0.533f, 1.0f); // #888888

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text Colors")
    FLinearColor TextOnAccent = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f); // Dark text for accent backgrounds

    // Border and outline settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border Settings")
    FLinearColor BorderNormal = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f); // #333333

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border Settings")
    FLinearColor BorderHover = FLinearColor(0.333f, 0.333f, 0.333f, 1.0f); // #555555

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Border Settings")
    float BorderRadius = 12.0f;

    // Animation settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings")
    float AnimationSpeed = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings")
    float HoverScale = 1.05f;

    // Shadow/glow settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    FLinearColor GlowColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.2f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    float GlowIntensity = 0.3f;

    // Default constructor
    FCyberThemeData()
    {
        // All values already initialized above
    }
};

// Button style variant enum for different use cases
UENUM(BlueprintType)
enum class ECyberButtonVariant : uint8
{
    Default     UMETA(DisplayName = "Default"),
    Primary     UMETA(DisplayName = "Primary"),
    Secondary   UMETA(DisplayName = "Secondary"),
    Outline     UMETA(DisplayName = "Outline"),
    Ghost       UMETA(DisplayName = "Ghost"),
    Chip        UMETA(DisplayName = "Chip")
};

// Size variants
UENUM(BlueprintType)
enum class ECyberButtonSize : uint8
{
    Small       UMETA(DisplayName = "Small"),
    Medium      UMETA(DisplayName = "Medium"),
    Large       UMETA(DisplayName = "Large"),
    ExtraLarge  UMETA(DisplayName = "Extra Large")
};

UCLASS(BlueprintType, Blueprintable)
class GRIT_API UCyberButtonWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UCyberButtonWidget(const FObjectInitializer& ObjectInitializer);

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

    UPROPERTY(meta = (BindWidget))
    class UBorder* ButtonBorder;

public:
    // Theme and styling
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cyber Theme")
    FCyberThemeData ThemeData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Style")
    ECyberButtonVariant ButtonVariant = ECyberButtonVariant::Default;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Style")
    ECyberButtonSize ButtonSize = ECyberButtonSize::Medium;

    // Content settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    FText ButtonTextContent = FText::FromString(TEXT("Button"));

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    bool bIsSelected = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    bool bIsCyberEnabled = true;

    // Animation settings (can override theme defaults)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float CustomHoverScale = 0.0f; // 0 = use theme default

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float CustomAnimationDuration = 0.0f; // 0 = use theme default

    // Interactive state settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    bool bShowHoverEffect = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    bool bShowClickFeedback = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    bool bShowGlowEffect = true;

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

    // Glow animation state
    float GlowAnimationTime;
    bool bGlowAnimating;

    // Helper functions
    void ApplyButtonVariantStyle();
    void UpdateHoverState(bool bHovered);
    void UpdateSelectedState();
    void UpdateEnabledState();
    void ApplyThemeToComponents();
    FVector2D GetSizeMultiplierForButtonSize() const;
    FMargin GetPaddingForButtonSize() const;
    float GetFontSizeForButtonSize() const;

public:
    // Blueprint events
    UFUNCTION(BlueprintImplementableEvent, Category = "Cyber Button Events")
    void OnCyberButtonClicked();

    UFUNCTION(BlueprintImplementableEvent, Category = "Cyber Button Events")
    void OnCyberButtonHovered();

    UFUNCTION(BlueprintImplementableEvent, Category = "Cyber Button Events")
    void OnCyberButtonUnhovered();

    UFUNCTION(BlueprintImplementableEvent, Category = "Cyber Button Events")
    void OnCyberButtonSelectionChanged(bool bNewSelected);

    // C++ delegates
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCyberButtonClicked);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCyberButtonSelectionChanged, bool, bNewSelected);
    
    UPROPERTY(BlueprintAssignable, Category = "Cyber Button Events")
    FOnCyberButtonClicked OnCyberButtonClickedDelegate;

    UPROPERTY(BlueprintAssignable, Category = "Cyber Button Events")
    FOnCyberButtonSelectionChanged OnCyberButtonSelectionChangedDelegate;

    // Public interface functions
    UFUNCTION(BlueprintCallable, Category = "Cyber Button")
    void SetButtonText(const FText& NewText);

    UFUNCTION(BlueprintCallable, Category = "Cyber Button")
    void SetSelected(bool bNewSelected);

    UFUNCTION(BlueprintCallable, Category = "Cyber Button")
    void SetEnabled(bool bNewEnabled);

    UFUNCTION(BlueprintCallable, Category = "Cyber Button")
    void SetTheme(const FCyberThemeData& NewTheme);

    UFUNCTION(BlueprintCallable, Category = "Cyber Button")
    void SetVariant(ECyberButtonVariant NewVariant);

    UFUNCTION(BlueprintCallable, Category = "Cyber Button")
    void SetSize(ECyberButtonSize NewSize);

    UFUNCTION(BlueprintCallable, Category = "Cyber Button")
    bool IsSelected() const { return bIsSelected; }

    UFUNCTION(BlueprintCallable, Category = "Cyber Button")
    ECyberButtonVariant GetVariant() const { return ButtonVariant; }

    UFUNCTION(BlueprintCallable, Category = "Cyber Button")
    FText GetButtonText() const { return ButtonTextContent; }

    // Static function to create consistent theme across app
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cyber Theme")
    static FCyberThemeData GetDefaultCyberTheme();
};