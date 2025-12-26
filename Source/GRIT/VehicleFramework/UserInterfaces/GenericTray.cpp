#include "GenericTray.h"
#include "Components/VerticalBox.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBoxSlot.h"
#include "Components/SlateWrapperTypes.h"

// FORCE REBUILD: Fixed all BlendCurve issues - 2024-12-24 v3

/*====================================================================================================================================
                                                         INITIALIZATION
======================================================================================================================================*/

void UGenericTray::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    // FIXED: Apply animation settings from PreConstruct to override Blueprint defaults
    UE_LOG(LogTemp, Warning, TEXT("[GenericTray] NativePreConstruct - AnimDuration=%.2fs, Curve=%d, AutoSizeButton=%s, AutoSizeContent=%s"), 
           AnimDuration, (int32)AnimationCurve, 
           bAutoSizeFromButton ? TEXT("TRUE") : TEXT("FALSE"),
           bAutoSizeFromContent ? TEXT("TRUE") : TEXT("FALSE"));
}

void UGenericTray::NativeConstruct()
{
    Super::NativeConstruct();

    // Reason: Wire button click to flip handler
    if (TriggerButton)
    {
        TriggerButton->OnClicked.AddDynamic(this, &UGenericTray::OnTriggerPressed);
    }

    // FIXED: Apply safe fallback size immediately to prevent invisible button
    ApplyCollapsedSize();
    
    // FIXED: Force Slate layout prepass to get real widget dimensions
    UE_LOG(LogTemp, Warning, TEXT("[GenericTray] NativeConstruct - Forcing SlatePrepass for dimension caching"));
    TakeWidget()->SlatePrepass();
    
    // Cache dimensions now that layout is complete
    CacheDimensions();
    
    // Apply the cached dimensions
    if (bDimensionsCached && ContentBox)
    {
        ContentBox->SetHeightOverride(CachedButtonHeight);
        ContentBox->SetWidthOverride(CachedButtonWidth);
        UE_LOG(LogTemp, Warning, TEXT("[GenericTray] ✅ Applied cached dimensions: %.1fx%.1f px"), CachedButtonWidth, CachedButtonHeight);
    }
}

void UGenericTray::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Reason: Update animation if active
    if (bAnimating)
    {
        DriveMotion(InDeltaTime);
    }
}

/*====================================================================================================================================
                                                         PANEL CONTROL
======================================================================================================================================*/

void UGenericTray::FlipPanel()
{
    // FIXED: Cache dimensions before first animation
    if (!bDimensionsCached)
    {
        CacheDimensions();
    }

    bIsOpen = !bIsOpen;
    bAnimating = true;
    AnimTime = 0.0f;

    //------------------------------------------------------------------------------
    // Set Animation Range
    //------------------------------------------------------------------------------
    if (ContentBox)
    {
        StartHeight = ContentBox->GetHeightOverride();
        StartWidth = ContentBox->GetWidthOverride();
        
        TargetAnimHeight = bIsOpen ? CachedContentHeight : CachedButtonHeight;
        TargetAnimWidth = bIsOpen ? CachedContentWidth : CachedButtonWidth;
        
        UE_LOG(LogTemp, Warning, TEXT("[FlipPanel] State: %s | Start: %.1fx%.1f -> Target: %.1fx%.1f px | Cached: %s"), 
               bIsOpen ? TEXT("OPEN") : TEXT("CLOSED"), 
               StartWidth, StartHeight, TargetAnimWidth, TargetAnimHeight,
               bDimensionsCached ? TEXT("YES") : TEXT("NO"));
    }

    OnTrayToggled.Broadcast(bIsOpen);
}

void UGenericTray::OnTriggerPressed()
{
    // FIXED: Ensure dimensions are cached before animation
    if (!bDimensionsCached)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GenericTray] OnTriggerPressed - Dimensions not cached, forcing SlatePrepass"));
        TakeWidget()->SlatePrepass();
        CacheDimensions();
        
        // Apply dimensions if we got them
        if (bDimensionsCached && ContentBox)
        {
            ContentBox->SetHeightOverride(CachedButtonHeight);
            ContentBox->SetWidthOverride(CachedButtonWidth);
        }
    }

    FlipPanel();
}

/*====================================================================================================================================
                                                         ANIMATION PIPELINE (FIXED: EFlowCurve support)
======================================================================================================================================*/

void UGenericTray::DriveMotion(float DeltaTime)
{
    AnimTime += DeltaTime;
    float Progress = FMath::Clamp(AnimTime / AnimDuration, 0.0f, 1.0f);
    
    // FIXED: Use UIToolkit EFlowCurve instead of hardcoded cubic curve
    float Eased = UUIToolkit::EvalFlowCurve(AnimationCurve, Progress);

    //------------------------------------------------------------------------------
    // Interpolate Dimensions
    //------------------------------------------------------------------------------
    float CurrentHeight = FMath::Lerp(StartHeight, TargetAnimHeight, Eased);
    float CurrentWidth = FMath::Lerp(StartWidth, TargetAnimWidth, Eased);

    if (ContentBox)
    {
        ContentBox->SetHeightOverride(CurrentHeight);
        ContentBox->SetWidthOverride(CurrentWidth);
    }

    //------------------------------------------------------------------------------
    // End Animation
    //------------------------------------------------------------------------------
    if (Progress >= 1.0f)
    {
        bAnimating = false;

        if (ContentBox)
        {
            ContentBox->SetHeightOverride(TargetAnimHeight);
            ContentBox->SetWidthOverride(TargetAnimWidth);
        }
        
        UE_LOG(LogTemp, Log, TEXT("[GenericTray] Animation complete - Final size: %.1fx%.1f px"), TargetAnimWidth, TargetAnimHeight);
    }
}

/*====================================================================================================================================
                                                         SIZING HELPERS (FIXED: Proper dimension caching)
======================================================================================================================================*/

void UGenericTray::CacheDimensions()
{
    UE_LOG(LogTemp, Warning, TEXT("[GenericTray] CacheDimensions - Starting dimension measurement"));

    bool bButtonSizeValid = false;
    bool bContentSizeValid = false;

    //------------------------------------------------------------------------------
    // Measure Button Dimensions (Collapsed State)
    //------------------------------------------------------------------------------
    if (TriggerButton && bAutoSizeFromButton)
    {
        FVector2D ButtonSize = TriggerButton->GetDesiredSize();
        
        if (ButtonSize.Y > 1.0f && ButtonSize.X > 1.0f)
        {
            CachedButtonHeight = ButtonSize.Y + Margin.Y;
            CachedButtonWidth = ButtonSize.X + Margin.X;
            bButtonSizeValid = true;
            UE_LOG(LogTemp, Warning, TEXT("[CacheDimensions] ✅ Button Size: %.1fx%.1f px + Margin: %.1fx%.1f px = %.1fx%.1f px"), 
                   ButtonSize.X, ButtonSize.Y, Margin.X, Margin.Y, CachedButtonWidth, CachedButtonHeight);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[CacheDimensions] ❌ Button size invalid (%.1fx%.1f px) - using manual sizing"), ButtonSize.X, ButtonSize.Y);
        }
    }
    
    if (!bButtonSizeValid)
    {
        CachedButtonHeight = 40.0f + Margin.Y;
        CachedButtonWidth = 100.0f + Margin.X;
        UE_LOG(LogTemp, Warning, TEXT("[CacheDimensions] Using manual button size: %.1fx%.1f px"), CachedButtonWidth, CachedButtonHeight);
        bButtonSizeValid = true;
    }

    //------------------------------------------------------------------------------
    // Measure Full Content Dimensions (Expanded State)
    //------------------------------------------------------------------------------
    if (Border && bAutoSizeFromContent)
    {
        FVector2D ContentSize = Border->GetDesiredSize();
        
        if (ContentSize.Y > 1.0f && ContentSize.X > 1.0f)
        {
            CachedContentHeight = ContentSize.Y;
            CachedContentWidth = ContentSize.X;
            bContentSizeValid = true;
            UE_LOG(LogTemp, Warning, TEXT("[CacheDimensions] ✅ Content Size: %.1fx%.1f px (auto-detected)"), CachedContentWidth, CachedContentHeight);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[CacheDimensions] ❌ Content size invalid (%.1fx%.1f px) - using manual sizing"), ContentSize.X, ContentSize.Y);
        }
    }
    
    if (!bContentSizeValid)
    {
        CachedContentHeight = TargetPanelHeight;
        CachedContentWidth = TargetWidth;
        bContentSizeValid = true;
        UE_LOG(LogTemp, Warning, TEXT("[CacheDimensions] Using manual content size: %.1fx%.1f px"), CachedContentWidth, CachedContentHeight);
    }

    // Mark as cached
    bDimensionsCached = true;
    UE_LOG(LogTemp, Warning, TEXT("[GenericTray] ✅ ALL DIMENSIONS CACHED - Button: %.1fx%.1f, Content: %.1fx%.1f"), 
           CachedButtonWidth, CachedButtonHeight, CachedContentWidth, CachedContentHeight);
}



void UGenericTray::ApplyCollapsedSize()
{
    if (!ContentBox) return;

    // FIXED: Always start with reasonable fallback to prevent 0x0 invisible button
    float InitialWidth = 120.0f;   // [px] - Safe default button width
    float InitialHeight = 48.0f;   // [px] - Safe default button height

    // Try to get actual button size if available
    if (bAutoSizeFromButton && TriggerButton)
    {
        FVector2D ButtonSize = TriggerButton->GetDesiredSize();
        if (ButtonSize.Y > 1.0f && ButtonSize.X > 1.0f)  // Check for valid size
        {
            InitialWidth = ButtonSize.X + Margin.X;
            InitialHeight = ButtonSize.Y + Margin.Y;
            
            UE_LOG(LogTemp, Warning, TEXT("[ApplyCollapsedSize] ✅ Applied button-based size: %.1fx%.1f px"), InitialWidth, InitialHeight);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[ApplyCollapsedSize] ❌ Button size invalid (%.1fx%.1f px) - using safe fallback"), ButtonSize.X, ButtonSize.Y);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ApplyCollapsedSize] Using manual fallback size: %.1fx%.1f px"), InitialWidth, InitialHeight);
    }
    
    // ALWAYS apply a valid size to prevent invisible button
    ContentBox->SetHeightOverride(InitialHeight);
    ContentBox->SetWidthOverride(InitialWidth);
    
    UE_LOG(LogTemp, Warning, TEXT("[ApplyCollapsedSize] ✅ APPLIED INITIAL SIZE: %.1fx%.1f px (prevents 0x0 issue)"), InitialWidth, InitialHeight);
}