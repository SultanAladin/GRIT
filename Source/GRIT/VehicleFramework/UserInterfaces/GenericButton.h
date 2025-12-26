// GenericButton.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "GenericButton.generated.h"

//------------------------------------------------------------------------------
// Corner rounding presets
//------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECornerStyle : uint8
{
    None        UMETA(DisplayName = "None (Sharp)"),
    Slight      UMETA(DisplayName = "Slight"),
    Medium      UMETA(DisplayName = "Medium"),
    Rounded     UMETA(DisplayName = "Rounded"),
    Pill        UMETA(DisplayName = "Pill (Fully Rounded)")
};

//------------------------------------------------------------------------------
// Easing types for scale animation
//------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EScaleEasing : uint8
{
    Linear      UMETA(DisplayName = "Linear"),
    EaseIn      UMETA(DisplayName = "Ease In"),
    EaseOut     UMETA(DisplayName = "Ease Out"),
    EaseInOut   UMETA(DisplayName = "Ease In-Out"),
    Cubic       UMETA(DisplayName = "Cubic")
};

//------------------------------------------------------------------------------
// Easing types for rotation animation
//------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ERotationEasing : uint8
{
    Linear      UMETA(DisplayName = "Linear"),
    EaseIn      UMETA(DisplayName = "Ease In"),
    EaseOut     UMETA(DisplayName = "Ease Out"),
    EaseInOut   UMETA(DisplayName = "Ease In-Out"),
    Cubic       UMETA(DisplayName = "Cubic")
};

//------------------------------------------------------------------------------
// Tooltip spawn position
//------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ETooltipPosition : uint8
{
    Left        UMETA(DisplayName = "Left"),
    Right       UMETA(DisplayName = "Right"),
    Above       UMETA(DisplayName = "Above"),
    Below       UMETA(DisplayName = "Below"),
    AtCursor    UMETA(DisplayName = "At Cursor")
};

UCLASS(BlueprintType, Blueprintable)
class GRIT_API UGenericButton : public UUserWidget
{
    GENERATED_BODY()

public:
    UGenericButton(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    //------------------------------------------------------------------------------
    // Widget bindings
    //------------------------------------------------------------------------------

    UPROPERTY(meta = (BindWidget))
    class UButton* MainButton;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* ButtonText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UImage* ButtonImage;

    //------------------------------------------------------------------------------
    // Feature toggles
    //------------------------------------------------------------------------------

    /** Enable scale animation on hover */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
    bool bEnableScaleOnHover = true;

    /** Enable tooltip display on hover */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
    bool bEnableTooltip = false;

    /** Enable image rotation on hover */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
    bool bEnableImageRotation = false;

    //------------------------------------------------------------------------------
    // Scale settings
    //------------------------------------------------------------------------------

    /** Scale multiplier when hovered [dimensionless] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", meta = (EditCondition = "bEnableScaleOnHover"))
    float HoverScale = 1.25f;

    /** Scale animation duration [s] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", meta = (EditCondition = "bEnableScaleOnHover"))
    float ScaleAnimationDuration = 0.2f;

    /** Scale animation easing type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", meta = (EditCondition = "bEnableScaleOnHover"))
    EScaleEasing ScaleEasingType = EScaleEasing::EaseOut;

    //------------------------------------------------------------------------------
    // Tooltip settings
    //------------------------------------------------------------------------------

    /** Tooltip widget class to spawn */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tooltip", meta = (EditCondition = "bEnableTooltip"))
    TSubclassOf<class UUserWidget> TooltipWidgetClass;

    /** Tooltip spawn position relative to button */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tooltip", meta = (EditCondition = "bEnableTooltip"))
    ETooltipPosition TooltipPosition = ETooltipPosition::Right;

    /** Offset from computed position [px] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tooltip", meta = (EditCondition = "bEnableTooltip"))
    FVector2D TooltipOffset = FVector2D(10.0f, 0.0f);

    //------------------------------------------------------------------------------
    // Image rotation settings
    //------------------------------------------------------------------------------

    /** Target rotation angle on hover [deg] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Image Rotation", meta = (EditCondition = "bEnableImageRotation"))
    float HoverRotationAngle = 90.0f;

    /** Rotation animation duration [s] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Image Rotation", meta = (EditCondition = "bEnableImageRotation"))
    float RotationAnimationDuration = 0.2f;

    /** Rotation animation easing type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Image Rotation", meta = (EditCondition = "bEnableImageRotation"))
    ERotationEasing RotationEasingType = ERotationEasing::EaseOut;

    //------------------------------------------------------------------------------
    // Theme settings
    //------------------------------------------------------------------------------

    /** Border outline color - normal state [RGBA] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor OutlineColorNormal = FLinearColor::Gray;

    /** Border outline color - hovered state [RGBA] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor OutlineColorHovered = FLinearColor(0.231f, 0.510f, 0.965f, 1.0f);

    /** Border outline color - pressed state [RGBA] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor OutlineColorPressed = FLinearColor(0.15f, 0.35f, 0.75f, 1.0f);

    /** Border outline width [px] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    float OutlineWidth = 2.0f;

    /** Background color - normal state [RGBA] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor BackgroundColorNormal = FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);

    /** Background color - hovered state [RGBA] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor BackgroundColorHovered = FLinearColor(0.15f, 0.15f, 0.15f, 1.0f);

    /** Background color - pressed state [RGBA] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    FLinearColor BackgroundColorPressed = FLinearColor(0.05f, 0.05f, 0.05f, 1.0f);

    /** Corner rounding style */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
    ECornerStyle CornerStyle = ECornerStyle::Medium;

    //------------------------------------------------------------------------------
    // Color transition
    //------------------------------------------------------------------------------

    /** Enable smooth color interpolation between states */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Transition")
    bool bEnableColorTransition = false;

    /** Color transition duration [s] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Transition", meta = (EditCondition = "bEnableColorTransition"))
    float ColorTransitionDuration = 0.15f;

    //------------------------------------------------------------------------------
    // Content settings
    //------------------------------------------------------------------------------

    /** Button label text */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    FText LabelText;

    /** Button icon texture */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    UTexture2D* IconTexture;

private:
    //------------------------------------------------------------------------------
    // Event handlers
    //------------------------------------------------------------------------------

    UFUNCTION()
    void OnButtonHovered();

    UFUNCTION()
    void OnButtonUnhovered();

    UFUNCTION()
    void OnButtonPressed();

    UFUNCTION()
    void OnButtonReleased();

    UFUNCTION()
    void OnButtonClicked();

    //------------------------------------------------------------------------------
    // Animation state
    //------------------------------------------------------------------------------

    FVector2D OriginalScale;
    FVector2D StartScale;
    FVector2D TargetScale;
    float ScaleAnimProgress = 0.0f;

    float StartRotation = 0.0f;
    float TargetRotation = 0.0f;
    float RotationAnimProgress = 0.0f;

    bool bIsHovered = false;
    bool bIsPressed = false;

    //------------------------------------------------------------------------------
    // Color transition state
    //------------------------------------------------------------------------------

    float ColorAnimProgress = 0.0f;
    FLinearColor StartOutlineColor;
    FLinearColor TargetOutlineColor;
    FLinearColor StartBackgroundColor;
    FLinearColor TargetBackgroundColor;

    //------------------------------------------------------------------------------
    // Tooltip state
    //------------------------------------------------------------------------------

    UPROPERTY()
    class UUserWidget* SpawnedTooltip;

    FTimerHandle TooltipDelayTimer;

    //------------------------------------------------------------------------------
    // Animation timers
    //------------------------------------------------------------------------------

    FTimerHandle ScaleAnimTimer;
    FTimerHandle RotationAnimTimer;
    FTimerHandle ColorAnimTimer;

    //------------------------------------------------------------------------------
    // Helper functions
    //------------------------------------------------------------------------------

    void ApplyButtonStyling();
    void ApplyButtonColors();
    void ApplyContent();
    void SpawnTooltip();
    void RemoveTooltip();
    void ShowTooltipDelayed();
    void UpdateTooltipPosition();
    FVector2D ComputeTooltipPosition() const;
    float ApplyScaleEasing(float Alpha) const;
    float ApplyRotationEasing(float Alpha) const;
    float GetCornerRadius() const;
    void StartColorTransition();
    void GetTargetColors(FLinearColor& OutOutlineColor, FLinearColor& OutBackgroundColor) const;

    //------------------------------------------------------------------------------
    // Animation step callbacks
    //------------------------------------------------------------------------------

    void ScaleAnimationStep();
    void RotationAnimationStep();
    void ColorAnimationStep();

public:
    //------------------------------------------------------------------------------
    // Events
    //------------------------------------------------------------------------------

    UFUNCTION(BlueprintImplementableEvent, Category = "Button Events")
    void OnButtonClickedBP();

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnButtonClicked);

    UPROPERTY(BlueprintAssignable, Category = "Button Events")
    FOnButtonClicked OnButtonClickedDelegate;

    //------------------------------------------------------------------------------
    // Public API
    //------------------------------------------------------------------------------

    /** Set button label text at runtime */
    UFUNCTION(BlueprintCallable, Category = "Content")
    void SetLabelText(const FText& NewText);

    /** Set button icon at runtime */
    UFUNCTION(BlueprintCallable, Category = "Content")
    void SetIcon(UTexture2D* NewTexture);
};
