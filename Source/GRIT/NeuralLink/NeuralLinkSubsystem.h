#pragma once
// NeuralLinkSubsystem.h — UGameInstanceSubsystem that manages batch IPC with CortexServer.
// Collects observations from all NeuralLinkComponents, sends them in batch, receives actions.

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "NeuralLinkSubsystem.generated.h"

class UNeuralLinkComponent;

namespace Cortex { class IPCClient; }

UCLASS()
class GRIT_API UNeuralLinkSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return true; }
	virtual bool IsTickableInEditor() const override { return false; }

	// Registration
	void RegisterComponent(UNeuralLinkComponent* Component);
	void UnregisterComponent(UNeuralLinkComponent* Component);

	// Connection management
	UFUNCTION(BlueprintCallable, Category = "NeuralLink")
	bool ConnectToServer();

	UFUNCTION(BlueprintCallable, Category = "NeuralLink")
	void DisconnectFromServer();

	UFUNCTION(BlueprintPure, Category = "NeuralLink")
	bool IsConnected() const;

	UFUNCTION(BlueprintPure, Category = "NeuralLink")
	int32 GetActiveEntityCount() const { return RegisteredComponents.Num(); }

private:
	void BatchCollectAndSend();
	void BatchReceiveAndApply();
	void SendHeartbeat();

	UPROPERTY()
	TArray<UNeuralLinkComponent*> RegisteredComponents;

	Cortex::IPCClient* Client = nullptr;

	float HeartbeatTimer = 0.0f;
	float HeartbeatInterval = 5.0f;
	float ServerTimeoutSec = 2.0f;
	float LastServerSeenTime = 0.0f;
	uint32 FrameCounter = 0;
	bool bConnected = false;
	bool bServerStalled = false;
};
