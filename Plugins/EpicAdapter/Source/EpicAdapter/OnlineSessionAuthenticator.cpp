// OnlineSessionAuthenticator.cpp
#include "OnlineSessionAuthenticator.h"
#include "OnlineServiceLauncher.h"
#include "eos_auth.h"
#include "eos_connect.h"
#include "eos_userinfo.h"
#include <string>

DEFINE_LOG_CATEGORY_STATIC(LogAuthenticator, Log, All);

/* ---------- Construction / Initialisation ---------- */

UOnlineSessionAuthenticator::UOnlineSessionAuthenticator()
    : ServiceLauncher(nullptr)
    , AuthHandle(nullptr)
    , ConnectHandle(nullptr)
    , AuthenticatedEpicAccountId(nullptr)
    , AuthenticatedProductUserId(nullptr)
{}

void UOnlineSessionAuthenticator::Initialize(FSubsystemCollectionBase& Collection)
{
    ServiceLauncher = GEngine->GetEngineSubsystem<UOnlineServiceLauncher>();
    if (!ServiceLauncher)
    {
        UE_LOG(LogAuthenticator, Error, TEXT("Failed to find OnlineServiceLauncher subsystem"));
        return;
    }
    InitializeAuthContext();
    UE_LOG(LogAuthenticator, Log, TEXT("OnlineSessionAuthenticator initialized"));
}

void UOnlineSessionAuthenticator::Deinitialize()
{
    EOS_ELoginStatus CurrentStatus = GetEOSLoginStatus();
    if (CurrentStatus == EOS_ELoginStatus::EOS_LS_LoggedIn &&
        AuthHandle && !AuthenticatedLocalUserId.IsEmpty())
    {
        EOS_Auth_LogoutOptions Opts = {};
        Opts.ApiVersion = EOS_AUTH_LOGOUT_API_LATEST;
        EOS_EpicAccountId Id = ConvertStringToEpicAccountId(AuthenticatedLocalUserId);
        Opts.LocalUserId = Id;
        EOS_Auth_Logout(AuthHandle, &Opts, nullptr, nullptr);
    }

    AuthenticatedLocalUserId.Empty();
    AuthenticatedProductId.Empty();
    AuthenticatedPlayerNickname.Empty();
    AuthenticatedNetId.Empty();
    AuthenticatedEpicAccountId = nullptr;
    AuthenticatedProductUserId = nullptr;
    AuthHandle   = nullptr;
    ConnectHandle = nullptr;
    ServiceLauncher = nullptr;
    UE_LOG(LogAuthenticator, Log, TEXT("OnlineSessionAuthenticator deinitialized"));
}

/* ---------- Auth Context ---------- */

void UOnlineSessionAuthenticator::InitializeAuthContext()
{
    if (!ServiceLauncher) return;

    EOS_HPlatform Platform = ServiceLauncher->GetPlatformHandle();
    if (!Platform)
    {
        UE_LOG(LogAuthenticator, Error, TEXT("PlatformHandle null"));
        return;
    }

    AuthHandle = EOS_Platform_GetAuthInterface(Platform);
    if (!AuthHandle) UE_LOG(LogAuthenticator, Error, TEXT("Failed to get EOS Auth interface"));
}

/* ---------- Login Methods ---------- */

void UOnlineSessionAuthenticator::LoginWithAccountPortal()
{
    EOS_HPlatform Platform = ServiceLauncher ? ServiceLauncher->GetPlatformHandle() : nullptr;
    if (!Platform) return;

    AuthHandle = EOS_Platform_GetAuthInterface(Platform);
    if (!AuthHandle) return;

    EOS_Auth_Credentials Creds = {};
    Creds.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
    Creds.Type       = EOS_ELoginCredentialType::EOS_LCT_AccountPortal;

    EOS_Auth_LoginOptions Opts = {};
    Opts.ApiVersion  = EOS_AUTH_LOGIN_API_LATEST;
    Opts.Credentials = &Creds;
    Opts.ScopeFlags  = EOS_EAuthScopeFlags::EOS_AS_BasicProfile |
                       EOS_EAuthScopeFlags::EOS_AS_FriendsList  |
                       EOS_EAuthScopeFlags::EOS_AS_Presence;

    EOS_Auth_Login(AuthHandle, &Opts, this, &UOnlineSessionAuthenticator::ProcessAccountPortalLogin);
}

void EOS_CALL UOnlineSessionAuthenticator::ProcessAccountPortalLogin(const EOS_Auth_LoginCallbackInfo* Data)
{
    if (!Data)
    {
        UE_LOG(LogAuthenticator, Error, TEXT("ProcessAccountPortalLogin received null data"));
        return;
    }

    UOnlineSessionAuthenticator* Auth = static_cast<UOnlineSessionAuthenticator*>(Data->ClientData);
    if (!Auth || !IsValid(Auth))
    {
        UE_LOG(LogAuthenticator, Error, TEXT("ProcessAccountPortalLogin: ClientData is null or invalid"));
        return;
    }

    // Additional safety check - ensure the object is still valid
    if (Auth->IsUnreachable() || Auth->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
    {
        UE_LOG(LogAuthenticator, Error, TEXT("ProcessAccountPortalLogin: Auth object is being destroyed"));
        return;
    }

    if (Data->ResultCode == EOS_EResult::EOS_Success)
    {
        /* ----- SUCCESS ----- */
        if (Auth->OnLoginSuccess.IsBound())
        {
            Auth->OnLoginSuccess.Broadcast();
        }

        /* Immediately chain Connect & details */
        Auth->LoginWithConnectInterface();
        Auth->RetrieveAccountCredentials(Data->LocalUserId);
    }
    else
    {
        /* ----- FAILURE ----- */
        UE_LOG(LogAuthenticator, Warning, TEXT("Account Portal login failed: %s"), 
               UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
        
        if (Auth->OnLoginFailure.IsBound())
        {
            Auth->OnLoginFailure.Broadcast(static_cast<int32>(Data->ResultCode));
        }
    }
}

void UOnlineSessionAuthenticator::LoginWithPersistentAuth()
{
    EOS_HPlatform Platform = ServiceLauncher ? ServiceLauncher->GetPlatformHandle() : nullptr;
    if (!Platform) return;

    AuthHandle = EOS_Platform_GetAuthInterface(Platform);
    if (!AuthHandle) return;

    EOS_Auth_Credentials Creds = {};
    Creds.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
    Creds.Type       = EOS_ELoginCredentialType::EOS_LCT_PersistentAuth;

    EOS_Auth_LoginOptions Opts = {};
    Opts.ApiVersion  = EOS_AUTH_LOGIN_API_LATEST;
    Opts.Credentials = &Creds;

    EOS_Auth_Login(AuthHandle, &Opts, this, &UOnlineSessionAuthenticator::ProcessPersistentAuthLogin);
}

void EOS_CALL UOnlineSessionAuthenticator::ProcessPersistentAuthLogin(const EOS_Auth_LoginCallbackInfo* Data)
{
    if (!Data)
    {
        UE_LOG(LogAuthenticator, Error, TEXT("Persistent Auth Callback: Null Data"));
        return;
    }

    UOnlineSessionAuthenticator* Auth = static_cast<UOnlineSessionAuthenticator*>(Data->ClientData);
    if (!Auth)
    {
        UE_LOG(LogAuthenticator, Error, TEXT("Persistent Auth Callback: Null Authenticator"));
        return;
    }

    switch (Data->ResultCode)
    {
        case EOS_EResult::EOS_Success:
            /* ----- SUCCESS ----- */
            Auth->OnLoginSuccess.Broadcast();                   // NEW
            Auth->LoginWithConnectInterface();
            Auth->RetrieveAccountCredentials(Data->LocalUserId);
            break;

        case EOS_EResult::EOS_Auth_Expired:
        case EOS_EResult::EOS_InvalidAuth:
        case EOS_EResult::EOS_InvalidUser:
            /* ----- FAILURE (retry via Portal) ----- */
            Auth->OnLoginFailure.Broadcast(static_cast<int32>(Data->ResultCode)); // NEW
            UE_LOG(LogAuthenticator, Error, TEXT("Persistent Auth failed: %s"),
                   UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
            Auth->LoginWithAccountPortal();
            break;

        default:
            /* ----- FAILURE (no retry) ----- */
            Auth->OnLoginFailure.Broadcast(static_cast<int32>(Data->ResultCode)); // NEW
            UE_LOG(LogAuthenticator, Error, TEXT("Persistent Auth unhandled error: %s"),
                   UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
            break;
    }
}

void UOnlineSessionAuthenticator::LoginWithConnectInterface()
{
    EOS_HPlatform Platform = ServiceLauncher ? ServiceLauncher->GetPlatformHandle() : nullptr;
    if (!Platform) return;

    AuthHandle = EOS_Platform_GetAuthInterface(Platform);
    if (!AuthHandle) return;

    EOS_EpicAccountId EpicId = EOS_Auth_GetLoggedInAccountByIndex(AuthHandle, 0);
    if (!EpicId) return;

    EOS_Auth_CopyUserAuthTokenOptions TokenOpts = {};
    TokenOpts.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

    EOS_Auth_Token* Token = nullptr;
    if (EOS_Auth_CopyUserAuthToken(AuthHandle, &TokenOpts, EpicId, &Token) != EOS_EResult::EOS_Success)
        return;

    EOS_Connect_Credentials Creds = {};
    Creds.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
    Creds.Token      = Token->AccessToken;
    Creds.Type       = EOS_EExternalCredentialType::EOS_ECT_EPIC;

    EOS_Connect_LoginOptions Opts = {};
    Opts.ApiVersion  = EOS_CONNECT_LOGIN_API_LATEST;
    Opts.Credentials = &Creds;

    ConnectHandle = EOS_Platform_GetConnectInterface(Platform);
    if (!ConnectHandle) { EOS_Auth_Token_Release(Token); return; }

    EOS_Connect_Login(ConnectHandle, &Opts, this,
        [](const EOS_Connect_LoginCallbackInfo* Data)
        {
            auto* Auth = static_cast<UOnlineSessionAuthenticator*>(Data->ClientData);
            if (Auth) Auth->ProcessConnectLogin(Data);
        });

    EOS_Auth_Token_Release(Token);
}

void EOS_CALL UOnlineSessionAuthenticator::ProcessConnectLogin(const EOS_Connect_LoginCallbackInfo* Data)
{
    if (!Data)
    {
        UE_LOG(LogAuthenticator, Error, TEXT("ProcessConnectLogin: Received null callback data"));
        return;
    }

    UOnlineSessionAuthenticator* Auth = static_cast<UOnlineSessionAuthenticator*>(Data->ClientData);
    if (!Auth)
    {
        UE_LOG(LogAuthenticator, Error, TEXT("ProcessConnectLogin: Invalid authenticator pointer"));
        return;
    }

    if (Data->ResultCode == EOS_EResult::EOS_Success)
    {
        /* ----- SUCCESS ----- */
        Auth->OnLoginSuccess.Broadcast();                   // NEW

        /* Store Product User ID & map */
        Auth->AuthenticatedProductUserId = Data->LocalUserId;

        char Buf[EOS_PRODUCTUSERID_MAX_LENGTH + 1] = {0};
        int32 Len = sizeof(Buf);
        EOS_ProductUserId_ToString(Data->LocalUserId, Buf, &Len);
        Auth->AuthenticatedProductId = UTF8_TO_TCHAR(Buf);

        Auth->MapProductUserId(Data->LocalUserId);
    }
    else
    {
        /* ----- FAILURE ----- */
        Auth->OnLoginFailure.Broadcast(static_cast<int32>(Data->ResultCode)); // NEW
        UE_LOG(LogAuthenticator, Warning, TEXT("Connect login failed: %s"),
               UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
    }
}

/* ---------- Logout / Delete Persistent ---------- */

void UOnlineSessionAuthenticator::Logout()
{
    if (!AuthHandle || AuthenticatedLocalUserId.IsEmpty()) return;

    DeletePersistentAuth();

    EOS_Auth_LogoutOptions Opts = {};
    Opts.ApiVersion = EOS_AUTH_LOGOUT_API_LATEST;
    EOS_EpicAccountId Id = ConvertStringToEpicAccountId(AuthenticatedLocalUserId);
    Opts.LocalUserId = Id;

    EOS_Auth_Logout(AuthHandle, &Opts, this, &UOnlineSessionAuthenticator::ProcessLogout);
}

void EOS_CALL UOnlineSessionAuthenticator::ProcessLogout(const EOS_Auth_LogoutCallbackInfo* Data)
{
    if (!Data)
    {
        UE_LOG(LogAuthenticator, Error, TEXT("Logout Callback: Null Data"));
        return;
    }

    UOnlineSessionAuthenticator* Auth = static_cast<UOnlineSessionAuthenticator*>(Data->ClientData);
    if (!Auth)
    {
        UE_LOG(LogAuthenticator, Error, TEXT("Logout Callback: Null Authenticator"));
        return;
    }

    if (Data->ResultCode == EOS_EResult::EOS_Success)
    {
        /* ----- SUCCESS ----- */
        UE_LOG(LogAuthenticator, Log, TEXT("Logout Successful"));
        Auth->OnLogoutSuccess.Broadcast();                  // NEW

        /* Clear stored info */
        Auth->AuthenticatedLocalUserId.Empty();
        Auth->AuthenticatedProductId.Empty();
        Auth->AuthenticatedPlayerNickname.Empty();
        Auth->AuthenticatedNetId.Empty();
        Auth->AuthenticatedEpicAccountId = nullptr;
        Auth->AuthenticatedProductUserId = nullptr;
    }
    else
    {
        /* ----- FAILURE ----- */
        UE_LOG(LogAuthenticator, Warning, TEXT("Logout Failed: %s"),
               UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
        Auth->OnLogoutFailure.Broadcast();                  // NEW
    }
}

void UOnlineSessionAuthenticator::DeletePersistentAuth()
{
    if (!AuthHandle) return;

    EOS_Auth_DeletePersistentAuthOptions Opts = {};
    Opts.ApiVersion = EOS_AUTH_DELETEPERSISTENTAUTH_API_LATEST;
    Opts.RefreshToken = nullptr;

    EOS_Auth_DeletePersistentAuth(AuthHandle, &Opts, this,
                                  &UOnlineSessionAuthenticator::ProcessDeletePersistentAuth);
}

void EOS_CALL UOnlineSessionAuthenticator::ProcessDeletePersistentAuth(
    const EOS_Auth_DeletePersistentAuthCallbackInfo* Data)
{
    if (!Data) return;
    auto* Auth = static_cast<UOnlineSessionAuthenticator*>(Data->ClientData);
    if (!Auth) return;

    UE_LOG(LogAuthenticator, Log, TEXT("DeletePersistentAuth result: %s"),
           UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
}

/* ---------- User Details Query ---------- */

void UOnlineSessionAuthenticator::RetrieveAccountCredentials(EOS_EpicAccountId UserId)
{
    if (!ServiceLauncher) return;

    EOS_HPlatform Platform = ServiceLauncher->GetPlatformHandle();
    EOS_HUserInfo UserInfo = EOS_Platform_GetUserInfoInterface(Platform);
    if (!UserInfo) return;

    EOS_UserInfo_QueryUserInfoOptions Opts = {};
    Opts.ApiVersion  = EOS_USERINFO_QUERYUSERINFO_API_LATEST;
    Opts.LocalUserId = UserId;
    Opts.TargetUserId = UserId;

    EOS_UserInfo_QueryUserInfo(UserInfo, &Opts, this,
                               &UOnlineSessionAuthenticator::OnRetrievedUserDetails);
}

void EOS_CALL UOnlineSessionAuthenticator::OnRetrievedUserDetails(
    const EOS_UserInfo_QueryUserInfoCallbackInfo* Data)
{
    if (!Data) return;
    auto* Auth = static_cast<UOnlineSessionAuthenticator*>(Data->ClientData);
    if (!Auth) return;

    if (Data->ResultCode != EOS_EResult::EOS_Success) return;

    EOS_HPlatform Platform = Auth->ServiceLauncher->GetPlatformHandle();
    EOS_HUserInfo UserInfo = EOS_Platform_GetUserInfoInterface(Platform);

    EOS_UserInfo_CopyUserInfoOptions Opts = {};
    Opts.ApiVersion  = EOS_USERINFO_COPYUSERINFO_API_LATEST;
    Opts.LocalUserId = Data->LocalUserId;
    Opts.TargetUserId = Data->TargetUserId;

    EOS_UserInfo* Info = nullptr;
    if (EOS_UserInfo_CopyUserInfo(UserInfo, &Opts, &Info) != EOS_EResult::EOS_Success || !Info)
        return;

    Auth->AuthenticatedPlayerNickname =
        (Info->DisplayName && strlen(Info->DisplayName) > 0)
            ? UTF8_TO_TCHAR(Info->DisplayName)
            : TEXT("Player");

    UE_LOG(LogAuthenticator, Log, TEXT("Player Nickname Retrieved: %s"),
           *Auth->AuthenticatedPlayerNickname);

    if (Auth->AuthenticatedLocalUserId.IsEmpty() && Data->LocalUserId)
    {
        char Buf[EOS_EPICACCOUNTID_MAX_LENGTH + 1] = {0};
        int32 Len = sizeof(Buf);
        EOS_EpicAccountId_ToString(Data->LocalUserId, Buf, &Len);
        Auth->AuthenticatedLocalUserId = UTF8_TO_TCHAR(Buf);
    }

    EOS_UserInfo_Release(Info);

    /* >>> NEW <<< */
    UE_LOG(LogAuthenticator, Log, TEXT("Broadcasting OnUserDetailsReady"));
    Auth->OnUserDetailsReady.Broadcast();
}

/* ---------- Utility Conversions ---------- */

FString UOnlineSessionAuthenticator::ConvertEpicAccountIdToString(EOS_EpicAccountId In)
{
    if (!In) return TEXT("NULL");
    char Buf[EOS_EPICACCOUNTID_MAX_LENGTH + 1] = {0};
    int32 Len = sizeof(Buf);
    return EOS_EpicAccountId_ToString(In, Buf, &Len) == EOS_EResult::EOS_Success
               ? UTF8_TO_TCHAR(Buf)
               : TEXT("ERROR");
}

FString UOnlineSessionAuthenticator::ConvertProductUserIdToString(EOS_ProductUserId In)
{
    if (!In) return TEXT("NULL");
    char Buf[EOS_PRODUCTUSERID_MAX_LENGTH + 1] = {0};
    int32 Len = sizeof(Buf);
    return EOS_ProductUserId_ToString(In, Buf, &Len) == EOS_EResult::EOS_Success
               ? UTF8_TO_TCHAR(Buf)
               : TEXT("ERROR");
}

EOS_EpicAccountId UOnlineSessionAuthenticator::ConvertStringToEpicAccountId(const FString& S)
{
    return S.IsEmpty() ? nullptr : EOS_EpicAccountId_FromString(TCHAR_TO_UTF8(*S));
}

EOS_ProductUserId UOnlineSessionAuthenticator::ConvertStringToProductUserId(const FString& S)
{
    return S.IsEmpty() ? nullptr : EOS_ProductUserId_FromString(TCHAR_TO_UTF8(*S));
}
/*
EOS_EpicAccountId UOnlineSessionAuthenticator::GetAuthenticatedEpicAccountId()
{
    return AuthenticatedEpicAccountId;
}
*/
EOS_ProductUserId UOnlineSessionAuthenticator::GetAuthenticatedProductUserId()
{
    return AuthenticatedProductUserId;
}

/* ---------- Retrieval ---------- */

EOS_EpicAccountId UOnlineSessionAuthenticator::GetLocalEpicAccountId()
{
    if (!AuthHandle) return nullptr;
    int32 Count = EOS_Auth_GetLoggedInAccountsCount(AuthHandle);
    return Count > 0 ? EOS_Auth_GetLoggedInAccountByIndex(AuthHandle, 0) : nullptr;
}

EOS_ProductUserId UOnlineSessionAuthenticator::GetLocalProductUserId()
{
    return AuthenticatedProductUserId;
}

FString UOnlineSessionAuthenticator::GetPlatformAuthToken()
{
    EOS_EpicAccountId Id = GetLocalEpicAccountId();
    if (!Id || !AuthHandle) return FString();

    EOS_Auth_Token* Token = nullptr;
    EOS_Auth_CopyUserAuthTokenOptions Opts = {};
    Opts.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

    if (EOS_Auth_CopyUserAuthToken(AuthHandle, &Opts, Id, &Token) == EOS_EResult::EOS_Success && Token)
    {
        FString Out = UTF8_TO_TCHAR(Token->AccessToken);
        EOS_Auth_Token_Release(Token);
        return Out;
    }
    return FString();
}

EOS_ELoginStatus UOnlineSessionAuthenticator::GetEOSLoginStatus()
{
    EOS_EpicAccountId Id = GetLocalEpicAccountId();
    if (!Id || !AuthHandle) return EOS_ELoginStatus::EOS_LS_NotLoggedIn;
    return EOS_Auth_GetLoginStatus(AuthHandle, Id);
}

void UOnlineSessionAuthenticator::RetrieveAuthenticationDetails(FString& OutEpic,
                                                                FString& OutProduct,
                                                                FString& OutToken,
                                                                FString& OutStatus)
{
    OutEpic   = ConvertEpicAccountIdToString(GetLocalEpicAccountId());
    OutProduct= ConvertProductUserIdToString(GetLocalProductUserId());
    OutToken  = GetPlatformAuthToken();
    if (OutToken.Len() > 8)
        OutToken = OutToken.Left(4) + TEXT("...") + OutToken.Right(4) + TEXT(" (") + FString::FromInt(OutToken.Len()) + TEXT(" chars)");

    EOS_ELoginStatus S = GetEOSLoginStatus();
    switch (S)
    {
        case EOS_ELoginStatus::EOS_LS_NotLoggedIn:     OutStatus = TEXT("Not Logged In"); break;
        case EOS_ELoginStatus::EOS_LS_UsingLocalProfile:OutStatus = TEXT("Using Local Profile"); break;
        case EOS_ELoginStatus::EOS_LS_LoggedIn:        OutStatus = TEXT("Logged In"); break;
        default:                                       OutStatus = TEXT("Unknown"); break;
    }
}

bool UOnlineSessionAuthenticator::MapProductUserId(EOS_ProductUserId ProductUserId)
{
    if (!ProductUserId || !ConnectHandle) return false;

    EOS_ProductUserId LocalId = EOS_Connect_GetLoggedInUserByIndex(ConnectHandle, 0);
    if (!LocalId) return false;

    char External[EOS_EPICACCOUNTID_MAX_LENGTH + 1] = {0};
    int32 Len = sizeof(External);

    EOS_Connect_GetProductUserIdMappingOptions Opts = {};
    Opts.ApiVersion       = EOS_CONNECT_GETPRODUCTUSERIDMAPPING_API_LATEST;
    Opts.LocalUserId      = LocalId;
    Opts.AccountIdType    = EOS_EExternalAccountType::EOS_EAT_EPIC;
    Opts.TargetProductUserId = ProductUserId;

    if (EOS_Connect_GetProductUserIdMapping(ConnectHandle, &Opts, External, &Len) == EOS_EResult::EOS_Success)
    {
        AuthenticatedNetId = UTF8_TO_TCHAR(External);
        return true;
    }
    return false;
}