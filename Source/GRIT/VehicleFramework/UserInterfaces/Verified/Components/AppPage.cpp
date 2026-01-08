#include "AppPage.h"
#include "ThemeUtil.h"
#include "AnimUtil.h"

DEFINE_LOG_CATEGORY_STATIC(LogAppPage, Log, All);

void UAppPage::NativeConstruct()
{
    Super::NativeConstruct();
    
    bSpawnComplete = false;
    SpawnProgress = 0.0f;
    SpawnSpeed = 5.0f; // [s⁻¹] - Fast spawn animation
    
    BootstrapTheme();
}

void UAppPage::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    
    if (!bSpawnComplete) // Reason: Animate spawn transition
    {
        SpawnProgress = FMath::Clamp(SpawnProgress + (InDeltaTime * SpawnSpeed), 0.0f, 1.0f);
        
        float Eased = UAnimUtil::ComputeCurve(SpawnProgress, EMotionCurve::CubicOut);
        
        CurrentSize = FMath::Lerp(StartSize, TargetSize, Eased);
        
        float ScaleX = TargetSize.X > 0.0f ? (CurrentSize.X / TargetSize.X) : 1.0f; // [scale] - Prevent divide by zero
        float ScaleY = TargetSize.Y > 0.0f ? (CurrentSize.Y / TargetSize.Y) : 1.0f; // [scale] - Prevent divide by zero
        
        SetRenderScale(FVector2D(ScaleX, ScaleY));
        
        if (SpawnProgress >= 1.0f) // Reason: Animation finished
        {
            bSpawnComplete = true;
            SetRenderScale(FVector2D(1.0f, 1.0f));
            UE_LOG(LogAppPage, Log, TEXT("Spawn animation complete for AppID=%d"), AppID);
        } // End if (Progress complete check)
    } // End if (Spawn animation check)
}

void UAppPage::ConfigurePage(int32 InAppID)
{
    AppID = InAppID;
    UE_LOG(LogAppPage, Log, TEXT("ConfigurePage: AppID=%d configured"), AppID);
}

void UAppPage::RequestClose()
{
    UE_LOG(LogAppPage, Log, TEXT("RequestClose: AppID=%d closing"), AppID);
    OnClosed.Broadcast(AppID);
    RemoveFromParent();
}

void UAppPage::DriveSpawnAnimation(FVector2D FromPosition, FVector2D FromSize, FVector2D ToPosition, FVector2D ToSize, float DeltaTime)
{
    StartSize = FromSize;
    CurrentSize = FromSize;
    TargetSize = ToSize;
    
    SpawnProgress = 0.0f;
    bSpawnComplete = false;
    
    UE_LOG(LogAppPage, Log, TEXT("DriveSpawnAnimation: Size=(%.1f,%.1f) -> (%.1f,%.1f)"), FromSize.X, FromSize.Y, ToSize.X, ToSize.Y);
}

//------------------------------------------------------------------------------
// theme application
//------------------------------------------------------------------------------

void UAppPage::BootstrapTheme()
{
    FPalette Palette = UThemeUtil::FetchPalette(this);
    FBorderSpec Border = UThemeUtil::FetchBorderSpec(this);
    
    if (PageBorder) // Reason: Configure page border
    {
        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
        Brush.TintColor = FSlateColor(Palette.SurfaceRaised);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        Brush.OutlineSettings.CornerRadii = FVector4(Border.RadiusLoose, Border.RadiusLoose, Border.RadiusLoose, Border.RadiusLoose);
        if (Border.ThicknessBase > 0.0f) { Brush.OutlineSettings.Width = Border.ThicknessBase; Brush.OutlineSettings.Color = FSlateColor(Palette.LineShift); }
        PageBorder->SetBrush(Brush);
        PageBorder->SetBrushColor(Palette.SurfaceRaised);
    } // End if (PageBorder check)
}
