#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Components/ScaleBox.h"
#include "Animation/UMGSequencePlayer.h"
#include "Animation/WidgetAnimation.h"
#include "InteractiveButtonA1.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractiveButtonClicked);

/**
 * High-performance interactive button with hover scaling animation
 * Optimized for use with multiple instances (30+)
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Interactive Button A1"))
class GRIT_API UInteractiveButtonA1 : public UUserWidget
{
    GENERATED_BODY()

public:
    UInteractiveButtonA1(const FObjectInitializer& ObjectInitializer);

    // Blueprint assignable event for button clicks
    UPROPERTY(BlueprintAssignable, Category = "Interactive Button")
    FOnInteractiveButtonClicked OnButtonClicked;

    // Configuration properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (ClampMin = "1.0", ClampMax = "2.0"))
    float HoverScale = 1.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (ClampMin = "0.05", ClampMax = "1.0"))
    float AnimationSpeed = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    bool bUseEaseInOut = true;

    // Public methods
    UFUNCTION(BlueprintCallable, Category = "Interactive Button")
    void SetButtonEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Interactive Button")
    bool IsButtonEnabled() const;

    UFUNCTION(BlueprintCallable, Category = "Interactive Button")
    void SetHoverScale(float NewScale);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Widget components
    UPROPERTY(meta = (BindWidget))
    class UButton* MainButton;

    UPROPERTY(meta = (BindWidget))
    class UScaleBox* ScaleContainer;

private:
    // Animation system
    struct FScaleAnimation
    {
        float StartScale;
        float TargetScale;
        float CurrentTime;
        float Duration;
        bool bIsAnimating;
        bool bIsHovering;

        FScaleAnimation() : StartScale(1.0f), TargetScale(1.0f), CurrentTime(0.0f), Duration(0.15f), bIsAnimating(false), bIsHovering(false) {}
    };

    FScaleAnimation ScaleAnim;
    FTimerHandle AnimationTimerHandle;

    // Cached world reference for timer management
    UWorld* CachedWorld;

    // Event handlers
    UFUNCTION()
    void OnButtonHovered();

    UFUNCTION()
    void OnButtonUnhovered();

    UFUNCTION()
    void OnButtonPressed();

    // Animation methods
    void StartScaleAnimation(float TargetScale);
    void UpdateScaleAnimation();
    void SetScale(float Scale);
    
    // Inline utility for easing
    FORCEINLINE float EaseInOut(float t) const
    {
        return bUseEaseInOut ? (t * t * (3.0f - 2.0f * t)) : t;
    }

    // Performance optimization: cache scale values to avoid redundant calls
    float LastAppliedScale;
    static constexpr float SCALE_EPSILON = 0.001f;
};