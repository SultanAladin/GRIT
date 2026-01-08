#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "AppPage.generated.h"

/*====================================================================================================================================
                                                         APPLICATION PAGE BASE
======================================================================================================================================*/

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAppClosed, int32, AppID);

/** Base class for all application pages */
UCLASS()
class GRIT_API UAppPage : public UUserWidget
{
    GENERATED_BODY()

public:
    //------------------------------------------------------------------------------
    // widget bindings
    //------------------------------------------------------------------------------
    
    UPROPERTY(meta = (BindWidget))
    UBorder* PageBorder; // [UBorder*] - Page container

    //------------------------------------------------------------------------------
    // events
    //------------------------------------------------------------------------------
    
    UPROPERTY(BlueprintAssignable, Category = "AppPage")
    FOnAppClosed OnClosed; // [Delegate] - Close broadcast

    /** Configure page with app ID */
    void ConfigurePage(int32 InAppID);

    /** Retrieve app identifier */
    int32 FetchAppID() const { return AppID; }

    /** Trigger page close */
    UFUNCTION(BlueprintCallable, Category = "AppPage")
    void RequestClose();

    /** Drive spawn animation from position to target */
    void DriveSpawnAnimation(FVector2D FromPosition, FVector2D FromSize, FVector2D ToPosition, FVector2D ToSize, float DeltaTime);

    /** Check if spawn animation complete */
    bool IsSpawnComplete() const { return bSpawnComplete; }

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    int32 AppID; // [int32] - Assigned app identifier
    
    bool bSpawnComplete; // [bool] - Spawn animation finished
    float SpawnProgress; // [0,1] - Spawn animation progress
    float SpawnSpeed; // [s⁻¹] - Animation rate
    
    FVector2D CurrentSize; // [px] - Active dimensions
    FVector2D TargetSize; // [px] - Desired dimensions
    FVector2D StartSize; // [px] - Spawn origin size

    /** Bootstrap theme styling */
    void BootstrapTheme();
};
