#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/UniformGridPanel.h"
#include "AppData.h"
#include "AppGrid.generated.h"

/*====================================================================================================================================
                                                         APPLICATION GRID CONTAINER
======================================================================================================================================*/

/** Grid container for app icons */
UCLASS()
class GRIT_API UAppGrid : public UUserWidget
{
    GENERATED_BODY()

public:
    //------------------------------------------------------------------------------
    // widget bindings
    //------------------------------------------------------------------------------
    
    UPROPERTY(meta = (BindWidget))
    UBorder* GridBorder; // [UBorder*] - Container border

    UPROPERTY(meta = (BindWidget))
    UUniformGridPanel* AppGridPanel; // [UUniformGridPanel*] - App layout grid

    //------------------------------------------------------------------------------
    // configuration
    //------------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AppGrid")
    TArray<FAppConfig> AppConfigs; // [TArray] - App configuration list

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AppGrid")
    TSubclassOf<class UAppIcon> AppIconClass; // [TSubclassOf] - Icon widget blueprint

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AppGrid")
    int32 ColumnsPerRow; // [int32] - Grid column count

    /** Populate grid with configured apps */
    UFUNCTION(BlueprintCallable, Category = "AppGrid")
    void PopulateApps();

    /** Purge all apps from grid */
    UFUNCTION(BlueprintCallable, Category = "AppGrid")
    void PurgeApps();

    /** Spawn app page from icon position */
    void SpawnAppPage(const FAppConfig& Config, FVector2D IconCenter, FVector2D IconSize);

    /** Close app page by ID */
    UFUNCTION(BlueprintCallable, Category = "AppGrid")
    void CloseApp(int32 AppID);

    /** Check if app is currently open */
    UFUNCTION(BlueprintPure, Category = "AppGrid")
    bool IsAppOpen(int32 AppID) const;

    /** Retrieve currently selected app ID */
    UFUNCTION(BlueprintPure, Category = "AppGrid")
    int32 FetchSelectedAppID() const { return SelectedAppID; }

protected:
    virtual void NativeConstruct() override;

private:
    //------------------------------------------------------------------------------
    // internal tracking
    //------------------------------------------------------------------------------
    
    UPROPERTY()
    TMap<int32, class UAppPage*> OpenApps; // [TMap] - Active app pages by ID
    
    int32 SelectedAppID; // [int32] - Currently focused app
    
    /** Bootstrap theme styling */
    void BootstrapTheme();

    /** Handle app page close event */
    UFUNCTION()
    void HandleAppClosed(int32 AppID);
};
