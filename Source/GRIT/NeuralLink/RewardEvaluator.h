#pragma once
// RewardEvaluator.h — Per-entity-type reward functions for reinforcement learning.

#include "CoreMinimal.h"
#include "NeuralLinkComponent.h"
#include "RewardEvaluator.generated.h"

UCLASS()
class GRIT_API URewardEvaluator : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AActor* Owner, ENeuralLinkEntityType EntityType);
	float Evaluate(bool& bOutDone);
	void Reset();

private:
	float EvaluateGeneric(bool& bOutDone);

	AActor* OwnerActor = nullptr;
	ENeuralLinkEntityType Type = ENeuralLinkEntityType::Vehicle;

	// Tracking state
	FVector PreviousLocation = FVector::ZeroVector;
	FVector PreviousVelocity = FVector::ZeroVector;
	FVector PreviousAccel = FVector::ZeroVector;
	float PreviousThrottle = 0.0f;
	float PreviousSteering = 0.0f;
	float PreviousBrake = 0.0f;
	float EpisodeTimer = 0.0f;
	float LastTickTime = 0.0f;
	bool bInitialized = false;

	// Episode termination
	static constexpr float MAX_EPISODE_TIME = 120.0f;      // 2 minutes
	static constexpr float COLLISION_PENALTY = -10.0f;
	static constexpr float SURVIVAL_BONUS_PER_SEC = 0.01f;
};
