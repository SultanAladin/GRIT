#pragma once

#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"
#include "GameTracker.generated.h"

UENUM(BlueprintType)
enum class EGritSessionPhase : uint8
{
    Lobby,
    Warmup,
    InProgress,
    PostMatch
};

UCLASS()
class GRIT_API AGameTracker : public AGameStateBase
{
    GENERATED_BODY()

public:
    AGameTracker();

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Session")
    EGritSessionPhase SessionPhase = EGritSessionPhase::Lobby;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Session")
    int32 SessionId = 0;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Session")
    void SetSessionPhase(EGritSessionPhase NewPhase);

    UFUNCTION(BlueprintCallable, Category = "Session")
    void ResetSession();
};
