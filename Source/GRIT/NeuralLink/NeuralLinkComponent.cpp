#include "NeuralLinkComponent.h"
#include "ObservationCollector.h"
#include "RewardEvaluator.h"
#include "NeuralNetModel.h"
#include "Misc/Paths.h"

uint32 UNeuralLinkComponent::NextInstanceId = 1;

UNeuralLinkComponent::UNeuralLinkComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

UNeuralLinkComponent::~UNeuralLinkComponent() = default;

void UNeuralLinkComponent::BeginPlay()
{
	Super::BeginPlay();

	InstanceId = NextInstanceId++;

	// FNV-1a hash of entity type name
	EntityTypeHash = 0x811C9DC5;
	for (TCHAR C : EntityTypeName)
	{
		EntityTypeHash ^= (uint8)(C & 0xFF);
		EntityTypeHash *= 0x01000193;
	}

	// Create observation collector and reward evaluator
	ObsCollector = NewObject<UObservationCollector>(this);
	ObsCollector->Initialize(GetOwner(), EntityType, ObservationDim);

	RewardEval = NewObject<URewardEvaluator>(this);
	RewardEval->Initialize(GetOwner(), EntityType);

	LatestObservation.Data.SetNumZeroed(ObservationDim);
	LatestObservation.Dim = ObservationDim;
	LatestAction.Data.SetNumZeroed(ActionDim);
	LatestAction.Dim = ActionDim;

	PreviousLocation = GetOwner()->GetActorLocation();
	EpisodeStartTime = GetWorld()->GetTimeSeconds();

	// Load local model if configured.
	if (!ModelFilePath.IsEmpty())
	{
		LocalModel = MakeUnique<FNeuralNetModel>();
		FString ResolvedPath = ModelFilePath;
		if (FPaths::IsRelative(ResolvedPath))
		{
			ResolvedPath = FPaths::ProjectContentDir() / ResolvedPath;
		}
		FString LoadErr;
		if (LocalModel->LoadFromFile(ResolvedPath, &LoadErr) && LocalModel->IsValid())
		{
			bLocalModelLoaded = true;
			UE_LOG(LogTemp, Log, TEXT("[NeuralLink] Loaded local model '%s' (%d->%d, %d steps)"),
				*LocalModel->Name, LocalModel->InputDim, LocalModel->OutputDim, LocalModel->TrainedSteps);
		}
		else
		{
			LocalModel.Reset();
			UE_LOG(LogTemp, Warning, TEXT("[NeuralLink] Failed to load model '%s': %s"),
				*ResolvedPath, *LoadErr);
		}
	}
}

void UNeuralLinkComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                          FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEnabled) return;

	++CurrentEpisodeSteps;
	PreviousLocation = GetOwner()->GetActorLocation();

	// Local inference path: runs when preferred or when the server isn't
	// driving this component. The subsystem will overwrite LatestAction if the
	// server sends one later in the same frame.
	if (bLocalModelLoaded && bPreferLocalInference)
	{
		TryLocalInference();
	}
}

void UNeuralLinkComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UNeuralLinkComponent::CollectObservation(FNeuralLinkObservation& OutObs)
{
	if (ObsCollector)
	{
		ObsCollector->Collect(OutObs);
	}
	LatestObservation = OutObs;
}

void UNeuralLinkComponent::ApplyAction(const FNeuralLinkAction& Action)
{
	LatestAction = Action;

	// Fire delegate — the owning entity handles how to apply this
	OnActionReceived.Broadcast(Action);
}

float UNeuralLinkComponent::ComputeReward(bool& bOutDone)
{
	float Reward = 0.0f;
	bOutDone = false;

	if (RewardEval)
	{
		Reward = RewardEval->Evaluate(bOutDone);
	}

	CurrentEpisodeReward += Reward;
	return Reward;
}

FNeuralLinkObservation UNeuralLinkComponent::GetLatestObservation() const
{
	return LatestObservation;
}

FNeuralLinkAction UNeuralLinkComponent::GetLatestAction() const
{
	return LatestAction;
}

bool UNeuralLinkComponent::TryLocalInference()
{
	if (!bLocalModelLoaded || !LocalModel.IsValid() || !LocalModel->IsValid()) return false;

	// Collect observation now so inference runs on fresh state.
	FNeuralLinkObservation Obs;
	CollectObservation(Obs);

	if (Obs.Dim != LocalModel->InputDim)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[NeuralLink] Local model input mismatch: obs=%d, model=%d"),
			Obs.Dim, LocalModel->InputDim);
		return false;
	}

	FNeuralLinkAction Action;
	Action.Dim = LocalModel->OutputDim;
	LocalModel->Forward(Obs.Data, Action.Data);

	ApplyAction(Action);
	return true;
}

void UNeuralLinkComponent::ResetEpisode()
{
	CurrentEpisodeReward = 0.0f;
	CurrentEpisodeSteps = 0;
	EpisodeStartTime = GetWorld()->GetTimeSeconds();
	PreviousLocation = GetOwner()->GetActorLocation();
	PreviousSpeed = 0.0f;

	if (RewardEval)
	{
		RewardEval->Reset();
	}
}
