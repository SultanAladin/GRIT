// OnlineLobbyService.h
// A subsystem class for managing online lobbies in UE5
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnlineServiceLauncher.h"
#include "OnlineSessionAuthenticator.h"
#include "eos_lobby.h"

#include "eos_rtc.h"
#include "eos_rtc_audio.h"

#include "OnlineLobbyService.generated.h"



/** 
 * Permission level for lobbies - gets more restrictive further down 
 */
UENUM(BlueprintType)
enum class ELobbyPermissionLevel : uint8
{
    /** Anyone can find this lobby as long as it isn't full */
    PublicAdvertised = 0 UMETA(DisplayName = "Public Advertised"),
    
    /** Players who have access to presence can see this lobby */
    JoinViaPresence = 1 UMETA(DisplayName = "Join Via Presence"),
    
    /** Only players with invites registered can see this lobby */
    InviteOnly = 2 UMETA(DisplayName = "Invite Only")
};


/** Structure to track RTC state for lobby members */
USTRUCT(BlueprintType)
struct FLobbyMemberRTCState
{
    GENERATED_BODY()

    /** Is this person currently connected to the RTC room? */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby|RTC")
    bool bIsInRTCRoom = false;

    /** Is this person currently talking (audible sounds from their audio output) */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby|RTC")
    bool bIsTalking = false;

    /** We have locally muted this person (others can still hear them) */
    UPROPERTY(BlueprintReadWrite, Category = "Online|Lobby|RTC")
    bool bIsLocallyMuted = false;

    /** Is this person hard muted (muted for everyone in the lobby by lobby owner) */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby|RTC")
    bool bIsHardMuted = false;

    /** Has this person muted their own audio output (nobody can hear them) */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby|RTC")
    bool bIsAudioOutputDisabled = false;

    /** Are we currently muting this person? */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby|RTC")
    bool bMuteActionInProgress = false;

    /** Are we currently hard muting this person? */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby|RTC")
    bool bHardMuteActionInProgress = false;
};

/** Structure for lobby member data */
USTRUCT(BlueprintType)
struct FLobbyMember
{
    GENERATED_BODY()

    /** EOS Account ID */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    FString AccountId;

    /** EOS Product User ID */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    FString ProductUserId;

    /** Display name of the member */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    FString DisplayName;

    /** Whether the member is ready to start the game */
    UPROPERTY(BlueprintReadWrite, Category = "Online|Lobby")
    bool bIsReady = false;

    /** RTC related state information */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby|RTC")
    FLobbyMemberRTCState RTCState;

};


/** Structure for lobby data */
USTRUCT(BlueprintType)
struct FLobbyMetadata
{
    GENERATED_BODY()
    /** Unique identifier for the lobby */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    FString Id;
    /** Product User ID of the lobby owner */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    FString LobbyOwner;
    /** Account ID of the lobby owner */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    FString LobbyOwnerAccountId;
    /** Display name of the lobby owner */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    FString LobbyOwnerDisplayName;
    /** Bucket ID for lobby categorization */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    FString BucketId;
    /** Permission level for the lobby */
   /** Permission level for the lobby */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    ELobbyPermissionLevel Permission = ELobbyPermissionLevel::PublicAdvertised; // Add default value
    /** Members in the lobby */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    TArray<FLobbyMember> Members;
    /** Maximum number of members allowed */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    int32 MaxNumLobbyMembers = 0;
    /** Number of available slots */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    int32 AvailableSlots = 0;
    /** Whether invites are allowed */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    bool bAllowInvites = true;
    /** Whether the lobby is enabled in presence */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    bool bPresenceEnabled = false;
    /** Whether RTC room is enabled */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    bool bRTCRoomEnabled = false;
    /** RTC room name */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby|RTC")
    FString RTCRoomName;
    /** Whether we're connected to the RTC room */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby|RTC")
    bool bRTCRoomConnected = false;
    /** Is this a search result or active lobby */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    bool bSearchResult = false;
    /** Is this lobby currently being created */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    bool bBeingCreated = false;
    
    /** All string attributes */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby|Attributes")
    TMap<FString, FString> StringAttributes;
    /** All integer attributes */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby|Attributes")
    TMap<FString, int64> Int64Attributes;
    /** All boolean attributes */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby|Attributes")
    TMap<FString, bool> BoolAttributes;
    /** All float attributes */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby|Attributes")
    TMap<FString, float> FloatAttributes;
};

/** Structure for lobby invites */
USTRUCT(BlueprintType)
struct FLobbyInvite
{
    GENERATED_BODY()

    /** The lobby being invited to */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    FLobbyMetadata LobbyMetadata;

    /** Friend's product user ID */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    FString FriendId;

    /** Friend's Epic account ID */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    FString FriendEpicId;

    /** Friend's display name */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    FString FriendDisplayName;

    /** Unique identifier for the invite */
    UPROPERTY(BlueprintReadOnly, Category = "Online|Lobby")
    FString InviteId;
};

/** Delegate fired when a lobby is created */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLobbyCreatedDelegate, bool, bSuccess, const FString&, LobbyId);

/** Delegate fired when a lobby search is completed */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLobbySearchComplete, bool, bWasSuccessful, const TArray<FString>&, LobbyIds);

/**
 * OnlineLobbyService - Game Instance Subsystem
 * Handles creation, joining, and management of online game lobbies using EOS
 */
UCLASS()
class EPICADAPTER_API UOnlineLobbyService : public UGameInstanceSubsystem
{
    GENERATED_BODY()
    
public:
    /** Constructor */
    UOnlineLobbyService();
    
    /** USubsystem interface */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    //
    // Lobby Management
    //
    
  /* Creates a new lobby instance with the specified parameters */
void LogLobbyMetadata(const FLobbyMetadata& LobbyData);
FLobbyMetadata RetrieveLobbyMetadata(const FString& LobbyId, EOS_HLobbyDetails ExistingLobbyDetailsHandle, bool bReleaseHandle );

FLobbyMetadata RetrieveLobbyMetadata2(const FString& LobbyId,EOS_HLobbyDetails ExistingLobbyDetailsHandle);

UFUNCTION(BlueprintCallable, Category = "Online|Lobby")
bool OpenLobbyInstance(int32 MaxMembers /* Maximum number of players */, ELobbyPermissionLevel PermissionLevel /* Who can join */, bool bEnablePresence /* Show in friends' presence */, bool bAllowInvites /* Allow invites */, const FString& BucketId /* Categorization ID */, bool bAllowHostMigration /* Enable host migration */, bool bEnableRTCRoom /* Enable RTC for voice */, const TMap<FString, FString>& StringAttributes /* String attributes */, const TMap<FString, int64>& IntAttributes /* Integer attributes */, const TMap<FString, bool>& BoolAttributes /* Boolean attributes */, const TMap<FString, float>& FloatAttributes /* Float attributes */);

/* Leave the current lobby */
UFUNCTION(BlueprintCallable, Category = "Online|Lobby")
void LeaveLobby();

/* Destroys the current lobby instance */
UFUNCTION(BlueprintCallable, Category = "Online|Lobby")
bool TerminateCurrentLobbyInstance(); /* True if the request was sent successfully (host only) */

/* Kicks a member from the lobby */
void KickMember(EOS_ProductUserId MemberId); /* The Product User ID of the member to kick (host only) */

/* Promotes a member to host */
void PromoteMember(EOS_ProductUserId MemberId); /* The Product User ID of the member to promote (host only) */

/* Set attributes for the current lobby */
UFUNCTION(BlueprintCallable, Category = "Online|Lobby")
bool ApplyLobbyAttributes(const TMap<FString, FString>& StringAttributes /* String attributes */, const TMap<FString, int64>& IntAttributes /* Integer attributes */, const TMap<FString, bool>& BoolAttributes /* Boolean attributes */, const TMap<FString, float>& FloatAttributes /* Float attributes */); /* Set attributes for the current lobby, returns true if successful */


/* Search for lobbies with specified criteria */
UFUNCTION(BlueprintCallable, Category = "Online|Lobby")
void SearchAllLobbies(const FString& BucketId /* Filter by bucket ID */, const TMap<FString, FString>& StringAttributes /* String attributes to match */, const TMap<FString, int64>& IntAttributes /* Integer attributes to match */, const TMap<FString, bool>& BoolAttributes /* Boolean attributes to match */, const TMap<FString, float>& FloatAttributes /* Float attributes to match */, int32 MaxResults /* Max results to return */);

UFUNCTION(BlueprintCallable, Category = "Online|Lobby")
void JoinLobbyById(const FString& LobbyId);

UFUNCTION(BlueprintCallable, Category = "Online|Lobby")
void JoinLobby(int32 LobbyIndex);

UFUNCTION(BlueprintCallable, Category = "Online|Lobby")
bool SendLobbyInvite(const FString& RecipientProductUserId);

UFUNCTION(BlueprintCallable, Category = "Online|Lobby")
bool AcceptLobbyInvite(const FString& InviteId);

UFUNCTION(BlueprintCallable, Category = "Online|Lobby")
bool RejectLobbyInvite(const FString& InviteId);

    
    /** Fired when a lobby is created */
    UPROPERTY(BlueprintAssignable, Category = "Online|Lobby")
    FOnLobbyCreatedDelegate OnLobbyCreatedDelegate;
    
    /** Fired when a lobby search is completed */
    UPROPERTY(BlueprintAssignable, Category = "EOS|Lobby")
    FOnLobbySearchComplete OnLobbySearchCompleteDelegate;

      // Delegates
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLobbyInviteReceived, const FString&, InviteId, const FString&, SenderId);
    UPROPERTY(BlueprintAssignable, Category = "Online|Lobby")
    FOnLobbyInviteReceived OnLobbyInviteReceivedDelegate;
    
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInviteSent, bool, bSuccess);
    UPROPERTY(BlueprintAssignable, Category = "Online|Lobby")
    FOnInviteSent OnInviteSentDelegate;
    
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInviteRejected, bool, bSuccess, const FString&, InviteId);
    UPROPERTY(BlueprintAssignable, Category = "Online|Lobby")
    FOnInviteRejected OnInviteRejectedDelegate;
    public :

    //---------------------------------------------------------------------------//
    

    

//----------------------------------------------------------------------------------------------//
    //
    // Internal Methods
    //


     /** Initializes lobby interface */    
    bool InitializeLobbyInterface();
    
    /** Configure the lobby instance after creation */
    void ConfigureLobbyInstance();
    
    /** Process lobby search results */
    void ProcessLobbySearchResults();
    
    /** Set up notifications for lobby events */
    void SetupLobbyNotifications();
    
    /** Remove and clean up lobby notifications */
    void RemoveLobbyNotifications();

// Add these function declarations to your UOnlineLobbyService class in the header file
private:
    EOS_HLobbyDetails RetrieveLobbyDetailsHandle(const FString& LobbyId, EOS_ProductUserId LocalUserId);
    void RetrieveLobbyOwnerInfo(FLobbyMetadata& LobbyData, EOS_HLobbyDetails LobbyDetailsHandle);
    void RetrieveLobbyAttributes(FLobbyMetadata& LobbyData, EOS_HLobbyDetails LobbyDetailsHandle);
    void ProcessLobbyAttribute(FLobbyMetadata& LobbyData, EOS_HLobbyDetails LobbyDetailsHandle,  uint32 Index, uint32 MemberCount);
    void RetrieveLobbyMembers(FLobbyMetadata& LobbyData, EOS_HLobbyDetails LobbyDetailsHandle);
    
private:
    /** Authenticate the session before performing lobby operations */
    bool AuthenticateSession();
    
    //
    // Static Callbacks
    //
    
    /** Callback for when a lobby is created */
    static void EOS_CALL ProcessOpenedLobbyInstance(const EOS_Lobby_CreateLobbyCallbackInfo* Data);
    
    /** Callback for when a lobby is destroyed */
    static void EOS_CALL OnLobbyInstanceterminated(const EOS_Lobby_DestroyLobbyCallbackInfo* Data);
    
    /** Callback for when a lobby search is completed */
    static void EOS_CALL OnLobbySearchFinishedCallback(const EOS_LobbySearch_FindCallbackInfo* Data);
    
    /** Callback for when a member is kicked from the lobby */
    static void EOS_CALL OnKickMemberFinished(const EOS_Lobby_KickMemberCallbackInfo* Data);
    
    /** Callback for when a member is promoted to host */
    static void EOS_CALL OnPromoteMemberFinished(const EOS_Lobby_PromoteMemberCallbackInfo* Data);
    
    /** Callback for when leaving a lobby */
    static void EOS_CALL ProcessLobbyLeave(const EOS_Lobby_LeaveLobbyCallbackInfo* Data);
    
    /** Callback for when the user requests to leave via the overlay */
    static void EOS_CALL OnLeaveLobbyRequested(const EOS_Lobby_LeaveLobbyRequestedCallbackInfo* Data);

    static void EOS_CALL OnJoinLobbyByIdCallback(const EOS_Lobby_JoinLobbyByIdCallbackInfo* Data);

    static void EOS_CALL OnJoinLobbyCallback(const EOS_Lobby_JoinLobbyCallbackInfo* Data);

    static void EOS_CALL OnSendInviteCompleteCallback(const EOS_Lobby_SendInviteCallbackInfo* Data);

    static void EOS_CALL OnLobbyInviteReceivedCallback(const EOS_Lobby_LobbyInviteReceivedCallbackInfo* Data);

    static void EOS_CALL OnRejectInviteCompleteCallback(const EOS_Lobby_RejectInviteCallbackInfo* Data);
    
    //
    // Member Variables
    //
    
    /** Session authenticator for validating player credentials */
    UPROPERTY()
    UOnlineSessionAuthenticator* SessionAuthenticator;
    
    /** Reference to the Online Service Launcher subsystem */
    UPROPERTY()
    UOnlineServiceLauncher* ServiceLauncher;
    
    /** EOS Lobby interface handle */
    EOS_HLobby LobbyHandle;
    
    /** Current active lobby ID */
    FString ActiveLobbyId;
    
    /** EOS Lobby search handle */
    EOS_HLobbySearch LobbySearchHandle;
    
    /** List of lobby search results */
    TArray<FString> LobbySearchResults;
    
    /** Pending attributes to set after lobby creation */
    TMap<FString, FString> TransientStringAttributes;
    TMap<FString, int64>   TransientInt64Attributes;
    TMap<FString, bool>    TransientBoolAttributes;
    TMap<FString, float>   TransientFloatAttributes;
    
    /** Flag to determine if the lobby should be destroyed after leaving */
    bool bShouldDestroyLobbyAfterLeaving;

    // Invite cache
    TMap<FString, FString> PendingInvites; // InviteId -> SenderId

    
    /** Notification handle for leave lobby requests from overlay */
    EOS_NotificationId LeaveLobbyNotificationId = EOS_INVALID_NOTIFICATIONID;
      EOS_NotificationId InviteNotificationId = EOS_INVALID_NOTIFICATIONID;
};