#include "RewardEvaluator.h"
#include "Engine/World.h"

void URewardEvaluator::Initialize(AActor* Owner, ENeuralLinkEntityType EntityType)
{
	OwnerActor = Owner;
	Type = EntityType;
	Reset();
}

void URewardEvaluator::Reset()
{
	if (OwnerActor)
	{
		PreviousLocation = OwnerActor->GetActorLocation();
		PreviousVelocity = OwnerActor->GetVelocity();
	}
	else
	{
		PreviousLocation = FVector::ZeroVector;
		PreviousVelocity = FVector::ZeroVector;
	}
	PreviousAccel = FVector::ZeroVector;
	PreviousThrottle = 0.0f;
	PreviousSteering = 0.0f;
	PreviousBrake = 0.0f;
	EpisodeTimer = 0.0f;
	LastTickTime = 0.0f;
	bInitialized = true;
}

float URewardEvaluator::Evaluate(bool& bOutDone)
{
	bOutDone = false;
	return EvaluateGeneric(bOutDone);
}

float URewardEvaluator::EvaluateGeneric(bool& bOutDone)
{
	if (!OwnerActor || !OwnerActor->GetWorld()) return 0.0f;

	float CurrentTime = OwnerActor->GetWorld()->GetTimeSeconds();
	float DeltaTime = (LastTickTime > 0.0f) ? (CurrentTime - LastTickTime) : 0.016f;
	LastTickTime = CurrentTime;
	EpisodeTimer += DeltaTime;

	FVector Location = OwnerActor->GetActorLocation();
	FVector Velocity = OwnerActor->GetVelocity();
	FVector Forward = OwnerActor->GetActorForwardVector();

	float ForwardSpeedCms = FVector::DotProduct(Velocity, Forward);
	float ForwardSpeedKmh = ForwardSpeedCms * 0.036f;

	float Reward = 0.0f;

	// 1. Speed reward: encourage forward motion (0..1)
	float SpeedReward = FMath::Clamp(ForwardSpeedKmh / 200.0f, 0.0f, 1.0f);
	Reward += SpeedReward * 1.0f;

	// 2. Heading alignment with velocity direction (0..1)
	FVector VelDir = Velocity.GetSafeNormal();
	if (!VelDir.IsNearlyZero())
	{
		float HeadingAlignment = FMath::Max(0.0f, FVector::DotProduct(Forward, VelDir));
		Reward += HeadingAlignment * 0.5f;
	}

	// 3. Survival bonus
	Reward += SURVIVAL_BONUS_PER_SEC * DeltaTime;

	// 4. Action smoothness penalty (jerk = d(accel)/dt)
	FVector Accel = (Velocity - PreviousVelocity) / FMath::Max(DeltaTime, 0.001f);
	FVector Jerk = (Accel - PreviousAccel) / FMath::Max(DeltaTime, 0.001f);
	Reward -= FMath::Min(Jerk.Size() * 1e-5f, 0.1f);
	PreviousAccel = Accel;

	// 5. Collision detection (sudden deceleration proxy)
	float SpeedDelta = ForwardSpeedKmh - (PreviousVelocity.Size() * 0.036f);
	if (ForwardSpeedKmh < 1.0f && PreviousVelocity.Size() * 0.036f > 20.0f && SpeedDelta < -15.0f)
	{
		Reward += COLLISION_PENALTY;
		bOutDone = true;
	}

	// 6. Episode time limit
	if (EpisodeTimer >= MAX_EPISODE_TIME)
	{
		bOutDone = true;
	}

	// 7. Fell off the world
	if (Location.Z < -10000.0f)
	{
		Reward += COLLISION_PENALTY;
		bOutDone = true;
	}

	// Update state for next frame
	PreviousLocation = Location;
	PreviousVelocity = Velocity;

	return Reward;
}
