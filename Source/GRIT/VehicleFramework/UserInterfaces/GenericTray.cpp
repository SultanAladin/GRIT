#include "GenericTray.h"
#include "Components/VerticalBox.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBoxSlot.h"
#include "Components/SlateWrapperTypes.h"

/*====================================================================================================================================
                                                         INITIALIZATION
======================================================================================================================================*/

void UGenericTray::NativeConstruct()
{
    Super::NativeConstruct();

    // Reason: Enable tick for animation updates
    SetIsFocusable(false);

    // Reason: Wire button click to flip handler
    if (TriggerButton)
    {
        TriggerButton->OnClicked.AddDynamic(this, &UGenericTray::OnTriggerPressed);
    } // End if (TriggerButton check)

    //------------------------------------------------------------------------------
    // Initial Collapsed State
    //------------------------------------------------------------------------------
    // Reason: Start with reasonable button-sized dimensions so button is visible
    if (ContentBox)
    {
        // Use reasonable defaults until first measurement
        float InitialWidth = 100.0f + Margin.X;   // [px] - Default button width + margin
        float InitialHeight = 40.0f + Margin.Y;   // [px] - Default button height + margin
        
        ContentBox->SetHeightOverride(InitialHeight);
        ContentBox->SetWidthOverride(InitialWidth);
        StartHeight = InitialHeight;
        TargetAnimHeight = InitialHeight;
        StartWidth = InitialWidth;
        TargetAnimWidth = InitialWidth;
    } // End if (ContentBox check)
}

void UGenericTray::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Reason: Update animation if active
    if (bAnimating)
    {
        DriveMotion(InDeltaTime);
    } // End if (bAnimating check)
}

/*====================================================================================================================================
                                                         PANEL CONTROL
======================================================================================================================================*/

void UGenericTray::FlipPanel()
{
    bIsOpen = !bIsOpen;
    bAnimating = true;
    AnimTime = 0.0f;

    //------------------------------------------------------------------------------
    // Set Animation Range
    //------------------------------------------------------------------------------
    // Reason: Store current dimensions as start, calculate targets based on state
    if (ContentBox)
    {
        StartHeight = ContentBox->GetHeightOverride();                          // [px] - Current height
        StartWidth = ContentBox->GetWidthOverride();                            // [px] - Current width
        
        TargetAnimHeight = bIsOpen ? CachedContentHeight : CachedButtonHeight;  // [px] - Expand or collapse height
        TargetAnimWidth = bIsOpen ? CachedContentWidth : CachedButtonWidth;     // [px] - Expand or collapse width
        
        UE_LOG(LogTemp, Warning, TEXT("[FlipPanel] State: %s | Target: %.2fx%.2f px"), 
               bIsOpen ? TEXT("OPEN") : TEXT("CLOSED"), TargetAnimWidth, TargetAnimHeight);
    } // End if (ContentBox check)

    OnTrayToggled.Broadcast(bIsOpen);
}

void UGenericTray::OnTriggerPressed()
{
    //------------------------------------------------------------------------------
    // Measure Button Dimensions (Collapsed State)
    //------------------------------------------------------------------------------
    // Reason: Capture actual button size at runtime + add custom margin
    if (TriggerButton && CachedButtonHeight < UE_SMALL_NUMBER)
    {
        FVector2D ButtonSize = TriggerButton->GetDesiredSize();     // [px] - Measured button size
        
        if (ButtonSize.Y < UE_SMALL_NUMBER)
        {
            UE_LOG(LogTemp, Warning, TEXT("[OnTriggerPressed] Button size invalid (%.2f px) - Using fallback"), ButtonSize.Y);
            CachedButtonHeight = 40.0f + Margin.Y;                  // [px] - Fallback + margin
            CachedButtonWidth = 100.0f + Margin.X;                  // [px] - Fallback + margin
        } // End if (invalid size check)
        else
        {
            CachedButtonHeight = ButtonSize.Y + Margin.Y;           // [px] - Button height + vertical margin
            CachedButtonWidth = ButtonSize.X + Margin.X;            // [px] - Button width + horizontal margin
            UE_LOG(LogTemp, Warning, TEXT("[OnTriggerPressed] Button Size: %.2fx%.2f px + Margin: %.2fx%.2f px = %.2fx%.2f px"), 
                   ButtonSize.X, ButtonSize.Y, Margin.X, Margin.Y, CachedButtonWidth, CachedButtonHeight);
        } // End else (valid size)
    } // End if (TriggerButton check)

    //------------------------------------------------------------------------------
    // Measure Full Content Dimensions (Expanded State)
    //------------------------------------------------------------------------------
    // Reason: Capture actual content size at runtime
    if (Border && CachedContentHeight < UE_SMALL_NUMBER)
    {
        FVector2D OverlaySize = Border->GetDesiredSize();           // [px] - Measured overlay size
        
        if (OverlaySize.Y < UE_SMALL_NUMBER)
        {
            UE_LOG(LogTemp, Warning, TEXT("[OnTriggerPressed] Border size invalid (%.2f px) - Using UPROPERTY target"), OverlaySize.Y);
            CachedContentHeight = TargetPanelHeight;                // [px] - Fallback to explicit target
            CachedContentWidth = TargetWidth;                       // [px] - Fallback to explicit target
        } // End if (invalid size check)
        else
        {
            CachedContentHeight = OverlaySize.Y;                    // [px] - Use measured size
            CachedContentWidth = OverlaySize.X;                     // [px] - Use measured size
            UE_LOG(LogTemp, Warning, TEXT("[OnTriggerPressed] Border Size: %.2fx%.2f px"), CachedContentWidth, CachedContentHeight);
        } // End else (valid size)
    } // End if (Border check)

    FlipPanel();
}

/*====================================================================================================================================
                                                         ANIMATION PIPELINE
======================================================================================================================================*/

void UGenericTray::DriveMotion(float DeltaTime)
{
    AnimTime += DeltaTime;                                                    // [s] - Accumulate time
    float Progress = FMath::Clamp(AnimTime / AnimDuration, 0.0f, 1.0f);       // [0-1] - Normalized progress
    float Eased = BlendCurve(Progress);                                       // [0-1] - Smoothed curve

    //------------------------------------------------------------------------------
    // Interpolate Dimensions
    //------------------------------------------------------------------------------
    float CurrentHeight = FMath::Lerp(StartHeight, TargetAnimHeight, Eased); // [px] - Smooth height blend
    float CurrentWidth = FMath::Lerp(StartWidth, TargetAnimWidth, Eased);    // [px] - Smooth width blend

    // Reason: Update SizeBox dimensions to match animation
    if (ContentBox)
    {
        ContentBox->SetHeightOverride(CurrentHeight);
        ContentBox->SetWidthOverride(CurrentWidth);
    } // End if (ContentBox check)

    //------------------------------------------------------------------------------
    // End Animation
    //------------------------------------------------------------------------------
    // Reason: Animation complete when progress reaches 1.0
    if (Progress >= 1.0f)
    {
        bAnimating = false;

        // Reason: Snap to exact targets to avoid float precision drift
        if (ContentBox)
        {
            ContentBox->SetHeightOverride(TargetAnimHeight);
            ContentBox->SetWidthOverride(TargetAnimWidth);
        } // End if (ContentBox check)
    } // End if (progress complete check)
}

/*====================================================================================================================================
                                                         EASING MATH
======================================================================================================================================*/

float UGenericTray::BlendCurve(float t) const
{
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - FMath::Pow(-2.0f * t + 2.0f, 3.0f) / 2.0f; // Cubic ease in/out
}