#pragma once

#include "VehicleSolver.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "VehicleConstruct.generated.h"

class USpringArmComponent;
class UCameraComponent;

//------------------------------------------------------------------------------
//                              VEHICLE MENU ENTRY STRUCT
//------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct GRIT_API FVehicleMenuEntry
{
    GENERATED_BODY()

    /** Widget class to spawn */
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UUserWidget> WidgetClass = nullptr;

    /** Socket name on vehicle mesh to attach to */
    UPROPERTY(EditAnywhere, Category = "UI")
    FName AttachSocket = NAME_None;

    /** Optional offsets relative to socket transform */
    UPROPERTY(EditAnywhere, Category = "UI")
    FTransform RelativeOffset = FTransform::Identity;

    /** Widget space (World, Screen, or WorldLocked) */
    UPROPERTY(EditAnywhere, Category = "UI")
    EWidgetSpace WidgetSpace = EWidgetSpace::World;

    /** Draw at desired size */
    UPROPERTY(EditAnywhere, Category = "UI")
    bool bDrawAtDesiredSize = true;

    /** Widget scale multiplier [-] */
    UPROPERTY(EditAnywhere, Category = "UI")
    float WidgetScale = 0.05f; // [-] - Scale factor for widget content

    /** Enable mouse interaction */
    UPROPERTY(EditAnywhere, Category = "UI")
    bool bEnableInteraction = true;

    /** Max interaction distance [cm] */
    UPROPERTY(EditAnywhere, Category = "UI")
    float InteractionDistance = 500.0f; // [cm] - Maximum distance for interaction

    /** Two-sided rendering */
    UPROPERTY(EditAnywhere, Category = "UI")
    bool bTwoSided = true;
};


//------------------------------------------------------------------------------
//                                   actor class
//------------------------------------------------------------------------------
UCLASS()
class GRIT_API AVehicleConstruct : public AVehicleSolver
{
    GENERATED_BODY()

public:
    AVehicleConstruct();

    /** Get spring arm component */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    USpringArmComponent* GetSpringArm() const { return SpringArm; }

    /** Get chase camera component */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    UCameraComponent* GetCamera() const { return ChaseCamera; }

    /** Spawn HMI widgets at configured socket locations */
    UFUNCTION(BlueprintCallable, Category = "HMI")
    void SpawnHMIWidgets();

    /** Clear all spawned HMI widgets */
    UFUNCTION(BlueprintCallable, Category = "HMI")
    void ClearHMIWidgets();

    /** Get spawned widget component by socket name */
    UFUNCTION(BlueprintCallable, Category = "HMI")
    UWidgetComponent* GetHMIWidget(FName SocketName) const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    //------------------------------------------------------------------------------
    // HMI Configuration
    //------------------------------------------------------------------------------

    /** Array of HMI widgets to spawn at vehicle sockets */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HMI")
    TArray<FVehicleMenuEntry> HMIMenus;

private:
    UPROPERTY(VisibleAnywhere, Category = "Camera")
    USpringArmComponent* SpringArm = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Camera")
    UCameraComponent* ChaseCamera = nullptr;

    //------------------------------------------------------------------------------
    // HMI Runtime State
    //------------------------------------------------------------------------------

    /** Spawned widget components */
    UPROPERTY()
    TArray<UWidgetComponent*> SpawnedHMIComponents;

    /** Socket name to widget component mapping */
    UPROPERTY()
    TMap<FName, UWidgetComponent*> SocketToWidgetMap;
};