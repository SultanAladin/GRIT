#include "AppIcon.h"
#include "AppGrid.h"
#include "ThemeUtil.h"

DEFINE_LOG_CATEGORY_STATIC(LogAppIcon, Log, All);

void UAppIcon::NativeConstruct()
{
    Super::NativeConstruct();
    
    CurrentScale = 1.0f;  // [scale] - Start at normal size
    TargetScale = 1.0f;   // [scale]
    ScaleSpeed = 8.0f;    // [s⁻¹] - Fast snap response
    bIsHovered = false;
    bIsPressed = false;
    
    BootstrapTheme();
}

void UAppIcon::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    DriveScale(InDeltaTime);
}

FReply UAppIcon::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) // Reason: Left click only
    {
        bIsPressed = true;
        TargetScale = 0.95f; // [scale] - Slight press compression
        return FReply::Handled();
    } // End if (Left button check)
    
    return FReply::Unhandled();
}

FReply UAppIcon::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsPressed) // Reason: Valid click release
    {
        bIsPressed = false;
        TargetScale = bIsHovered ? 1.1f : 1.0f; // [scale] - Return to hover or normal
        
        if (IsHovered()) // Reason: Full click completed
        {
            TriggerLaunch();
        } // End if (IsHovered check)
        
        return FReply::Handled();
    } // End if (Left button check)
    
    return FReply::Unhandled();
}

void UAppIcon::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
    
    bIsHovered = true;
    TargetScale = 1.1f; // [scale] - Expand on hover
    
    if (AppBorder) // Reason: Apply hilight color
    {
        AppBorder->SetBrushColor(HilightColor);
    } // End if (AppBorder check)
}

void UAppIcon::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);
    
    bIsHovered = false;
    bIsPressed = false;
    TargetScale = 1.0f; // [scale] - Return to normal
    
    if (AppBorder) // Reason: Restore base color
    {
        AppBorder->SetBrushColor(BaseColor);
    } // End if (AppBorder check)
}

void UAppIcon::ConfigureApp(const FAppConfig& Config)
{
    AppConfig = Config;
    AppID = Config.AppID;
    
    if (AppText) // Reason: Set label text
    {
        AppText->SetText(Config.AppLabel);
    } // End if (AppText check)
    
    if (AppImage && Config.AppIcon) // Reason: Set icon texture
    {
        AppImage->SetBrushFromTexture(Config.AppIcon);
    } // End if (AppImage check)
}

//------------------------------------------------------------------------------
// internal mechanics
//------------------------------------------------------------------------------

void UAppIcon::BootstrapTheme()
{
    FPalette Palette = UThemeUtil::FetchPalette(this);
    FBorderSpec Border = UThemeUtil::FetchBorderSpec(this);
    FTypeScale Type = UThemeUtil::FetchTypeScale(this);
    
    BaseColor = Palette.SurfaceShift;
    HilightColor = UThemeUtil::BlendOverlay(BaseColor, Palette.StateHover);
    
    if (AppBorder) // Reason: Configure border visual
    {
        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
        Brush.TintColor = FSlateColor(FLinearColor::White);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        Brush.OutlineSettings.CornerRadii = FVector4(Border.RadiusSnug, Border.RadiusSnug, Border.RadiusSnug, Border.RadiusSnug);
        AppBorder->SetBrush(Brush);
        AppBorder->SetBrushColor(BaseColor);
    } // End if (AppBorder check)
    
    if (AppText) // Reason: Configure text style
    {
        UThemeUtil::ApplyTypeSpec(AppText, Type.BodyS);
    } // End if (AppText check)
}

void UAppIcon::DriveScale(float DeltaTime)
{
    if (FMath::Abs(CurrentScale - TargetScale) > 0.001f) // Reason: Animation active
    {
        CurrentScale = FMath::FInterpTo(CurrentScale, TargetScale, DeltaTime, ScaleSpeed);
        SetRenderScale(FVector2D(CurrentScale, CurrentScale));
    } // End if (Scale interpolation check)
}

void UAppIcon::TriggerLaunch()
{
    UE_LOG(LogAppIcon, Log, TEXT("TriggerLaunch: AppID=%d launched"), AppID);
    
    if (ParentGrid) // Reason: Request spawn from parent
    {
        FGeometry IconGeometry = GetCachedGeometry();
        FVector2D IconCenter = IconGeometry.GetAbsolutePositionAtCoordinates(FVector2D(0.5f, 0.5f));
        FVector2D IconSize = IconGeometry.GetAbsoluteSize();
        
        ParentGrid->SpawnAppPage(AppConfig, IconCenter, IconSize);
    } // End if (ParentGrid check)
}
