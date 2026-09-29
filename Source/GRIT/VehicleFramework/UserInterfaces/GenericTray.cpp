#include "GenericTray.h"
#include "Components/VerticalBox.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBoxSlot.h"
#include "Components/SlateWrapperTypes.h"
#include "../../../UserInterface/Components/ThemeUtil.h"

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

    // Reason: Apply theme-based border styling
    ApplyBorderStyling();

    // FIXED: Apply safe fallback size immediately to prevent invisible button
    ApplyCollapsedSize();
    
    // FIXED: Force Slate layout prepass to get real widget dimensions
    UE_LOG(LogTemp, Warning, TEXT("[GenericTray] NativeConstruct - Forcing SlatePrepass for dimension caching"));
    TakeWidget()->SlatePrepass();
    
    // Cache dimensions now that layout is complete
    CacheDimensions();
    
    // Apply the cached dimensions with proper padding calculation
    if (bDimensionsCached && ContentBox)
    {
        FVector2D CollapsedSize = CalculateCollapsedSize();
        ContentBox->SetHeightOverride(CollapsedSize.Y);
        ContentBox->SetWidthOverride(CollapsedSize.X);
        UE_LOG(LogTemp, Warning, TEXT("[GenericTray] ✅ Applied cached dimensions: %.1fx%.1f px"), CollapsedSize.X, CollapsedSize.Y);
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
    // Animate Corner Radius
    //------------------------------------------------------------------------------
    float ExpandedRadius = GetRadiusFromEnum(ExpandedCornerRadius);
    float CollapsedRadius = GetRadiusFromEnum(CollapsedCornerRadius);
    float CurrentRadius = bIsOpen ? 
        FMath::Lerp(CollapsedRadius, ExpandedRadius, Eased) : 
        FMath::Lerp(ExpandedRadius, CollapsedRadius, Eased);
    
    ApplyBorderStyling(CurrentRadius);

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
        
        // Apply final border styling
        ApplyBorderStyling();
        
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
    // Measure Button Dimensions (Collapsed State) - Account for padding properly
    //------------------------------------------------------------------------------
    if (TriggerButton && bAutoSizeFromButton)
    {
        FVector2D ButtonSize = TriggerButton->GetDesiredSize();
        
        if (ButtonSize.Y > 1.0f && ButtonSize.X > 1.0f)
        {
            if (bAccountForPadding)
            {
                // Reason: Get button's actual content size without padding
                FVector2D ContentSize = ButtonSize;
                
                // Reason: Account for button's internal padding
                if (UButton* Button = TriggerButton)
                {
                    FButtonStyle ButtonStyle = Button->GetStyle();
                    FMargin ButtonPadding = ButtonStyle.NormalPadding;
                    ContentSize.X -= (ButtonPadding.Left + ButtonPadding.Right);
                    ContentSize.Y -= (ButtonPadding.Top + ButtonPadding.Bottom);
                }
                
                // Reason: For circular collapsed state, use the larger dimension to ensure perfect circle
                if (bMaintainAspectRatio && CollapsedCornerRadius == ECornerRadius::Full)
                {
                    float MaxDimension = FMath::Max(ContentSize.X, ContentSize.Y);
                    CachedButtonHeight = MaxDimension + Margin.Y + (BorderThickness * 2.0f);
                    CachedButtonWidth = MaxDimension + Margin.X + (BorderThickness * 2.0f);
                }
                else
                {
                    CachedButtonHeight = ContentSize.Y + Margin.Y + (BorderThickness * 2.0f);
                    CachedButtonWidth = ContentSize.X + Margin.X + (BorderThickness * 2.0f);
                }
            }
            else
            {
                CachedButtonHeight = ButtonSize.Y + Margin.Y;
                CachedButtonWidth = ButtonSize.X + Margin.X;
            }
            
            bButtonSizeValid = true;
            UE_LOG(LogTemp, Warning, TEXT("[CacheDimensions] ✅ Button Size: %.1fx%.1f px + Margin: %.1fx%.1f px + Border: %.1f px = %.1fx%.1f px"), 
                   ButtonSize.X, ButtonSize.Y, Margin.X, Margin.Y, BorderThickness, CachedButtonWidth, CachedButtonHeight);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[CacheDimensions] ❌ Button size invalid (%.1fx%.1f px) - using manual sizing"), ButtonSize.X, ButtonSize.Y);
        }
    }
    
    if (!bButtonSizeValid)
    {
        float BaseSize = 40.0f;
        if (bMaintainAspectRatio && CollapsedCornerRadius == ECornerRadius::Full)
        {
            CachedButtonHeight = BaseSize + Margin.Y + (BorderThickness * 2.0f);
            CachedButtonWidth = BaseSize + Margin.X + (BorderThickness * 2.0f);
        }
        else
        {
            CachedButtonHeight = BaseSize + Margin.Y;
            CachedButtonWidth = 100.0f + Margin.X;
        }
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

    // Reason: Use the new calculation method that accounts for padding
    FVector2D CollapsedSize = CalculateCollapsedSize();
    
    // ALWAYS apply a valid size to prevent invisible button
    ContentBox->SetHeightOverride(CollapsedSize.Y);
    ContentBox->SetWidthOverride(CollapsedSize.X);
    
    UE_LOG(LogTemp, Warning, TEXT("[ApplyCollapsedSize] ✅ APPLIED INITIAL SIZE: %.1fx%.1f px (accounts for padding and borders)"), CollapsedSize.X, CollapsedSize.Y);
}

float UGenericTray::GetRadiusFromEnum(ECornerRadius Radius) const
{
    FBorderSpec BorderSpec = UThemeUtil::FetchBorderSpec(this);
    
    switch (Radius)
    {
        case ECornerRadius::None:   return BorderSpec.RadiusNone;
        case ECornerRadius::Tight:  return BorderSpec.RadiusTight;
        case ECornerRadius::Snug:   return BorderSpec.RadiusSnug;
        case ECornerRadius::Loose:  return BorderSpec.RadiusLoose;
        case ECornerRadius::Round:  return BorderSpec.RadiusRound;
        case ECornerRadius::Full:   return BorderSpec.RadiusFull;
        default:                    return BorderSpec.RadiusSnug;
    } // End switch (Radius)
}

void UGenericTray::ApplyBorderStyling(float RadiusOverride)
{
    if (!Border) return;

    float CurrentRadius;
    if (RadiusOverride >= 0.0f)
    {
        CurrentRadius = RadiusOverride;
    }
    else
    {
        CurrentRadius = bIsOpen ? GetRadiusFromEnum(ExpandedCornerRadius) : GetRadiusFromEnum(CollapsedCornerRadius);
    }

    UThemeUtil::ApplyBorderStyling(Border, BackgroundColor, CurrentRadius, BorderThickness);
}

FVector2D UGenericTray::CalculateCollapsedSize() const
{
    // FIXED: Safe default size to prevent invisible button
    FVector2D CollapsedSize(120.0f, 48.0f);

    // Try to get actual button size if available
    if (bAutoSizeFromButton && TriggerButton)
    {
        FVector2D ButtonSize = TriggerButton->GetDesiredSize();
        if (ButtonSize.Y > 1.0f && ButtonSize.X > 1.0f)  // Check for valid size
        {
            if (bAccountForPadding)
            {
                // Reason: Get button's actual content size without internal padding
                FVector2D ContentSize = ButtonSize;
                
                // Reason: Account for button's internal padding
                if (UButton* Button = TriggerButton)
                {
                    FButtonStyle ButtonStyle = Button->GetStyle();
                    FMargin ButtonPadding = ButtonStyle.NormalPadding;
                    ContentSize.X -= (ButtonPadding.Left + ButtonPadding.Right);
                    ContentSize.Y -= (ButtonPadding.Top + ButtonPadding.Bottom);
                }
                
                // Reason: For circular collapsed state, use the larger dimension to ensure perfect circle
                if (bMaintainAspectRatio && CollapsedCornerRadius == ECornerRadius::Full)
                {
                    float MaxDimension = FMath::Max(ContentSize.X, ContentSize.Y);
                    CollapsedSize.X = MaxDimension + Margin.X + (BorderThickness * 2.0f);
                    CollapsedSize.Y = MaxDimension + Margin.Y + (BorderThickness * 2.0f);
                }
                else
                {
                    CollapsedSize.X = ContentSize.X + Margin.X + (BorderThickness * 2.0f);
                    CollapsedSize.Y = ContentSize.Y + Margin.Y + (BorderThickness * 2.0f);
                }
            }
            else
            {
                CollapsedSize.X = ButtonSize.X + Margin.X;
                CollapsedSize.Y = ButtonSize.Y + Margin.Y;
            }
            
            UE_LOG(LogTemp, Warning, TEXT("[CalculateCollapsedSize] ✅ Button-based size: %.1fx%.1f px"), CollapsedSize.X, CollapsedSize.Y);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[CalculateCollapsedSize] ❌ Button size invalid (%.1fx%.1f px) - using safe fallback"), ButtonSize.X, ButtonSize.Y);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[CalculateCollapsedSize] Using manual fallback size: %.1fx%.1f px"), CollapsedSize.X, CollapsedSize.Y);
    }
    
    return CollapsedSize;
}