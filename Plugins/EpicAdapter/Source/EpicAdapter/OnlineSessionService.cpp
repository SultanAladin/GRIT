// OnlineSessionService.cpp
#include "OnlineSessionService.h"
#include "eos_sessions_types.h"
#include "eos_common.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogOnlineSession, Log, All);

/* ---------- Life-Cycle ---------- */

void UOnlineSessionService::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    ServiceLauncher = GEngine->GetEngineSubsystem<UOnlineServiceLauncher>();
    Auth            = GetGameInstance()->GetSubsystem<UOnlineSessionAuthenticator>();

    if (!InitEOSHandles())
    {
        UE_LOG(LogOnlineSession, Error, TEXT("Failed to initialize EOS Sessions"));
    }
}

bool UOnlineSessionService::InitEOSHandles()
{
    if (!ServiceLauncher || !Auth) return false;

    EOS_HPlatform Platform = ServiceLauncher->GetPlatformHandle();
    if (!Platform) return false;

    SessionsHandle = EOS_Platform_GetSessionsInterface(Platform);
    return SessionsHandle != nullptr;
}

void UOnlineSessionService::Deinitialize()
{
    CleanupSearchHandle();
    Super::Deinitialize();
}

/* ---------- Create Session ---------- */

bool UOnlineSessionService::CreateSession(const FString& SessionName,
                                          int32 MaxPlayers,
                                          bool bPresence,
                                          bool bAllowInvites,
                                          const FString& BucketId)
{
    if (!SessionsHandle || !Auth->GetAuthenticatedProductUserId()) return false;

    EOS_Sessions_CreateSessionModificationOptions ModOpt = {};
    ModOpt.ApiVersion     = EOS_SESSIONS_CREATESESSIONMODIFICATION_API_LATEST;
    ModOpt.SessionName    = TCHAR_TO_ANSI(*SessionName);
    ModOpt.MaxPlayers     = MaxPlayers;
    ModOpt.LocalUserId    = Auth->GetAuthenticatedProductUserId();
    ModOpt.bPresenceEnabled = bPresence ? EOS_TRUE : EOS_FALSE;
    ModOpt.BucketId       = TCHAR_TO_ANSI(*BucketId);

    EOS_HSessionModification ModHandle = nullptr;
    if (EOS_Sessions_CreateSessionModification(SessionsHandle, &ModOpt, &ModHandle) != EOS_EResult::EOS_Success)
        return false;

    // Basic attribute: allow invites
    EOS_SessionModification_SetInvitesAllowedOptions InvOpt = {};
    InvOpt.ApiVersion = EOS_SESSIONMODIFICATION_SETINVITESALLOWED_API_LATEST;
    InvOpt.bInvitesAllowed = bAllowInvites ? EOS_TRUE : EOS_FALSE;
    EOS_SessionModification_SetInvitesAllowed(ModHandle, &InvOpt);

    EOS_Sessions_UpdateSessionOptions UpdateOpt = {};
    UpdateOpt.ApiVersion = EOS_SESSIONS_UPDATESESSION_API_LATEST;
    UpdateOpt.SessionModificationHandle = ModHandle;

    EOS_Sessions_UpdateSession(SessionsHandle, &UpdateOpt, this, &UOnlineSessionService::OnCreateSessionCB);
    EOS_SessionModification_Release(ModHandle);
    return true;
}

void EOS_CALL UOnlineSessionService::OnCreateSessionCB(const EOS_Sessions_UpdateSessionCallbackInfo* Data)
{
    auto* Self = static_cast<UOnlineSessionService*>(Data->ClientData);
    const bool bOk = (Data->ResultCode == EOS_EResult::EOS_Success);
    UE_LOG(LogOnlineSession, Log, TEXT("CreateSession result: %s"),
           bOk ? TEXT("SUCCESS") : UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
    Self->OnSessionCreated.Broadcast(bOk, UTF8_TO_TCHAR(Data->SessionName));
}

/* ---------- Find Sessions ---------- */

void UOnlineSessionService::FindSessions(const FString& BucketId, int32 MaxResults)
{
    if (!SessionsHandle) return;

    CleanupSearchHandle();

    // 1) Create search handle
    EOS_Sessions_CreateSessionSearchOptions SearchOpt = {};
    SearchOpt.ApiVersion       = EOS_SESSIONS_CREATESESSIONSEARCH_API_LATEST;
    SearchOpt.MaxSearchResults = FMath::Clamp(MaxResults, 1, 200);

    EOS_EResult Result = EOS_Sessions_CreateSessionSearch(
        SessionsHandle, &SearchOpt, &SearchHandle);

    if (Result != EOS_EResult::EOS_Success || !SearchHandle)
    {
        UE_LOG(LogOnlineSession, Error, TEXT("CreateSessionSearch failed: %s"),
               UTF8_TO_TCHAR(EOS_EResult_ToString(Result)));
        return;
    }

    // 2) Optional bucket filter
    if (!BucketId.IsEmpty())
    {
        EOS_Sessions_AttributeData BucketAttr = {};
        BucketAttr.ApiVersion = EOS_SESSIONS_ATTRIBUTEDATA_API_LATEST;
        BucketAttr.Key        = "bucket";
        BucketAttr.ValueType  = EOS_ESessionAttributeType::EOS_SAT_String;
        BucketAttr.Value.AsUtf8 = TCHAR_TO_ANSI(*BucketId);

        EOS_SessionSearch_SetParameterOptions ParamOpt = {};
        ParamOpt.ApiVersion = EOS_SESSIONSEARCH_SETPARAMETER_API_LATEST;
        ParamOpt.Parameter  = &BucketAttr;
        ParamOpt.ComparisonOp = EOS_EOnlineComparisonOp::EOS_CO_EQUAL;

        EOS_SessionSearch_SetParameter(SearchHandle, &ParamOpt);
    }

    // 3) Execute search
    EOS_SessionSearch_FindOptions FindOpt = {};
    FindOpt.ApiVersion = EOS_SESSIONSEARCH_FIND_API_LATEST;
    FindOpt.LocalUserId = Auth->GetAuthenticatedProductUserId();

    EOS_SessionSearch_Find(SearchHandle, &FindOpt, this,
                           &UOnlineSessionService::OnFindSessionsCB);
}

void EOS_CALL UOnlineSessionService::OnFindSessionsCB(
    const EOS_SessionSearch_FindCallbackInfo* Data)
{
    UOnlineSessionService* Self = static_cast<UOnlineSessionService*>(Data->ClientData);
    bool bOk = (Data->ResultCode == EOS_EResult::EOS_Success);
    TArray<FString> SessionIds;

    if (bOk && Self->SearchHandle)
    {
        EOS_SessionSearch_GetSearchResultCountOptions CountOpt = {};
        CountOpt.ApiVersion = EOS_SESSIONSEARCH_GETSEARCHRESULTCOUNT_API_LATEST;

        uint32 Count = EOS_SessionSearch_GetSearchResultCount(Self->SearchHandle, &CountOpt);

        for (uint32 i = 0; i < Count; ++i)
        {
            EOS_SessionSearch_CopySearchResultByIndexOptions CopyOpt = {};
            CopyOpt.ApiVersion = EOS_SESSIONSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST;
            CopyOpt.SessionIndex = i;

            EOS_HSessionDetails Details = nullptr;
            if (EOS_SessionSearch_CopySearchResultByIndex(
                    Self->SearchHandle, &CopyOpt, &Details) == EOS_EResult::EOS_Success)
            {
                Self->FoundSessions.Add(Details);

                EOS_SessionDetails_CopyInfoOptions InfoOpt = {};
                InfoOpt.ApiVersion = EOS_SESSIONDETAILS_COPYINFO_API_LATEST;
                EOS_SessionDetails_Info* Info = nullptr;
                if (EOS_SessionDetails_CopyInfo(Details, &InfoOpt, &Info) == EOS_EResult::EOS_Success)
                {
                    SessionIds.Add(UTF8_TO_TCHAR(Info->SessionId));
                    EOS_SessionDetails_Info_Release(Info);
                }
            }
        }
    }

    Self->OnSessionSearchDone.Broadcast(bOk, SessionIds);
}

/* ---------- Join Session ---------- */

void UOnlineSessionService::JoinSession(int32 SearchResultIndex, const FString& SessionName)
{
    if (!SessionsHandle || !FoundSessions.IsValidIndex(SearchResultIndex)) return;

    EOS_Sessions_JoinSessionOptions JoinOpt = {};
    JoinOpt.ApiVersion      = EOS_SESSIONS_JOINSESSION_API_LATEST;
    JoinOpt.SessionHandle   = FoundSessions[SearchResultIndex];
    JoinOpt.SessionName     = TCHAR_TO_ANSI(*SessionName);
    JoinOpt.LocalUserId     = Auth->GetAuthenticatedProductUserId();
    JoinOpt.bPresenceEnabled = EOS_TRUE;

    EOS_Sessions_JoinSession(SessionsHandle, &JoinOpt, this, &UOnlineSessionService::OnJoinSessionCB);
}

void EOS_CALL UOnlineSessionService::OnJoinSessionCB(const EOS_Sessions_JoinSessionCallbackInfo* Data)
{
    auto* Self = static_cast<UOnlineSessionService*>(Data->ClientData);
    bool bOk = (Data->ResultCode == EOS_EResult::EOS_Success);
    UE_LOG(LogOnlineSession, Log, TEXT("JoinSession result: %s"),
           bOk ? TEXT("SUCCESS") : UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
    Self->OnSessionJoined.Broadcast(bOk, UTF8_TO_TCHAR("Game"));
}

/* ---------- Leave / Destroy Session ---------- */

void UOnlineSessionService::LeaveSession(const FString& SessionName)
{
    if (!SessionsHandle) return;

    EOS_Sessions_DestroySessionOptions Opts = {};
    Opts.ApiVersion = EOS_SESSIONS_DESTROYSESSION_API_LATEST;
    Opts.SessionName = TCHAR_TO_ANSI(*SessionName);
    EOS_Sessions_DestroySession(SessionsHandle, &Opts, this, &UOnlineSessionService::OnLeaveSessionCB);
}

void EOS_CALL UOnlineSessionService::OnLeaveSessionCB(const EOS_Sessions_DestroySessionCallbackInfo* Data)
{
    UE_LOG(LogOnlineSession, Log, TEXT("LeaveSession result: %s"),
           UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
}

/* ---------- Utility ---------- */

void UOnlineSessionService::CleanupSearchHandle()
{
    for (EOS_HSessionDetails Handle : FoundSessions)
        EOS_SessionDetails_Release(Handle);
    FoundSessions.Empty();

    if (SearchHandle)
    {
        EOS_SessionSearch_Release(SearchHandle);
        SearchHandle = nullptr;
    }
}