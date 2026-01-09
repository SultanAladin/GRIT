#include "AppGrid.h"
#include "AppIcon.h"
#include "AppPage.h"
#include "ThemeUtil.h"
#include "Components/UniformGridSlot.h"

DEFINE_LOG_CATEGORY_STATIC(LogAppGrid, Log, All);

void UAppGrid::NativeConstruct()
{
    Super::NativeConstruct();
    
    ColumnsPerRow = 4; // [int32] - Default 4x grid
    SelectedAppID = -1; // [int32] - No selection
    
    BootstrapTheme();
    
    if (AppConfigs.Num() > 0) // Reason: Auto-populate if configs exist
    {
        PopulateApps();
    } // End if (Configs exist check)
}

void UAppGrid::PopulateApps()
{
    if (!AppGridPanel) // Reason: Grid must exist
    {
        UE_LOG(LogAppGrid, Warning, TEXT("PopulateApps: AppGridPanel is null"));
        return;
    } // End if (AppGridPanel check)
    
    if (!AppIconClass) // Reason: Widget class required
    {
        UE_LOG(LogAppGrid, Warning, TEXT("PopulateApps: AppIconClass not set"));
        return;
    } // End if (AppIconClass check)
    
    PurgeApps();
    
    UE_LOG(LogAppGrid, Log, TEXT("PopulateApps: Populating %d apps"), AppConfigs.Num());
    
    for (int32 Index = 0; Index < AppConfigs.Num(); ++Index) // Reason: Iterate all configs
    {
        FAppConfig Config = AppConfigs[Index];
        Config.AppID = Index; // [int32] - Auto-assign ID from array position
        
        UAppIcon* AppWidget = CreateWidget<UAppIcon>(this, AppIconClass);
        
        if (AppWidget) // Reason: Widget created successfully
        {
            AppWidget->ConfigureApp(Config);
            AppWidget->AssignParentGrid(this);
            
            int32 Row = Index / ColumnsPerRow;    // [int32] - Grid row position
            int32 Column = Index % ColumnsPerRow; // [int32] - Grid column position
            
            UUniformGridSlot* GridSlot = AppGridPanel->AddChildToUniformGrid(AppWidget, Row, Column);
            
            if (GridSlot) // Reason: Apply slot padding
            {
                GridSlot->SetHorizontalAlignment(HAlign_Center);
                GridSlot->SetVerticalAlignment(VAlign_Center);
            } // End if (GridSlot check)
            
            UE_LOG(LogAppGrid, Log, TEXT("PopulateApps: Added app ID=%d at [%d,%d]"), Config.AppID, Row, Column);
        } // End if (AppWidget check)
        else
        {
            UE_LOG(LogAppGrid, Warning, TEXT("PopulateApps: Failed to create widget for app ID=%d"), Config.AppID);
        }
    } // End for (AppConfigs iteration)
}

void UAppGrid::PurgeApps()
{
    if (AppGridPanel) // Reason: Clear all children
    {
        AppGridPanel->ClearChildren();
        UE_LOG(LogAppGrid, Log, TEXT("PurgeApps: Cleared all apps"));
    } // End if (AppGridPanel check)
}

//------------------------------------------------------------------------------
// theme application
//------------------------------------------------------------------------------

void UAppGrid::BootstrapTheme()
{
    FPalette Palette = UThemeUtil::FetchPalette(this);
    FBorderSpec Border = UThemeUtil::FetchBorderSpec(this);
    
    if (GridBorder) // Reason: Configure container border
    {
        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
        Brush.TintColor = FSlateColor(Palette.SurfacePrime);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        Brush.OutlineSettings.CornerRadii = FVector4(Border.RadiusLoose, Border.RadiusLoose, Border.RadiusLoose, Border.RadiusLoose);
        GridBorder->SetBrush(Brush);
        GridBorder->SetBrushColor(Palette.SurfacePrime);
    } // End if (GridBorder check)
}

//------------------------------------------------------------------------------
// app page management
//------------------------------------------------------------------------------

void UAppGrid::SpawnAppPage(const FAppConfig& Config, FVector2D IconCenter, FVector2D IconSize)
{
    if (!Config.PageClass) // Reason: Page class required
    {
        UE_LOG(LogAppGrid, Warning, TEXT("SpawnAppPage: No PageClass defined for AppID=%d"), Config.AppID);
        return;
    } // End if (PageClass check)
    
    if (OpenApps.Contains(Config.AppID)) // Reason: App already open
    {
        UE_LOG(LogAppGrid, Warning, TEXT("SpawnAppPage: AppID=%d already open"), Config.AppID);
        SelectedAppID = Config.AppID;
        return;
    } // End if (Already open check)
    
    UAppPage* Page = CreateWidget<UAppPage>(this, Config.PageClass);
    
    if (Page) // Reason: Page created successfully
    {
        Page->ConfigurePage(Config.AppID);
        Page->OnClosed.AddDynamic(this, &UAppGrid::HandleAppClosed);
        
        FGeometry GridGeometry = GetCachedGeometry();
        FVector2D GridSize = GridGeometry.GetAbsoluteSize(); // [px] - Full grid size
        
        Page->AddToViewport(100); // [ZOrder] - Above all other elements
        Page->DriveSpawnAnimation(FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, GridSize, 0.0f);
        
        OpenApps.Add(Config.AppID, Page);
        SelectedAppID = Config.AppID;
        
        UE_LOG(LogAppGrid, Log, TEXT("SpawnAppPage: AppID=%d spawned, scaling from (0,0) to (%.1f,%.1f)"), Config.AppID, GridSize.X, GridSize.Y);
    } // End if (Page check)
    else
    {
        UE_LOG(LogAppGrid, Error, TEXT("SpawnAppPage: Failed to create page for AppID=%d"), Config.AppID);
    }
}

void UAppGrid::CloseApp(int32 AppID)
{
    if (OpenApps.Contains(AppID)) // Reason: App exists in open list
    {
        UAppPage* Page = OpenApps[AppID];
        
        if (Page) // Reason: Valid page reference
        {
            Page->RequestClose();
        } // End if (Page check)
        
        OpenApps.Remove(AppID);
        
        if (SelectedAppID == AppID) // Reason: Was selected app
        {
            SelectedAppID = -1;
        } // End if (Selected check)
        
        UE_LOG(LogAppGrid, Log, TEXT("CloseApp: AppID=%d closed"), AppID);
    } // End if (App open check)
}

bool UAppGrid::IsAppOpen(int32 AppID) const
{
    return OpenApps.Contains(AppID);
}

void UAppGrid::HandleAppClosed(int32 AppID)
{
    if (OpenApps.Contains(AppID)) // Reason: Remove from tracking
    {
        OpenApps.Remove(AppID);
        
        if (SelectedAppID == AppID) // Reason: Clear selection
        {
            SelectedAppID = -1;
        } // End if (Selected check)
        
        UE_LOG(LogAppGrid, Log, TEXT("HandleAppClosed: AppID=%d removed from tracking"), AppID);
    } // End if (Contains check)
}
