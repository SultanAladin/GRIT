// OnlineSessionService.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnlineServiceLauncher.h"
#include "OnlineSessionAuthenticator.h"
#include "eos_sessions.h"
#include "OnlineSessionService.generated.h"

class UOnlineServiceLauncher;
class UOnlineSessionAuthenticator;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionCreated, bool, bSuccess, const FString&, SessionName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionJoined,  bool, bSuccess, const FString&, SessionName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionSearchDone, bool, bSuccess, const TArray<FString>&, SessionIds);

UCLASS(BlueprintType)
class EPICADAPTER_API UOnlineSessionService : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /* ---------- Core Game-Facing API ---------- */

    /** Creates a named session (the “host” flow). */
    UFUNCTION(BlueprintCallable, Category="Online|Session")
    bool CreateSession(const FString& SessionName,
                       int32 MaxPlayers = 10,
                       bool bPresence = true,
                       bool bAllowInvites = true,
                       const FString& BucketId = TEXT("Default"));

    /** Joins a session by search result index. */
    UFUNCTION(BlueprintCallable, Category="Online|Session")
    void JoinSession(int32 SearchResultIndex, const FString& SessionName = TEXT("Game"));

    /** Searches for joinable sessions. */
    UFUNCTION(BlueprintCallable, Category="Online|Session")
    void FindSessions(const FString& BucketId = TEXT("Default"), int32 MaxResults = 50);

    /** Leaves / destroys the local session. */
    UFUNCTION(BlueprintCallable, Category="Online|Session")
    void LeaveSession(const FString& SessionName = TEXT("Game"));

    /* ---------- Delegates ---------- */
    UPROPERTY(BlueprintAssignable, Category="Online|Session")
    FOnSessionCreated OnSessionCreated;

    UPROPERTY(BlueprintAssignable, Category="Online|Session")
    FOnSessionJoined  OnSessionJoined;

    UPROPERTY(BlueprintAssignable, Category="Online|Session")
    FOnSessionSearchDone OnSessionSearchDone;

private:
    /* ---------- Internal helpers / callbacks ---------- */
    bool InitEOSHandles();
    void CleanupSearchHandle();

    /* Static EOS callbacks */
    static void EOS_CALL OnCreateSessionCB(const EOS_Sessions_UpdateSessionCallbackInfo* Data);
    static void EOS_CALL OnJoinSessionCB   (const EOS_Sessions_JoinSessionCallbackInfo* Data);
    static void EOS_CALL OnFindSessionsCB  (const EOS_SessionSearch_FindCallbackInfo* Data);
    static void EOS_CALL OnLeaveSessionCB  (const EOS_Sessions_DestroySessionCallbackInfo* Data);

    /* ---------- Subsystem refs ---------- */
    UPROPERTY()
    UOnlineServiceLauncher* ServiceLauncher = nullptr;

    UPROPERTY()
    UOnlineSessionAuthenticator* Auth = nullptr;

    /* ---------- EOS handles ---------- */
    EOS_HSessions SessionsHandle = nullptr;
    EOS_HSessionSearch SearchHandle = nullptr;

    /* ---------- Cached search results ---------- */
    TArray<EOS_HSessionDetails> FoundSessions;
};