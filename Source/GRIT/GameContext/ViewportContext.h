#pragma once

#include "CoreMinimal.h"
#include "GameContext.h"
#include "ViewportContext.generated.h"

class AViewportNavigator;
class AVehicleConstruct;

//------------------------------------------------------------------------------
//                          VIEWPORT CONTEXT GAMEMODE
//------------------------------------------------------------------------------
/** Gamemode for configurator viewport - uses ViewportNavigator camera controller and locks vehicle brakes */
UCLASS(BlueprintType, Blueprintable)
class GRIT_API AViewportContext : public AGameContext
{
    GENERATED_BODY()

public:
    AViewportContext();

protected:
    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

    /** Configure vehicle for static display (lock brakes, disable physics input) */
    void ConfigureVehicleForDisplay(AVehicleConstruct* Vehicle);

public:
    //--------------------------------------------------------------------------
    //                          CONFIGURATOR SETTINGS
    //--------------------------------------------------------------------------

    /** Controller class to spawn - defaults to ViewportNavigator */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewport Settings")
    TSubclassOf<APlayerController> ViewportControllerClass;

    /** Pawn class to spawn - defaults to VehicleConstruct */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewport Settings")
    TSubclassOf<APawn> ViewportPawnClass;

    /** Lock brakes on spawned vehicles */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewport Settings")
    bool bLockBrakesOnSpawn = true;

    /** Session timeout in seconds (0 = disabled) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewport Settings")
    float SessionTimeout = 1800.0f;                       // [s] - 30 minutes default

private:
    /** Timer handle for session timeout */
    FTimerHandle SessionTimerHandle;

    /** Handle session timeout */
    void HandleSessionTimeout();
};
