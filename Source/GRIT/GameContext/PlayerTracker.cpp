#include "GameContext/PlayerTracker.h"
#include "Net/UnrealNetwork.h"

//------------------------------------------------------------------------------
//                              constructor
//------------------------------------------------------------------------------
APlayerTracker::APlayerTracker()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    SetNetUpdateFrequency(10.0f);                // [Hz] - PlayerState update rate

    CurrentSpeedKmh = 0.0f;
    CurrentThrottle = 0.0f;
    CurrentBrake = 0.0f;
    CurrentGear = 0;

    TopSpeedKmh = 0.0f;
    AverageSpeedKmh = 0.0f;
    TotalDistanceKm = 0.0f;
    CollisionCount = 0;

    Kills = 0;
    Deaths = 0;
    DamageDealt = 0.0f;
    DamageTaken = 0.0f;

    SpeedTimeAccumulator = 0.0f;
    SpeedAccumulator = 0.0f;
}

//------------------------------------------------------------------------------
//                              replication
//------------------------------------------------------------------------------
void APlayerTracker::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(APlayerTracker, CurrentSpeedKmh);
    DOREPLIFETIME(APlayerTracker, CurrentThrottle);
    DOREPLIFETIME(APlayerTracker, CurrentBrake);
    DOREPLIFETIME(APlayerTracker, CurrentGear);

    DOREPLIFETIME(APlayerTracker, TopSpeedKmh);
    DOREPLIFETIME(APlayerTracker, AverageSpeedKmh);
    DOREPLIFETIME(APlayerTracker, TotalDistanceKm);
    DOREPLIFETIME(APlayerTracker, CollisionCount);

    DOREPLIFETIME(APlayerTracker, Kills);
    DOREPLIFETIME(APlayerTracker, Deaths);
    DOREPLIFETIME(APlayerTracker, DamageDealt);
    DOREPLIFETIME(APlayerTracker, DamageTaken);
}

//------------------------------------------------------------------------------
//                              telemetry updates
//------------------------------------------------------------------------------
void APlayerTracker::UpdateTelemetry(float SpeedKmh, float Throttle, float Brake, int32 Gear)
{
    CurrentSpeedKmh = SpeedKmh;     // [km/h]
    CurrentThrottle = Throttle;     // [0..1]
    CurrentBrake = Brake;           // [0..1]
    CurrentGear = Gear;             // [-]
}

void APlayerTracker::UpdateMovementStats(float SpeedKmh, float DistanceDeltaKm, float DeltaSeconds)
{
    if (SpeedKmh > TopSpeedKmh)
    {
        TopSpeedKmh = SpeedKmh;
    }

    if (DeltaSeconds > 0.0f)
    {
        SpeedAccumulator += SpeedKmh * DeltaSeconds;
        SpeedTimeAccumulator += DeltaSeconds;
        AverageSpeedKmh = (SpeedTimeAccumulator > 0.0f) ? (SpeedAccumulator / SpeedTimeAccumulator) : 0.0f;
    }

    TotalDistanceKm += DistanceDeltaKm;
}

void APlayerTracker::RegisterCollision()
{
    ++CollisionCount;
}

void APlayerTracker::ResetSessionStats()
{
    CurrentSpeedKmh = 0.0f;
    CurrentThrottle = 0.0f;
    CurrentBrake = 0.0f;
    CurrentGear = 0;

    TopSpeedKmh = 0.0f;
    AverageSpeedKmh = 0.0f;
    TotalDistanceKm = 0.0f;
    CollisionCount = 0;

    Kills = 0;
    Deaths = 0;
    DamageDealt = 0.0f;
    DamageTaken = 0.0f;

    SpeedTimeAccumulator = 0.0f;
    SpeedAccumulator = 0.0f;
}
