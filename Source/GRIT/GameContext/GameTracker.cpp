#include "GameContext/GameTracker.h"

#include "Net/UnrealNetwork.h"

AGameTracker::AGameTracker()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    SetNetUpdateFrequency(5.0f);
}

void AGameTracker::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AGameTracker, SessionPhase);
    DOREPLIFETIME(AGameTracker, SessionId);
}

void AGameTracker::SetSessionPhase(EGritSessionPhase NewPhase)
{
    SessionPhase = NewPhase;
}

void AGameTracker::ResetSession()
{
    SessionPhase = EGritSessionPhase::Lobby;
    ++SessionId;
}
