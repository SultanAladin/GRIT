#pragma once

#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "PlayerTracker.generated.h"

//------------------------------------------------------------------------------
//                              player tracker (multiplayer playerstate)
//------------------------------------------------------------------------------
// PlayerState exists on the server and is replicated to all clients.
// Use it for persistent per-player data that should survive pawn changes
// (e.g., respawn into a new vehicle) and should be visible to other players
// (scoreboard, session stats, etc.).
UCLASS()
class GRIT_API APlayerTracker : public APlayerState
{
    GENERATED_BODY()

public:
    APlayerTracker();

    //------------------------------------------------------------------------------
    //                              live telemetry
    //------------------------------------------------------------------------------
    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Session|Telemetry")
    float CurrentSpeedKmh = 0.0f;                // [km/h] - Instantaneous speed (authoritative)

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Session|Telemetry")
    float CurrentThrottle = 0.0f;                // [0..1] - Current throttle input

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Session|Telemetry")
    float CurrentBrake = 0.0f;                   // [0..1] - Current brake input

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Session|Telemetry")
    int32 CurrentGear = 0;                       // [-] - Current gear (if available)

    //------------------------------------------------------------------------------
    //                              movement stats
    //------------------------------------------------------------------------------
    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Session|Stats")
    float TopSpeedKmh = 0.0f;                    // [km/h] - Peak session speed

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Session|Stats")
    float AverageSpeedKmh = 0.0f;                // [km/h] - Time-weighted average speed

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Session|Stats")
    float TotalDistanceKm = 0.0f;                // [km] - Total distance traveled

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Session|Stats")
    int32 CollisionCount = 0;                    // [-] - Collision events (if hooked up)

    //------------------------------------------------------------------------------
    //                              combat/session stats
    //------------------------------------------------------------------------------
    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Session|Combat")
    int32 Kills = 0;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Session|Combat")
    int32 Deaths = 0;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Session|Combat")
    float DamageDealt = 0.0f;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Session|Combat")
    float DamageTaken = 0.0f;

    //------------------------------------------------------------------------------
    //                              network replication
    //------------------------------------------------------------------------------
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    //------------------------------------------------------------------------------
    //                              telemetry updates
    //------------------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Session|Telemetry")
    void UpdateTelemetry(float SpeedKmh, float Throttle, float Brake, int32 Gear);

    UFUNCTION(BlueprintCallable, Category = "Session|Stats")
    void UpdateMovementStats(float SpeedKmh, float DistanceDeltaKm, float DeltaSeconds);

    UFUNCTION(BlueprintCallable, Category = "Session|Stats")
    void RegisterCollision();

    UFUNCTION(BlueprintCallable, Category = "Session")
    void ResetSessionStats();

private:
    float SpeedTimeAccumulator = 0.0f;           // [s]
    float SpeedAccumulator = 0.0f;               // [km/h*s]
};
