#include "InteractiveButtonA1.h"
#include "Components/Button.h"
#include "Components/ScaleBox.h"
#include "Engine/World.h"
#include "TimerManager.h"

UInteractiveButtonA1::UInteractiveButtonA1(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , HoverScale(1.1f)
    , AnimationSpeed(0.15f)
    , bUseEaseInOut(true)
    , CachedWorld(nullptr)
    , LastAppliedScale(1.0f)
{
    // Widget doesn't need to tick since we use timers
}

void UInteractiveButtonA1::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    // Reset scale in editor
    if (ScaleContainer)
    {
        ScaleContainer->SetUserSpecifiedScale(1.0f);
        LastAppliedScale = 1.0f;
    }
}

void UInteractiveButtonA1::NativeConstruct()
{
    Super::NativeConstruct();

    // Cache world reference for performance
    CachedWorld = GetWorld();

    // Bind button events
    if (MainButton)
    {
        MainButton->OnHovered.AddDynamic(this, &UInteractiveButtonA1::OnButtonHovered);
        MainButton->OnUnhovered.AddDynamic(this, &UInteractiveButtonA1::OnButtonUnhovered);
        MainButton->OnClicked.AddDynamic(this, &UInteractiveButtonA1::OnButtonPressed);
    }

    // Initialize animation state
    ScaleAnim = FScaleAnimation();
    ScaleAnim.Duration = AnimationSpeed;
    ScaleAnim.StartScale = 1.0f;
    ScaleAnim.TargetScale = 1.0f;

    // Ensure initial scale is set
    SetScale(1.0f);
}

void UInteractiveButtonA1::NativeDestruct()
{
    // Clean up timer to prevent memory leaks
    if (CachedWorld && AnimationTimerHandle.IsValid())
    {
        CachedWorld->GetTimerManager().ClearTimer(AnimationTimerHandle);
    }

    Super::NativeDestruct();
}

void UInteractiveButtonA1::OnButtonHovered()
{
    if (!IsButtonEnabled()) return;

    ScaleAnim.bIsHovering = true;
    StartScaleAnimation(HoverScale);
}

void UInteractiveButtonA1::OnButtonUnhovered()
{
    if (!IsButtonEnabled()) return;

    ScaleAnim.bIsHovering = false;
    StartScaleAnimation(1.0f);
}

void UInteractiveButtonA1::OnButtonPressed()
{
    if (!IsButtonEnabled()) return;

    // Broadcast the click event
    OnButtonClicked.Broadcast();
}

void UInteractiveButtonA1::StartScaleAnimation(float TargetScale)
{
    if (!ScaleContainer || !CachedWorld) return;

    // Performance optimization: Skip animation if target is very close to current
    if (FMath::Abs(TargetScale - LastAppliedScale) < SCALE_EPSILON)
    {
        return;
    }

    // Set up animation parameters
    ScaleAnim.StartScale = LastAppliedScale;
    ScaleAnim.TargetScale = TargetScale;
    ScaleAnim.CurrentTime = 0.0f;
    ScaleAnim.Duration = AnimationSpeed;
    ScaleAnim.bIsAnimating = true;

    // Clear existing timer
    if (AnimationTimerHandle.IsValid())
    {
        CachedWorld->GetTimerManager().ClearTimer(AnimationTimerHandle);
    }

    // Start animation timer - using small interval for smooth animation
    // 60 FPS update rate (0.0166f) for smooth scaling
    CachedWorld->GetTimerManager().SetTimer(
        AnimationTimerHandle,
        this,
        &UInteractiveButtonA1::UpdateScaleAnimation,
        0.0166f, // ~60 FPS
        true
    );
}

void UInteractiveButtonA1::UpdateScaleAnimation()
{
    if (!ScaleAnim.bIsAnimating || !CachedWorld) return;

    ScaleAnim.CurrentTime += 0.0166f; // Match timer interval

    // Calculate animation progress (0.0 to 1.0)
    float Progress = FMath::Clamp(ScaleAnim.CurrentTime / ScaleAnim.Duration, 0.0f, 1.0f);
    
    // Apply easing if enabled
    float EasedProgress = EaseInOut(Progress);
    
    // Interpolate scale value
    float CurrentScale = FMath::Lerp(ScaleAnim.StartScale, ScaleAnim.TargetScale, EasedProgress);
    
    // Apply the scale
    SetScale(CurrentScale);

    // Check if animation is complete
    if (Progress >= 1.0f)
    {
        ScaleAnim.bIsAnimating = false;
        SetScale(ScaleAnim.TargetScale); // Ensure exact final value
        
        // Clear timer
        if (AnimationTimerHandle.IsValid())
        {
            CachedWorld->GetTimerManager().ClearTimer(AnimationTimerHandle);
        }
    }
}

void UInteractiveButtonA1::SetScale(float Scale)
{
    if (!ScaleContainer) return;

    // Performance optimization: only update if scale changed significantly
    if (FMath::Abs(Scale - LastAppliedScale) < SCALE_EPSILON) return;

    ScaleContainer->SetUserSpecifiedScale(Scale);
    LastAppliedScale = Scale;
}

void UInteractiveButtonA1::SetButtonEnabled(bool bEnabled)
{
    if (MainButton)
    {
        MainButton->SetIsEnabled(bEnabled);
        
        // If disabling while hovering, reset scale immediately
        if (!bEnabled && ScaleAnim.bIsHovering)
        {
            if (CachedWorld && AnimationTimerHandle.IsValid())
            {
                CachedWorld->GetTimerManager().ClearTimer(AnimationTimerHandle);
            }
            ScaleAnim.bIsAnimating = false;
            SetScale(1.0f);
        }
    }
}

bool UInteractiveButtonA1::IsButtonEnabled() const
{
    return MainButton ? MainButton->GetIsEnabled() : false;
}

void UInteractiveButtonA1::SetHoverScale(float NewScale)
{
    HoverScale = FMath::Clamp(NewScale, 1.0f, 2.0f);
    
    // If currently hovering, update the animation target
    if (ScaleAnim.bIsHovering && ScaleAnim.bIsAnimating)
    {
        StartScaleAnimation(HoverScale);
    }
}