#pragma once
// NeuralLinkComponent.h — Generic UActorComponent for AI control via CortexServer.
// Attach to any Actor/Pawn. Observations collected via ObservationCollector,
// actions delivered via OnActionReceived delegate.

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NeuralNetModel.h"
#include "NeuralLinkComponent.generated.h"

class UObservationCollector;
class URewardEvaluator;

UENUM(BlueprintType)
enum class ENeuralLinkEntityType : uint8
{
	Vehicle,
	Human,
	Creature,
	Aircraft,
	Transport,
	Custom
};

USTRUCT(BlueprintType)
struct FNeuralLinkObservation
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<float> Data;

	UPROPERTY()
	int32 Dim = 0;
};

USTRUCT(BlueprintType)
struct FNeuralLinkAction
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<float> Data;

	UPROPERTY()
	int32 Dim = 0;
};

// Delegate: fired when an action is received from CortexServer
// The owning entity binds this to apply actions however it wants
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionReceived, const FNeuralLinkAction&, Action);

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class GRIT_API UNeuralLinkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNeuralLinkComponent();
	virtual ~UNeuralLinkComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Configuration
	UPROPERTY(EditAnywhere, Category = "NeuralLink")
	ENeuralLinkEntityType EntityType = ENeuralLinkEntityType::Vehicle;

	UPROPERTY(EditAnywhere, Category = "NeuralLink")
	FString EntityTypeName = TEXT("Vehicle");

	UPROPERTY(EditAnywhere, Category = "NeuralLink")
	int32 ObservationDim = 47;

	UPROPERTY(EditAnywhere, Category = "NeuralLink")
	int32 ActionDim = 4;

	UPROPERTY(EditAnywhere, Category = "NeuralLink")
	bool bEnabled = true;

	// Path to a trained .md model. If set, the component will run local CPU
	// inference whenever the CortexServer is not connected or silent.
	UPROPERTY(EditAnywhere, Category = "NeuralLink|Local Inference")
	FString ModelFilePath;

	// If true, prefer local inference even when the server is live. Useful for
	// shipping builds where you never want IPC at runtime.
	UPROPERTY(EditAnywhere, Category = "NeuralLink|Local Inference")
	bool bPreferLocalInference = false;

	UPROPERTY(BlueprintReadOnly, Category = "NeuralLink|Local Inference")
	bool bLocalModelLoaded = false;

	// Runtime state
	UPROPERTY(BlueprintReadOnly, Category = "NeuralLink")
	bool bConnectedToServer = false;

	UPROPERTY(BlueprintReadOnly, Category = "NeuralLink")
	float CurrentEpisodeReward = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "NeuralLink")
	int32 CurrentEpisodeSteps = 0;

	// Delegate — bind from your entity to handle actions
	UPROPERTY(BlueprintAssignable, Category = "NeuralLink")
	FOnActionReceived OnActionReceived;

	// Get the latest observation (for debugging/visualization)
	UFUNCTION(BlueprintCallable, Category = "NeuralLink")
	FNeuralLinkObservation GetLatestObservation() const;

	// Get the latest action (for debugging)
	UFUNCTION(BlueprintCallable, Category = "NeuralLink")
	FNeuralLinkAction GetLatestAction() const;

	// Force episode reset
	UFUNCTION(BlueprintCallable, Category = "NeuralLink")
	void ResetEpisode();

	// Internal — called by NeuralLinkSubsystem
	void CollectObservation(FNeuralLinkObservation& OutObs);
	void ApplyAction(const FNeuralLinkAction& Action);
	float ComputeReward(bool& bOutDone);

	// Runs the local .md model on the latest observation and broadcasts the
	// action via OnActionReceived. Returns true if a model was loaded and ran.
	bool TryLocalInference();

	UObservationCollector* GetObservationCollector() const { return ObsCollector; }

	uint32 GetInstanceId() const { return InstanceId; }
	uint32 GetEntityTypeHash() const { return EntityTypeHash; }

private:
	UPROPERTY()
	UObservationCollector* ObsCollector = nullptr;

	UPROPERTY()
	URewardEvaluator* RewardEval = nullptr;

	FNeuralLinkObservation LatestObservation;
	FNeuralLinkAction LatestAction;

	TUniquePtr<FNeuralNetModel> LocalModel;

	uint32 InstanceId = 0;
	uint32 EntityTypeHash = 0;
	float EpisodeStartTime = 0.0f;

	FVector PreviousLocation;
	float PreviousSpeed = 0.0f;

	static uint32 NextInstanceId;
};
