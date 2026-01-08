#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LobbyDescriptor.generated.h"

UENUM(BlueprintType)
enum class ELobbyStatus : uint8
{
    Open        UMETA(DisplayName = "Open"),
    Starting    UMETA(DisplayName = "Starting"), 
    InMatch     UMETA(DisplayName = "In Match")
};

UENUM(BlueprintType)
enum class ELobbyType : uint8
{
    Lobby           UMETA(DisplayName = "Lobby"),
    Invitational    UMETA(DisplayName = "Invitational"),
    Headquarters    UMETA(DisplayName = "Headquarters")
};

UENUM(BlueprintType)
enum class ELobbyTag : uint8
{
    QuickMatch  UMETA(DisplayName = "QuickMatch"),
    Ranked      UMETA(DisplayName = "Ranked"),
    Solo        UMETA(DisplayName = "Solo")
};

UENUM(BlueprintType)
enum class ELobbyActionType : uint8
{
    Play        UMETA(DisplayName = "Play"),
    Rejoin      UMETA(DisplayName = "Rejoin"),
    Spectate    UMETA(DisplayName = "Spectate"),
    Unavailable UMETA(DisplayName = "Unavailable")
};

USTRUCT(BlueprintType)
struct FLobbyData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    int32 ID = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    FString Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    FString Class;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    FString Date;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    FString Time;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    int32 CurrentSpots = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    int32 MaxSpots = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    FString Entry;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    ELobbyType LobbyType = ELobbyType::Lobby;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    FString Track;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    ELobbyStatus Status = ELobbyStatus::Open;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    TArray<ELobbyTag> Tags;

    FLobbyData()
    {
        Title = TEXT("");
        Class = TEXT("");
        Date = TEXT("");
        Time = TEXT("");
        Entry = TEXT("");
        Track = TEXT("");
    }
};

UCLASS(BlueprintType)
class GRIT_API ULobbyDataObject : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "Lobby Data")
    FLobbyData LobbyData;

    UPROPERTY(BlueprintReadOnly, Category = "Lobby Data")
    int32 Index;

    UPROPERTY(BlueprintReadOnly, Category = "Lobby Data")
    bool bPlayerWasInLobby = false;
};