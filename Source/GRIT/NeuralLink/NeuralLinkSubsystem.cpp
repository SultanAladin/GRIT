#include "NeuralLinkSubsystem.h"
#include "NeuralLinkComponent.h"

#include "CortexTypes.h"
#include "IPCClient.h"

void UNeuralLinkSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Client = new Cortex::IPCClient();
}

void UNeuralLinkSubsystem::Deinitialize()
{
	DisconnectFromServer();
	delete Client;
	Client = nullptr;
	Super::Deinitialize();
}

void UNeuralLinkSubsystem::Tick(float DeltaTime)
{
	if (!bConnected) return;

	++FrameCounter;

	// Heartbeat
	HeartbeatTimer += DeltaTime;
	if (HeartbeatTimer >= HeartbeatInterval)
	{
		SendHeartbeat();
		HeartbeatTimer = 0.0f;
	}

	// If server went silent, skip IPC entirely — the in-process inference path
	// (NeuralLinkComponent::TryLocalInference) keeps creatures alive without it.
	const bool bAlive = Client && Client->CheckServerAlive(ServerTimeoutSec);
	if (!bAlive)
	{
		if (!bServerStalled)
		{
			UE_LOG(LogTemp, Warning, TEXT("[NeuralLink] Server silent >%.1fs, falling back to local inference"),
				ServerTimeoutSec);
			bServerStalled = true;
		}
		// Drive any component with a local model so gameplay doesn't freeze.
		for (auto* Comp : RegisteredComponents)
		{
			if (Comp && Comp->bEnabled && Comp->bLocalModelLoaded)
			{
				Comp->TryLocalInference();
			}
		}
		return;
	}
	if (bServerStalled)
	{
		UE_LOG(LogTemp, Log, TEXT("[NeuralLink] Server back online"));
		bServerStalled = false;
	}

	// Batch IPC: collect → send → receive → apply
	BatchCollectAndSend();
	BatchReceiveAndApply();
}

TStatId UNeuralLinkSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UNeuralLinkSubsystem, STATGROUP_Tickables);
}

void UNeuralLinkSubsystem::RegisterComponent(UNeuralLinkComponent* Component)
{
	if (Component && !RegisteredComponents.Contains(Component))
	{
		RegisteredComponents.Add(Component);

		// Send entity config to server
		if (bConnected)
		{
			Cortex::FEntityRegistration Config;
			Config.EntityTypeHash = Component->GetEntityTypeHash();
			Config.ObservationDim = Component->ObservationDim;
			Config.ActionDim = Component->ActionDim;
			Config.ActionMin = -1.0f;
			Config.ActionMax = 1.0f;
			FCStringAnsi::Strncpy(Config.TypeName,
				TCHAR_TO_ANSI(*Component->EntityTypeName),
				Cortex::MAX_ENTITY_TYPE_NAME);

			Client->SendConfig(Config);
		}

		UE_LOG(LogTemp, Log, TEXT("[NeuralLink] Registered entity: %s (#%u)"),
			*Component->EntityTypeName, Component->GetInstanceId());
	}
}

void UNeuralLinkSubsystem::UnregisterComponent(UNeuralLinkComponent* Component)
{
	RegisteredComponents.Remove(Component);
}

bool UNeuralLinkSubsystem::ConnectToServer()
{
	if (bConnected) return true;

	bConnected = Client->Connect(Cortex::IPC_CHANNEL_NAME);
	if (bConnected)
	{
		UE_LOG(LogTemp, Log, TEXT("[NeuralLink] Connected to CortexServer"));

		// Register all existing components
		for (auto* Comp : RegisteredComponents)
		{
			Cortex::FEntityRegistration Config;
			Config.EntityTypeHash = Comp->GetEntityTypeHash();
			Config.ObservationDim = Comp->ObservationDim;
			Config.ActionDim = Comp->ActionDim;
			Config.ActionMin = -1.0f;
			Config.ActionMax = 1.0f;
			FCStringAnsi::Strncpy(Config.TypeName,
				TCHAR_TO_ANSI(*Comp->EntityTypeName),
				Cortex::MAX_ENTITY_TYPE_NAME);
			Client->SendConfig(Config);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[NeuralLink] Failed to connect to CortexServer"));
	}

	return bConnected;
}

void UNeuralLinkSubsystem::DisconnectFromServer()
{
	if (bConnected)
	{
		Client->SendShutdown();
		Client->Disconnect();
		bConnected = false;
		UE_LOG(LogTemp, Log, TEXT("[NeuralLink] Disconnected from CortexServer"));
	}
}

bool UNeuralLinkSubsystem::IsConnected() const
{
	return bConnected && Client && Client->IsConnected();
}

void UNeuralLinkSubsystem::BatchCollectAndSend()
{
	int32 Count = RegisteredComponents.Num();
	if (Count == 0) return;

	// Collect observations
	TArray<Cortex::FObservationVector> Observations;
	Observations.SetNum(Count);

	TArray<Cortex::FRewardSignal> Rewards;
	Rewards.SetNum(Count);

	for (int32 i = 0; i < Count; ++i)
	{
		auto* Comp = RegisteredComponents[i];
		if (!Comp || !Comp->bEnabled) continue;

		// Collect observation
		FNeuralLinkObservation Obs;
		Comp->CollectObservation(Obs);

		Observations[i].EntityTypeHash = Comp->GetEntityTypeHash();
		Observations[i].InstanceId = Comp->GetInstanceId();
		Observations[i].Dim = Obs.Dim;
		FMemory::Memcpy(Observations[i].Data, Obs.Data.GetData(),
			FMath::Min(Obs.Dim, (int32)Cortex::MAX_OBSERVATION_DIM) * sizeof(float));

		// Compute reward
		bool bDone = false;
		float Reward = Comp->ComputeReward(bDone);

		Rewards[i].EntityTypeHash = Comp->GetEntityTypeHash();
		Rewards[i].InstanceId = Comp->GetInstanceId();
		Rewards[i].Reward = Reward;
		Rewards[i].bDone = bDone;
		Rewards[i].bTruncated = false;
	}

	// Send to server
	Client->SendObservations(Observations.GetData(), Count);
	Client->SendRewards(Rewards.GetData(), Count);
}

void UNeuralLinkSubsystem::BatchReceiveAndApply()
{
	// Try to receive actions
	TArray<Cortex::FActionVector> Actions;
	Actions.SetNum(RegisteredComponents.Num());

	uint32 ActionCount = 0;
	if (!Client->ReceiveActions(Actions.GetData(), Actions.Num(), ActionCount))
		return;

	// Match actions to components by entity hash + instance ID
	for (uint32 i = 0; i < ActionCount; ++i)
	{
		const auto& Act = Actions[i];

		for (auto* Comp : RegisteredComponents)
		{
			if (Comp && Comp->GetEntityTypeHash() == Act.EntityTypeHash &&
				Comp->GetInstanceId() == Act.InstanceId)
			{
				FNeuralLinkAction Action;
				Action.Dim = Act.Dim;
				Action.Data.SetNum(Act.Dim);
				FMemory::Memcpy(Action.Data.GetData(), Act.Data, Act.Dim * sizeof(float));

				Comp->ApplyAction(Action);
				break;
			}
		}
	}

	// Check for episode resets
	for (auto* Comp : RegisteredComponents)
	{
		if (Comp && Comp->CurrentEpisodeSteps > 0)
		{
			bool bDone = false;
			Comp->ComputeReward(bDone);
			if (bDone)
			{
				Comp->ResetEpisode();
			}
		}
	}
}

void UNeuralLinkSubsystem::SendHeartbeat()
{
	if (Client)
	{
		Client->SendHeartbeat();

		if (!Client->CheckServerAlive(10.0))
		{
			UE_LOG(LogTemp, Warning, TEXT("[NeuralLink] CortexServer heartbeat timeout"));
		}
	}
}
