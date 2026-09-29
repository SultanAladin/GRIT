// OnlineSessionAuthenticator.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnlineServiceLauncher.h"
#include "eos_sdk.h"
#include "OnlineSessionAuthenticator.generated.h"

class UOnlineServiceLauncher;

/* --------------------------------------------------------------------------
   Delegates
-------------------------------------------------------------------------- */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOUserDetailsReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOLoginSuccessDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOLoginFailureDelegate, int32, ErrorCode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOLogoutSuccessDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOLogoutFailureDelegate);

UCLASS()
class EPICADAPTER_API UOnlineSessionAuthenticator : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UOnlineSessionAuthenticator();

    /* ---------- Subsystem life-cycle ---------- */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /* ---------- Authentication Context ---------- */
    void InitializeAuthContext();

    /* ---------- Authentication Methods ---------- */
    UFUNCTION(BlueprintCallable, Category = "Online|Authentication")
    void LoginWithAccountPortal();

    UFUNCTION(BlueprintCallable, Category = "Online|Authentication")
    void LoginWithPersistentAuth();

    UFUNCTION(BlueprintCallable, Category = "Online|Authentication")
    void LoginWithConnectInterface();

    UFUNCTION(BlueprintCallable, Category = "Online|Authentication")
    void DeletePersistentAuth();

    UFUNCTION(BlueprintCallable, Category = "Online|Authentication")
    void Logout();

    /* ---------- Authentication Information ---------- */
    UFUNCTION(BlueprintCallable, Category = "Online|Authentication")
    void RetrieveAuthenticationDetails(FString& OutEpicAccountId,
                                       FString& OutProductUserId,
                                       FString& OutAuthToken,
                                       FString& OutLoginStatus);

    FString GetPlayerNickname() const { return AuthenticatedPlayerNickname; }

    EOS_EpicAccountId GetLocalEpicAccountId();
    EOS_ProductUserId GetLocalProductUserId();
    FString         GetPlatformAuthToken();
    EOS_ELoginStatus GetEOSLoginStatus();

    /* ---------- Public getters for external code ---------- */
    EOS_ProductUserId GetAuthenticatedProductUserId();

    /* ---------- Delegates ---------- */
    UPROPERTY(BlueprintAssignable, Category = "Online|Authentication")
    FOUserDetailsReady  OnUserDetailsReady;

    UPROPERTY(BlueprintAssignable, Category = "Online|Authentication")
    FOLoginSuccessDelegate  OnLoginSuccess;

    UPROPERTY(BlueprintAssignable, Category = "Online|Authentication")
    FOLoginFailureDelegate  OnLoginFailure;

    UPROPERTY(BlueprintAssignable, Category = "Online|Authentication")
    FOLogoutSuccessDelegate OnLogoutSuccess;

    UPROPERTY(BlueprintAssignable, Category = "Online|Authentication")
    FOLogoutFailureDelegate OnLogoutFailure;

       void RetrieveAccountCredentials(EOS_EpicAccountId UserId);

protected:
    /* ---------- Callbacks (C-style EOS callbacks) ---------- */
    static void EOS_CALL ProcessAccountPortalLogin(const EOS_Auth_LoginCallbackInfo* Data);
    static void EOS_CALL ProcessPersistentAuthLogin(const EOS_Auth_LoginCallbackInfo* Data);
    static void EOS_CALL ProcessConnectLogin(const EOS_Connect_LoginCallbackInfo* Data);
    static void EOS_CALL ProcessLogout(const EOS_Auth_LogoutCallbackInfo* Data);
    static void EOS_CALL ProcessDeletePersistentAuth(const EOS_Auth_DeletePersistentAuthCallbackInfo* Data);
    static void EOS_CALL OnRetrievedUserDetails(const EOS_UserInfo_QueryUserInfoCallbackInfo* Data);

 
    bool MapProductUserId(EOS_ProductUserId ProductUserId);

private:
    /* ---------- Conversion helpers ---------- */
    static FString ConvertEpicAccountIdToString(EOS_EpicAccountId InAccountId);
    static FString ConvertProductUserIdToString(EOS_ProductUserId InAccountId);
    static EOS_EpicAccountId ConvertStringToEpicAccountId(const FString& AccountString);
    static EOS_ProductUserId ConvertStringToProductUserId(const FString& ProductUserIdString);

    /* ---------- Member variables ---------- */
    UPROPERTY()
    UOnlineServiceLauncher* ServiceLauncher;

    EOS_HAuth   AuthHandle;
    EOS_HConnect ConnectHandle;

    FString AuthenticatedLocalUserId;
    FString AuthenticatedProductId;
    FString AuthenticatedPlayerNickname;
    FString AuthenticatedNetId;

    EOS_EpicAccountId AuthenticatedEpicAccountId;
    EOS_ProductUserId AuthenticatedProductUserId;
};