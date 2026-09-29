#pragma once
// ObservationCollector.h — State extraction + normalization for entity observations.
//
// Two collection paths:
//   Vehicle — self physics state only (raycast fan removed; use dedicated vehicle sensors
//             at the gameplay layer if needed).
//   Creature — AIPerception (sight+hearing) + self state + tactical memory.
//              See CreatureObs in CortexTypes.h for the 24-dim layout.

#include "CoreMinimal.h"
#include "NeuralLinkComponent.h"
#include "ObservationCollector.generated.h"

class UAIPerceptionComponent;
class AActor;

UCLASS()
class GRIT_API UObservationCollector : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AActor* Owner, ENeuralLinkEntityType EntityType, int32 ObsDim);
	void Collect(FNeuralLinkObservation& OutObs);

	// Optional external feed — creature calls these when it takes damage / fires ability.
	void NotifyDamageTaken(const FVector& HitFromDirectionWorld);
	void NotifyAbilityUsed(float CooldownSeconds);
	void SetMode(int32 ModeEnum) { CurrentMode = ModeEnum; }

private:
	void CollectVehicleObservation(FNeuralLinkObservation& OutObs);
	void CollectCreatureObservation(FNeuralLinkObservation& OutObs);

	// Writes [fwd, right, distNorm] from a world position, relative to owner.
	// Returns distNorm (0..1).
	float WriteRelativeDirection(FNeuralLinkObservation& OutObs, int32 IdxFwd, int32 IdxRight,
	                             int32 IdxDist, const FVector& WorldPos, float MaxRange);

	UPROPERTY()
	AActor* OwnerActor = nullptr;

	UPROPERTY()
	UAIPerceptionComponent* Perception = nullptr;

	ENeuralLinkEntityType Type = ENeuralLinkEntityType::Vehicle;
	int32 Dim = 0;

	// Tactical memory (creature)
	float LastSightTime = -1e9f;
	FVector LastSightPos = FVector::ZeroVector;
	FVector LastSightVel = FVector::ZeroVector;

	float LastHearTime = -1e9f;
	FVector LastHearPos = FVector::ZeroVector;

	float LastDamageTime = -1e9f;
	FVector LastDamageDirWorld = FVector::ZeroVector;

	float AbilityReadyAtTime = 0.0f;
	float AbilityCooldownDuration = 1.0f;

	int32 CurrentMode = 0;

	// Normalization constants
	static constexpr float MAX_SPEED_KMH = 200.0f;
	static constexpr float MAX_YAW_RATE = 3.14159f;
	static constexpr float MAX_SIGHT_RANGE_CM = 5000.0f;   // 50 m
	static constexpr float MAX_HEAR_RANGE_CM  = 8000.0f;   // 80 m
	static constexpr float MEMORY_DECAY_SEC = 10.0f;       // age clamp horizon
};
