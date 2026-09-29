#include "ObservationCollector.h"
#include "CortexTypes.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

void UObservationCollector::Initialize(AActor* Owner, ENeuralLinkEntityType EntityType, int32 ObsDim)
{
	OwnerActor = Owner;
	Type = EntityType;
	Dim = ObsDim;

	if (Type == ENeuralLinkEntityType::Creature && OwnerActor)
	{
		Perception = OwnerActor->FindComponentByClass<UAIPerceptionComponent>();
	}
}

void UObservationCollector::Collect(FNeuralLinkObservation& OutObs)
{
	OutObs.Data.SetNumZeroed(Dim);
	OutObs.Dim = Dim;

	if (!OwnerActor) return;

	switch (Type)
	{
	case ENeuralLinkEntityType::Creature:
		CollectCreatureObservation(OutObs);
		break;
	default:
		CollectVehicleObservation(OutObs);
		break;
	}
}

void UObservationCollector::NotifyDamageTaken(const FVector& HitFromDirectionWorld)
{
	if (!OwnerActor || !OwnerActor->GetWorld()) return;
	LastDamageTime = OwnerActor->GetWorld()->GetTimeSeconds();
	LastDamageDirWorld = HitFromDirectionWorld.GetSafeNormal();
}

void UObservationCollector::NotifyAbilityUsed(float CooldownSeconds)
{
	if (!OwnerActor || !OwnerActor->GetWorld()) return;
	AbilityCooldownDuration = FMath::Max(0.01f, CooldownSeconds);
	AbilityReadyAtTime = OwnerActor->GetWorld()->GetTimeSeconds() + AbilityCooldownDuration;
}

float UObservationCollector::WriteRelativeDirection(FNeuralLinkObservation& OutObs,
	int32 IdxFwd, int32 IdxRight, int32 IdxDist,
	const FVector& WorldPos, float MaxRange)
{
	FVector Origin = OwnerActor->GetActorLocation();
	FVector Forward = OwnerActor->GetActorForwardVector();
	FVector Right = OwnerActor->GetActorRightVector();

	FVector ToTarget = WorldPos - Origin;
	float Dist = ToTarget.Size();
	FVector Dir = (Dist > 1.0f) ? (ToTarget / Dist) : FVector::ForwardVector;

	if (IdxFwd   >= 0 && IdxFwd   < Dim) OutObs.Data[IdxFwd]   = FVector::DotProduct(Forward, Dir);
	if (IdxRight >= 0 && IdxRight < Dim) OutObs.Data[IdxRight] = FVector::DotProduct(Right,   Dir);
	float DistNorm = FMath::Clamp(Dist / MaxRange, 0.0f, 1.0f);
	if (IdxDist  >= 0 && IdxDist  < Dim) OutObs.Data[IdxDist]  = DistNorm;
	return DistNorm;
}

void UObservationCollector::CollectVehicleObservation(FNeuralLinkObservation& OutObs)
{
	// Vehicle observations are self-state only. Kept at VehicleObs indices for
	// backward compat with existing CortexServer configs — raycast slots stay zeroed.
	using namespace Cortex;

	FVector Velocity = OwnerActor->GetVelocity();
	FVector Forward  = OwnerActor->GetActorForwardVector();
	FVector Right    = OwnerActor->GetActorRightVector();

	float ForwardSpeedKmh = FVector::DotProduct(Velocity, Forward) * 0.036f;
	float LateralSpeedKmh = FVector::DotProduct(Velocity, Right)   * 0.036f;

	if (Dim > (int32)VehicleObs::FORWARD_SPEED)
		OutObs.Data[VehicleObs::FORWARD_SPEED] = FMath::Clamp(ForwardSpeedKmh / MAX_SPEED_KMH, -1.0f, 1.0f);
	if (Dim > (int32)VehicleObs::LATERAL_SPEED)
		OutObs.Data[VehicleObs::LATERAL_SPEED] = FMath::Clamp(LateralSpeedKmh / MAX_SPEED_KMH, -1.0f, 1.0f);

	if (Dim > (int32)VehicleObs::HEADING_SIN)
	{
		FVector VelDir = Velocity.GetSafeNormal();
		if (!VelDir.IsNearlyZero())
		{
			OutObs.Data[VehicleObs::HEADING_COS] = FVector::DotProduct(Forward, VelDir);
			OutObs.Data[VehicleObs::HEADING_SIN] = FVector::DotProduct(Right,   VelDir);
		}
		else
		{
			OutObs.Data[VehicleObs::HEADING_COS] = 1.0f;
		}
	}

	if (Dim > (int32)VehicleObs::YAW_RATE)
	{
		if (UPrimitiveComponent* PrimRoot = Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent()))
		{
			FVector AngVel = PrimRoot->GetPhysicsAngularVelocityInRadians();
			OutObs.Data[VehicleObs::YAW_RATE] = FMath::Clamp(AngVel.Z / MAX_YAW_RATE, -1.0f, 1.0f);
		}
	}
}

void UObservationCollector::CollectCreatureObservation(FNeuralLinkObservation& OutObs)
{
	using namespace Cortex;

	UWorld* World = OwnerActor->GetWorld();
	if (!World) return;

	const float Now = World->GetTimeSeconds();

	// ---- Self state [0..5] -------------------------------------------------
	FVector Velocity = OwnerActor->GetVelocity();
	FVector Forward  = OwnerActor->GetActorForwardVector();
	FVector Right    = OwnerActor->GetActorRightVector();

	float FwdKmh = FVector::DotProduct(Velocity, Forward) * 0.036f;
	float LatKmh = FVector::DotProduct(Velocity, Right)   * 0.036f;

	if (Dim > (int32)CreatureObs::SELF_FORWARD_SPEED)
		OutObs.Data[CreatureObs::SELF_FORWARD_SPEED] = FMath::Clamp(FwdKmh / MAX_SPEED_KMH, -1.0f, 1.0f);
	if (Dim > (int32)CreatureObs::SELF_LATERAL_SPEED)
		OutObs.Data[CreatureObs::SELF_LATERAL_SPEED] = FMath::Clamp(LatKmh / MAX_SPEED_KMH, -1.0f, 1.0f);

	if (Dim > (int32)CreatureObs::SELF_YAW_RATE)
	{
		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent()))
		{
			FVector AngVel = Prim->GetPhysicsAngularVelocityInRadians();
			OutObs.Data[CreatureObs::SELF_YAW_RATE] = FMath::Clamp(AngVel.Z / MAX_YAW_RATE, -1.0f, 1.0f);
		}
	}

	// Health/stamina/ground-normal are gameplay-owned. Default to 1.0 unless
	// the creature actor exposes them (future: read from an interface).
	if (Dim > (int32)CreatureObs::SELF_HEALTH_NORM)  OutObs.Data[CreatureObs::SELF_HEALTH_NORM]  = 1.0f;
	if (Dim > (int32)CreatureObs::SELF_STAMINA_NORM) OutObs.Data[CreatureObs::SELF_STAMINA_NORM] = 1.0f;
	if (Dim > (int32)CreatureObs::SELF_GROUND_NORMAL_Z) OutObs.Data[CreatureObs::SELF_GROUND_NORMAL_Z] = 1.0f;

	// ---- Sight [6..13] -----------------------------------------------------
	// Find the closest currently-perceived hostile. AIPerception has already
	// done the trace + FOV + team filter work for us.
	AActor* BestSightActor = nullptr;
	float   BestSightDistSq = TNumericLimits<float>::Max();
	FAIStimulus BestSightStimulus;

	if (Perception)
	{
		TArray<AActor*> Known;
		Perception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), Known);
		const FVector Origin = OwnerActor->GetActorLocation();
		for (AActor* A : Known)
		{
			if (!A || A == OwnerActor) continue;
			const float DSq = FVector::DistSquaredXY(Origin, A->GetActorLocation());
			if (DSq < BestSightDistSq)
			{
				BestSightDistSq = DSq;
				BestSightActor = A;
			}
		}
	}

	if (BestSightActor)
	{
		FVector TargetPos = BestSightActor->GetActorLocation();
		FVector TargetVel = BestSightActor->GetVelocity();

		LastSightTime = Now;
		LastSightPos = TargetPos;
		LastSightVel = TargetVel;

		if (Dim > (int32)CreatureObs::SIGHT_HAS_TARGET)
			OutObs.Data[CreatureObs::SIGHT_HAS_TARGET] = 1.0f;

		WriteRelativeDirection(OutObs,
			CreatureObs::SIGHT_DIR_FORWARD,
			CreatureObs::SIGHT_DIR_RIGHT,
			CreatureObs::SIGHT_DISTANCE_NORM,
			TargetPos, MAX_SIGHT_RANGE_CM);

		if (Dim > (int32)CreatureObs::SIGHT_HEIGHT_DELTA)
			OutObs.Data[CreatureObs::SIGHT_HEIGHT_DELTA] =
				FMath::Clamp((TargetPos.Z - OwnerActor->GetActorLocation().Z) / MAX_SIGHT_RANGE_CM, -1.0f, 1.0f);

		if (Dim > (int32)CreatureObs::SIGHT_TARGET_SPEED)
			OutObs.Data[CreatureObs::SIGHT_TARGET_SPEED] =
				FMath::Clamp(TargetVel.Size() * 0.036f / MAX_SPEED_KMH, 0.0f, 1.0f);

		if (Dim > (int32)CreatureObs::SIGHT_TARGET_APPROACH)
		{
			FVector ToTarget = TargetPos - OwnerActor->GetActorLocation();
			FVector RelVel = TargetVel - Velocity;
			float Closing = -FVector::DotProduct(RelVel, ToTarget.GetSafeNormal()) * 0.036f;
			OutObs.Data[CreatureObs::SIGHT_TARGET_APPROACH] =
				FMath::Clamp(Closing / MAX_SPEED_KMH, -1.0f, 1.0f);
		}

		if (Dim > (int32)CreatureObs::SIGHT_AGE_NORM)
			OutObs.Data[CreatureObs::SIGHT_AGE_NORM] = 0.0f;
	}
	else if (LastSightTime > -1e8f)
	{
		// Remembered target — write decayed entry.
		float Age = Now - LastSightTime;
		float AgeNorm = FMath::Clamp(Age / MEMORY_DECAY_SEC, 0.0f, 1.0f);
		if (AgeNorm < 1.0f)
		{
			// Extrapolate position for a smoother learning signal.
			FVector Predicted = LastSightPos + LastSightVel * Age;
			if (Dim > (int32)CreatureObs::SIGHT_HAS_TARGET)
				OutObs.Data[CreatureObs::SIGHT_HAS_TARGET] = 1.0f - AgeNorm; // soft flag
			WriteRelativeDirection(OutObs,
				CreatureObs::SIGHT_DIR_FORWARD,
				CreatureObs::SIGHT_DIR_RIGHT,
				CreatureObs::SIGHT_DISTANCE_NORM,
				Predicted, MAX_SIGHT_RANGE_CM);
			if (Dim > (int32)CreatureObs::SIGHT_AGE_NORM)
				OutObs.Data[CreatureObs::SIGHT_AGE_NORM] = AgeNorm;
		}
	}

	// ---- Hearing [14..18] --------------------------------------------------
	if (Perception)
	{
		TArray<AActor*> Heard;
		Perception->GetCurrentlyPerceivedActors(UAISense_Hearing::StaticClass(), Heard);
		AActor* BestHearActor = nullptr;
		float   BestHearDistSq = TNumericLimits<float>::Max();
		const FVector Origin = OwnerActor->GetActorLocation();
		for (AActor* A : Heard)
		{
			if (!A || A == OwnerActor) continue;
			const float DSq = FVector::DistSquaredXY(Origin, A->GetActorLocation());
			if (DSq < BestHearDistSq)
			{
				BestHearDistSq = DSq;
				BestHearActor = A;
			}
		}

		if (BestHearActor)
		{
			LastHearTime = Now;
			LastHearPos = BestHearActor->GetActorLocation();
		}
	}

	if (LastHearTime > -1e8f)
	{
		float Age = Now - LastHearTime;
		float AgeNorm = FMath::Clamp(Age / MEMORY_DECAY_SEC, 0.0f, 1.0f);
		if (AgeNorm < 1.0f)
		{
			if (Dim > (int32)CreatureObs::HEAR_HAS_CUE)
				OutObs.Data[CreatureObs::HEAR_HAS_CUE] = 1.0f - AgeNorm;
			WriteRelativeDirection(OutObs,
				CreatureObs::HEAR_DIR_FORWARD,
				CreatureObs::HEAR_DIR_RIGHT,
				CreatureObs::HEAR_DISTANCE_NORM,
				LastHearPos, MAX_HEAR_RANGE_CM);
			if (Dim > (int32)CreatureObs::HEAR_AGE_NORM)
				OutObs.Data[CreatureObs::HEAR_AGE_NORM] = AgeNorm;
		}
	}

	// ---- Tactical memory [19..23] -----------------------------------------
	if (LastDamageTime > -1e8f && Dim > (int32)CreatureObs::LAST_DAMAGE_AGE)
	{
		float Age = Now - LastDamageTime;
		float AgeNorm = FMath::Clamp(Age / MEMORY_DECAY_SEC, 0.0f, 1.0f);
		OutObs.Data[CreatureObs::LAST_DAMAGE_AGE] = AgeNorm;
		if (AgeNorm < 1.0f)
		{
			if (Dim > (int32)CreatureObs::LAST_DAMAGE_DIR_FWD)
				OutObs.Data[CreatureObs::LAST_DAMAGE_DIR_FWD] = FVector::DotProduct(Forward, LastDamageDirWorld);
			if (Dim > (int32)CreatureObs::LAST_DAMAGE_DIR_RT)
				OutObs.Data[CreatureObs::LAST_DAMAGE_DIR_RT] = FVector::DotProduct(Right, LastDamageDirWorld);
		}
	}

	if (Dim > (int32)CreatureObs::ABILITY_COOLDOWN)
	{
		float Remaining = FMath::Max(0.0f, AbilityReadyAtTime - Now);
		OutObs.Data[CreatureObs::ABILITY_COOLDOWN] = FMath::Clamp(Remaining / AbilityCooldownDuration, 0.0f, 1.0f);
	}

	if (Dim > (int32)CreatureObs::MODE_ONEHOT)
		OutObs.Data[CreatureObs::MODE_ONEHOT] = FMath::Clamp(CurrentMode / 4.0f, 0.0f, 1.0f);
}
