#include "ViewportContext.h"
#include "ViewportNavigator.h"
#include "VehicleConstruct.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

//------------------------------------------------------------------------------
//                          CONSTRUCTOR
//------------------------------------------------------------------------------
AViewportContext::AViewportContext()
{
    // Set default controller to ViewportNavigator
    PlayerControllerClass = AViewportNavigator::StaticClass();

    // Set default pawn to VehicleConstruct
    DefaultPawnClass = AVehicleConstruct::StaticClass();

    // Disable auto-spawn - we want manual control via deployment points
    bAutoSpawnPlayers = true;
}

//------------------------------------------------------------------------------
//                          BEGIN PLAY
//------------------------------------------------------------------------------
void AViewportContext::BeginPlay()
{
    Super::BeginPlay();

    if (SessionTimeout > 0.0f) // Reason: start session timer
    {
        GetWorldTimerManager().SetTimer(
            SessionTimerHandle,
            this,
            &AViewportContext::HandleSessionTimeout,
            SessionTimeout,
            false
        );
    } // End if (timeout enabled)
} // End BeginPlay()

//------------------------------------------------------------------------------
//                          POST LOGIN
//------------------------------------------------------------------------------
void AViewportContext::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    // Reset session timer on player join
    if (SessionTimeout > 0.0f) // Reason: restart timeout on activity
    {
        GetWorldTimerManager().ClearTimer(SessionTimerHandle);
        GetWorldTimerManager().SetTimer(
            SessionTimerHandle,
            this,
            &AViewportContext::HandleSessionTimeout,
            SessionTimeout,
            false
        );
    } // End if (timeout enabled)

    // Configure the vehicle if player already has one
    if (APawn* Pawn = NewPlayer->GetPawn()) // Reason: configure existing pawn
    {
        if (AVehicleConstruct* Vehicle = Cast<AVehicleConstruct>(Pawn))
        {
            ConfigureVehicleForDisplay(Vehicle);
        } // End if (vehicle construct)
    } // End if (has pawn)
} // End PostLogin()

//------------------------------------------------------------------------------
//                          HANDLE STARTING NEW PLAYER
//------------------------------------------------------------------------------
void AViewportContext::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
    Super::HandleStartingNewPlayer_Implementation(NewPlayer);

    // Configure vehicle after spawn
    if (APawn* Pawn = NewPlayer->GetPawn()) // Reason: configure newly spawned pawn
    {
        if (AVehicleConstruct* Vehicle = Cast<AVehicleConstruct>(Pawn))
        {
            ConfigureVehicleForDisplay(Vehicle);
        } // End if (vehicle construct)
    } // End if (has pawn)
} // End HandleStartingNewPlayer_Implementation()

//------------------------------------------------------------------------------
//                          CONFIGURE VEHICLE FOR DISPLAY
//------------------------------------------------------------------------------
void AViewportContext::ConfigureVehicleForDisplay(AVehicleConstruct* Vehicle)
{
    if (!Vehicle)
        return;

    if (bLockBrakesOnSpawn) // Reason: lock brakes for static display
    {
        Vehicle->InputTensor_GameThread.Brake = 1.0f;
        Vehicle->InputTensor_GameThread.Handbrake = 1.0f;
    } // End if (lock brakes)

    // Reset other inputs to neutral
    Vehicle->InputTensor_GameThread.Throttle = 0.0f;
    Vehicle->InputTensor_GameThread.Steering = 0.0f;
    Vehicle->InputTensor_GameThread.Clutch = 1.0f;         // Clutch disengaged
} // End ConfigureVehicleForDisplay()

//------------------------------------------------------------------------------
//                          HANDLE SESSION TIMEOUT
//------------------------------------------------------------------------------
void AViewportContext::HandleSessionTimeout()
{
    // Session timeout reached - could return to main menu or show warning
    // For now, just log it
    UE_LOG(LogTemp, Warning, TEXT("ViewportContext: Session timeout reached"));
} // End HandleSessionTimeout()
