#include "DropListSlide.h"
#include "ThemeUtil.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogDropListSlide, Log, All);

UDropListSlide::UDropListSlide(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , bUseSlideAnimation(true)
    , SlideDistance(5.0f)
    , StartSlideOffset(0.0f)
    , TargetSlideOffset(0.0f)
{
}

void UDropListSlide::NativeConstruct()
{
    Super::NativeConstruct();
    
    if (bUseSlideAnimation) // Reason: Initialize slide transform
    {
        InitContentTransform();
    } // End if (Slide animation check)
}

//------------------------------------------------------------------------------
// slide animation initialization
//------------------------------------------------------------------------------

void UDropListSlide::InitContentTransform()
{
    if (!ContentBorder) // Reason: Null check
    {
        return;
    } // End if (ContentBorder check)

    ContentBorder->SetRenderTransformPivot(FVector2D(0.5f, 0.0f)); // [Pivot] - Top center origin
    
    FWidgetTransform InitialTransform;
    InitialTransform.Translation = FVector2D(0.0f, -SlideDistance); // [px] - Start behind header
    InitialTransform.Scale = FVector2D(1.0f, 1.0f);
    InitialTransform.Shear = FVector2D(0.0f, 0.0f);
    InitialTransform.Angle = 0.0f;
    ContentBorder->SetRenderTransform(InitialTransform);
    
    UE_LOG(LogDropListSlide, Log, TEXT("InitContentTransform: Slide distance=%.1f"), SlideDistance);
}

//------------------------------------------------------------------------------
// overridden animation methods
//------------------------------------------------------------------------------

void UDropListSlide::StartExpand()
{
    if (!ContentSizeBox) // Reason: Null check
    {
        return;
    } // End if (ContentSizeBox check)

    TargetHeight = ComputeTargetHeight();
    StartHeight = ContentSizeBox->GetHeightOverride();
    AnimTime = 0.0f;
    bIsAnimating = true;

    if (bUseSlideAnimation) // Reason: Setup slide animation
    {
        StartSlideOffset = -SlideDistance;
        TargetSlideOffset = 0.0f;
    } // End if (Slide animation check)

    UE_LOG(LogDropListSlide, Log, TEXT("StartExpand: Height %.1f→%.1f, Slide %.1f→%.1f"), StartHeight, TargetHeight, StartSlideOffset, TargetSlideOffset);

    GetWorld()->GetTimerManager().SetTimer(AnimTimer, this, &UDropListSlide::TickAnim, 0.016f, true); // [16ms] - ~60fps update rate

    if (bAnimChevron) // Reason: Animate chevron down
    {
        AnimateChevronTo(180.0f);
    } // End if (Chevron animation)
}

void UDropListSlide::StartCollapse()
{
    if (!ContentSizeBox) // Reason: Null check
    {
        return;
    } // End if (ContentSizeBox check)

    StartHeight = ContentSizeBox->GetHeightOverride();
    TargetHeight = 0.0f;
    AnimTime = 0.0f;
    bIsAnimating = true;

    if (bUseSlideAnimation) // Reason: Setup slide animation
    {
        StartSlideOffset = 0.0f;
        TargetSlideOffset = -SlideDistance;
    } // End if (Slide animation check)

    UE_LOG(LogDropListSlide, Log, TEXT("StartCollapse: Height %.1f→%.1f, Slide %.1f→%.1f"), StartHeight, TargetHeight, StartSlideOffset, TargetSlideOffset);

    GetWorld()->GetTimerManager().SetTimer(AnimTimer, this, &UDropListSlide::TickAnim, 0.016f, true); // [16ms] - ~60fps update rate

    if (bAnimChevron) // Reason: Animate chevron up
    {
        AnimateChevronTo(0.0f);
    } // End if (Chevron animation)
}

void UDropListSlide::TickAnim()
{
    if (!bIsAnimating || !ContentSizeBox) // Reason: Animation active check
    {
        return;
    } // End if (Animation check)

    AnimTime += 0.016f; // [s] - 16ms frame time
    float Progress = FMath::Clamp(AnimTime / AnimDuration, 0.0f, 1.0f);

    float CurrentHeight = UAnimUtil::LerpCurved(StartHeight, TargetHeight, Progress, AnimCurve);
    ContentSizeBox->SetHeightOverride(CurrentHeight);

    if (bUseSlideAnimation) // Reason: Apply slide animation
    {
        float CurrentSlideOffset = UAnimUtil::LerpCurved(StartSlideOffset, TargetSlideOffset, Progress, AnimCurve);
        ApplySlideTransform(CurrentSlideOffset);
    } // End if (Slide animation check)

    if (Progress >= 1.0f) // Reason: Animation complete
    {
        bIsAnimating = false;
        GetWorld()->GetTimerManager().ClearTimer(AnimTimer);
        UE_LOG(LogDropListSlide, Log, TEXT("Animation complete at height %.1f"), CurrentHeight);
    } // End if (Complete check)
}

void UDropListSlide::ApplySlideTransform(float YOffset)
{
    if (!ContentBorder) // Reason: Null check
    {
        return;
    } // End if (ContentBorder check)

    FWidgetTransform SlideTransform;
    SlideTransform.Translation = FVector2D(0.0f, YOffset); // [px] - Vertical offset
    SlideTransform.Scale = FVector2D(1.0f, 1.0f);
    SlideTransform.Shear = FVector2D(0.0f, 0.0f);
    SlideTransform.Angle = 0.0f;
    ContentBorder->SetRenderTransform(SlideTransform);
}
