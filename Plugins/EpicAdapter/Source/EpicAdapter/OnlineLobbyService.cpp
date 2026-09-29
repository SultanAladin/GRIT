// OnlineLobbyService.cpp
// Implementation of the OnlineLobbyService subsystem
#include "OnlineLobbyService.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "eos_lobby_types.h"
#include <string>

DEFINE_LOG_CATEGORY_STATIC(LogOnlineLobbyService, Log, All);


UOnlineLobbyService::UOnlineLobbyService()
{
    // Default constructor
    ActiveLobbyId = FString();
    LobbyHandle = nullptr;
}


void UOnlineLobbyService::Initialize(FSubsystemCollectionBase& Collection)
{
    // Declare dependency on ServiceLauncher and SessionAuthenticator
    //Collection.InitializeDependency<UOnlineServiceLauncher>();
    Collection.InitializeDependency<UOnlineSessionAuthenticator>();
    
    // Call the parent class initialize
    Super::Initialize(Collection);
    
    // Get references to the service subsystems
     ServiceLauncher = GEngine->GetEngineSubsystem<UOnlineServiceLauncher>();

    SessionAuthenticator = GetGameInstance()->GetSubsystem<UOnlineSessionAuthenticator>();

    // Initialize the lobby interface
    if (InitializeLobbyInterface())
    {
        UE_LOG(LogOnlineLobbyService, Log, TEXT("Lobby Interface successfully initialized during OnlineLobbyService initialization"));
    }
    else
    {
         UE_LOG(LogOnlineLobbyService, Error, TEXT("Failed to initialize Lobby Interface during OnlineLobbyService initialization"));
    }
    
    UE_LOG(LogOnlineLobbyService, Log, TEXT("OnlineLobbyService initialized"));
}

bool UOnlineLobbyService::InitializeLobbyInterface()
{
    if (!ServiceLauncher)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("ServiceLauncher is null"));
        return false;
    }
    
    EOS_HPlatform PlatformHandle = ServiceLauncher->GetPlatformHandle();
    if (!PlatformHandle)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("Platform Handle is invalid"));
        return false;
    }
    
    LobbyHandle = EOS_Platform_GetLobbyInterface(PlatformHandle);
    if (!LobbyHandle)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("Failed to get Lobby Interface"));
        return false;
    }
    
    UE_LOG(LogOnlineLobbyService, Log, TEXT("Lobby Interface initialized successfully"));
    return true;
}

// Helper function to get lobby details handle
EOS_HLobbyDetails UOnlineLobbyService::RetrieveLobbyDetailsHandle(const FString& LobbyId, EOS_ProductUserId LocalUserId)
{
    // Set up options to copy the lobby details handle
    EOS_Lobby_CopyLobbyDetailsHandleOptions CopyOptions = {};
    CopyOptions.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLE_API_LATEST;
    CopyOptions.LobbyId = TCHAR_TO_UTF8(*LobbyId);
    CopyOptions.LocalUserId = LocalUserId;
    
    EOS_HLobbyDetails LobbyDetailsHandle = nullptr;
    EOS_EResult Result = EOS_Lobby_CopyLobbyDetailsHandle(LobbyHandle, &CopyOptions, &LobbyDetailsHandle);
    
    if (Result != EOS_EResult::EOS_Success)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to copy lobby details handle: %s"),
               UTF8_TO_TCHAR(EOS_EResult_ToString(Result)));
        return nullptr;
    }
    
    return LobbyDetailsHandle;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------///

void UOnlineLobbyService::Deinitialize()
{
    // Clean up any active lobby
    if (!ActiveLobbyId.IsEmpty())
    {
        // Uncomment these lines when implementing LeaveLobby and Destroy logic
        // LeaveLobby();
        // Destroy(lobby);
        
        UE_LOG(LogOnlineLobbyService, Log, TEXT("Active lobby '%s' cleaned up during deinitialization"), *ActiveLobbyId);
    }
    
    // Clear references
    ServiceLauncher = nullptr;
    SessionAuthenticator = nullptr;
    LobbyHandle = nullptr;
    
    UE_LOG(LogOnlineLobbyService, Log, TEXT("OnlineLobbyService deinitialized"));
    
    // Call the parent class deinitialize
    Super::Deinitialize();
}

//----------------------------------------------------------------------------Lobby Creation-------------------------------------------------------------------------//






bool UOnlineLobbyService::OpenLobbyInstance(int32 MaxMembers, ELobbyPermissionLevel PermissionLevel, bool bEnablePresence, bool bAllowInvites, const FString& BucketId, bool bAllowHostMigration, bool bEnableRTCRoom, const TMap<FString, FString>& StringAttributes, const TMap<FString, int64>& IntAttributes, const TMap<FString, bool>& BoolAttributes, const TMap<FString, float>& FloatAttributes)
{
    // Ensure we have valid handles
  if (!LobbyHandle && !InitializeLobbyInterface())
{
    UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to initialize Lobby Interface"));
    return false;
}

    // Get the local user's Product User ID
    EOS_ProductUserId LocalUserId = SessionAuthenticator->GetAuthenticatedProductUserId();
    if (!LocalUserId)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Invalid Local User ID"));
        return false;
    }
    // Store the attribute maps for use in the callback
    TransientStringAttributes = StringAttributes;
    TransientInt64Attributes = IntAttributes;
    TransientBoolAttributes = BoolAttributes;
    TransientFloatAttributes = FloatAttributes;
    
    // Convert UE5 enum to EOS SDK enum
    EOS_ELobbyPermissionLevel EOSPermissionLevel;
    switch (PermissionLevel)
    {
        case ELobbyPermissionLevel::PublicAdvertised:EOSPermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;
            break;
        case ELobbyPermissionLevel::JoinViaPresence:EOSPermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_JOINVIAPRESENCE;
            break;
        case ELobbyPermissionLevel::InviteOnly:EOSPermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_INVITEONLY;
            break;
        default:
            EOSPermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;
            break;
    }
    
    // Prepare Lobby Creation Options
    EOS_Lobby_CreateLobbyOptions CreateOptions = {};
    CreateOptions.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
    CreateOptions.LocalUserId = LocalUserId;
    CreateOptions.MaxLobbyMembers = MaxMembers;
    CreateOptions.PermissionLevel = EOSPermissionLevel;
    CreateOptions.bPresenceEnabled = bEnablePresence ? EOS_TRUE : EOS_FALSE;
    CreateOptions.bAllowInvites = bAllowInvites ? EOS_TRUE : EOS_FALSE;
    CreateOptions.BucketId = TCHAR_TO_UTF8(*BucketId);
    
    // Add host migration setting (inverted as per SDK's bDisableHostMigration)
    CreateOptions.bDisableHostMigration = bAllowHostMigration ? EOS_FALSE : EOS_TRUE;
    
    // Configure RTC Options if enabled.
    // Voice design (not fully implemented yet):
    // - All match players join a single RTC room attached to this lobby (bEnableRTCRoom = true, AutomaticJoin).
    // - Game code keeps a mapping ProductUserId -> TeamId (0..N) for all participants in the match.
    // - On each local client, when teams are assigned or change, call EOS_RTCAudio_UpdateReceiving for each
    //   remote ProductUserId to enable audio for teammates and disable audio for non-teammates.
    // - This gives "hear teammates only" even with many teams (e.g. 30+ teams, 2-5 players each) and still
    //   works with host migration, because the mute/unmute logic is applied locally per client, not per host.
    EOS_Lobby_LocalRTCOptions LocalRTCOptions = {};
    if (bEnableRTCRoom)
    {
        CreateOptions.bEnableRTCRoom = EOS_TRUE;
        CreateOptions.RTCRoomJoinActionType = EOS_ELobbyRTCRoomJoinActionType::EOS_LRRJAT_AutomaticJoin;
        
        // Set up LocalRTCOptions
        LocalRTCOptions.ApiVersion = EOS_LOBBY_LOCALRTCOPTIONS_API_LATEST;
        LocalRTCOptions.Flags = EOS_RTC_JOINROOMFLAGS_ENABLE_DATACHANNEL; // Enable data channel for custom game data
        LocalRTCOptions.bUseManualAudioInput = EOS_FALSE;
        LocalRTCOptions.bUseManualAudioOutput = EOS_FALSE;
        LocalRTCOptions.bLocalAudioDeviceInputStartsMuted = EOS_FALSE;
        
        // Assign the LocalRTCOptions to CreateOptions
        CreateOptions.LocalRTCOptions = &LocalRTCOptions;
    }
    else
    {
        CreateOptions.bEnableRTCRoom = EOS_FALSE;
        CreateOptions.LocalRTCOptions = nullptr;
    }
    
    // Create the lobby
    EOS_Lobby_CreateLobby(LobbyHandle,&CreateOptions,this,&UOnlineLobbyService::ProcessOpenedLobbyInstance);
    
    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] CreateLobby call initiated with PermissionLevel: %d, MaxMembers: %d, AllowHostMigration: %d, EnableRTCRoom: %d"),static_cast<int>(PermissionLevel), MaxMembers, bAllowHostMigration, bEnableRTCRoom);
    return true;
}


bool UOnlineLobbyService::ApplyLobbyAttributes(const TMap<FString, FString>& StringAttributes,const TMap<FString, int64>& IntAttributes, const TMap<FString, bool>& BoolAttributes,const TMap<FString, float>& FloatAttributes)
{
    // Store these for later use if the lobby isn't created yet
    TransientStringAttributes = StringAttributes;
    TransientInt64Attributes = IntAttributes;
    TransientBoolAttributes = BoolAttributes;
    TransientFloatAttributes = FloatAttributes;
    
    if (ActiveLobbyId.IsEmpty())
    {
        UE_LOG(LogOnlineLobbyService, Warning, TEXT("[EOS Lobby] Cannot set attributes - no active lobby. Attributes will be applied when lobby is created."));
        return false;
    }
    
     if (!ServiceLauncher || !ServiceLauncher->GetPlatformHandle() || !LobbyHandle)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Invalid Platform or Lobby Handle"));
        return false;
    }
    
    // Get the local user's Product User ID
    EOS_ProductUserId LocalUserId =  SessionAuthenticator->GetAuthenticatedProductUserId();
    if (!LocalUserId)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Invalid Local User ID"));
        return false;
    }
    
    // Prepare Lobby Modification Options
    EOS_Lobby_UpdateLobbyModificationOptions ModifyOptions = {};
    ModifyOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
    ModifyOptions.LobbyId = TCHAR_TO_UTF8(*ActiveLobbyId);
    ModifyOptions.LocalUserId = LocalUserId;
    
    // Create Lobby Modification Handle
    EOS_HLobbyModification LobbyModification = nullptr;
    EOS_EResult Result = EOS_Lobby_UpdateLobbyModification(LobbyHandle, &ModifyOptions, &LobbyModification);
    
    if (Result != EOS_EResult::EOS_Success)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to create lobby modification: %s"),
               UTF8_TO_TCHAR(EOS_EResult_ToString(Result)));
        return false;
    }
    
    // Add string attributes
    for (const TPair<FString, FString>& Pair : StringAttributes)
    {
        // Check for empty key
        if (Pair.Key.IsEmpty())
        {
            UE_LOG(LogOnlineLobbyService, Warning, TEXT("[EOS Lobby] Empty attribute key found for string value '%s', will be skipped"), *Pair.Value);
            continue;
        }
        
        // Check for empty value
        if (Pair.Value.IsEmpty())
        {
            UE_LOG(LogOnlineLobbyService, Warning, TEXT("[EOS Lobby] Empty string value for attribute '%s' will be skipped"), *Pair.Key);
            continue;
        }
        
        EOS_Lobby_AttributeData Attribute = {};
        Attribute.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
        Attribute.Key = TCHAR_TO_UTF8(*Pair.Key);
        Attribute.ValueType = EOS_ELobbyAttributeType::EOS_AT_STRING;
        Attribute.Value.AsUtf8 = TCHAR_TO_UTF8(*Pair.Value);
        
        EOS_LobbyModification_AddAttributeOptions AddAttrOptions = {};
        AddAttrOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDATTRIBUTE_API_LATEST;
        AddAttrOptions.Attribute = &Attribute;
        AddAttrOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;
        
        Result = EOS_LobbyModification_AddAttribute(LobbyModification, &AddAttrOptions);
        if (Result != EOS_EResult::EOS_Success)
        {
            UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to add string attribute '%s': %s"),
                   *Pair.Key, UTF8_TO_TCHAR(EOS_EResult_ToString(Result)));
        }
    }
    
    // Add integer attributes
    for (const TPair<FString, int64>& Pair : IntAttributes)
    {
        // Check for empty key
        if (Pair.Key.IsEmpty())
        {
            UE_LOG(LogOnlineLobbyService, Warning, TEXT("[EOS Lobby] Empty attribute key found for integer value '%lld', will be skipped"), Pair.Value);
            continue;
        }
        
        EOS_Lobby_AttributeData Attribute = {};
        Attribute.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
        Attribute.Key = TCHAR_TO_UTF8(*Pair.Key);
        Attribute.ValueType = EOS_ELobbyAttributeType::EOS_AT_INT64;
        Attribute.Value.AsInt64 = Pair.Value;
        
        EOS_LobbyModification_AddAttributeOptions AddAttrOptions = {};
        AddAttrOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDATTRIBUTE_API_LATEST;
        AddAttrOptions.Attribute = &Attribute;
        AddAttrOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;
        
        Result = EOS_LobbyModification_AddAttribute(LobbyModification, &AddAttrOptions);
        if (Result != EOS_EResult::EOS_Success)
        {
            UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to add integer attribute '%s': %s"),
                   *Pair.Key, UTF8_TO_TCHAR(EOS_EResult_ToString(Result)));
        }
    }
    
    // Add boolean attributes
    for (const TPair<FString, bool>& Pair : BoolAttributes)
    {
        // Check for empty key
        if (Pair.Key.IsEmpty())
        {
            UE_LOG(LogOnlineLobbyService, Warning, TEXT("[EOS Lobby] Empty attribute key found for boolean value '%s', will be skipped"), 
                   Pair.Value ? TEXT("true") : TEXT("false"));
            continue;
        }
        
        EOS_Lobby_AttributeData Attribute = {};
        Attribute.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
        Attribute.Key = TCHAR_TO_UTF8(*Pair.Key);
        Attribute.ValueType = EOS_ELobbyAttributeType::EOS_AT_BOOLEAN;
        Attribute.Value.AsBool = Pair.Value ? EOS_TRUE : EOS_FALSE;
        
        EOS_LobbyModification_AddAttributeOptions AddAttrOptions = {};
        AddAttrOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDATTRIBUTE_API_LATEST;
        AddAttrOptions.Attribute = &Attribute;
        AddAttrOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;
        
        Result = EOS_LobbyModification_AddAttribute(LobbyModification, &AddAttrOptions);
        if (Result != EOS_EResult::EOS_Success)
        {
            UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to add boolean attribute '%s': %s"),
                   *Pair.Key, UTF8_TO_TCHAR(EOS_EResult_ToString(Result)));
        }
    }
    
    // Add float attributes (converted to double)
    for (const TPair<FString, float>& Pair : FloatAttributes)
    {
        // Check for empty key
        if (Pair.Key.IsEmpty())
        {
            UE_LOG(LogOnlineLobbyService, Warning, TEXT("[EOS Lobby] Empty attribute key found for float value '%f', will be skipped"), Pair.Value);
            continue;
        }
        
        EOS_Lobby_AttributeData Attribute = {};
        Attribute.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
        Attribute.Key = TCHAR_TO_UTF8(*Pair.Key);
        Attribute.ValueType = EOS_ELobbyAttributeType::EOS_AT_DOUBLE;
        Attribute.Value.AsDouble = static_cast<double>(Pair.Value);
        
        EOS_LobbyModification_AddAttributeOptions AddAttrOptions = {};
        AddAttrOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDATTRIBUTE_API_LATEST;
        AddAttrOptions.Attribute = &Attribute;
        AddAttrOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;
        
        Result = EOS_LobbyModification_AddAttribute(LobbyModification, &AddAttrOptions);
        if (Result != EOS_EResult::EOS_Success)
        {
            UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to add float attribute '%s': %s"),
                   *Pair.Key, UTF8_TO_TCHAR(EOS_EResult_ToString(Result)));
        }
    }
    
    // Trigger lobby update
    EOS_Lobby_UpdateLobbyOptions UpdateOptions = {};
    UpdateOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
    UpdateOptions.LobbyModificationHandle = LobbyModification;
    
    EOS_Lobby_UpdateLobby(LobbyHandle,&UpdateOptions,nullptr,[](const EOS_Lobby_UpdateLobbyCallbackInfo* Data)
        {
            if (Data->ResultCode == EOS_EResult::EOS_Success)
            {
                UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Lobby attributes updated successfully"));
            }
            else
            {
                UE_LOG(LogOnlineLobbyService, Warning, TEXT("[EOS Lobby] Failed to update lobby attributes :ErrorCode : %s"),
                       UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
            }
        }
    );
    
    return true;
}

void UOnlineLobbyService::ProcessOpenedLobbyInstance(const EOS_Lobby_CreateLobbyCallbackInfo* Data) {
    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Lobby Creation Callback Received"));
    
    // Convert result code to string for logging
    const char* ResultString = EOS_EResult_ToString(Data->ResultCode);
    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Result Code: %s"), UTF8_TO_TCHAR(ResultString));
    
    if (Data->ResultCode == EOS_EResult::EOS_Success) 
    {
        UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Lobby created successfully"));
        if (Data->LobbyId && Data->ClientData) 
        {
            // Convert lobby ID to FString
            FString LobbyId = UTF8_TO_TCHAR(Data->LobbyId);
            UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Lobby ID: %s"), *LobbyId);
            
            // Get the service instance from client data
            UOnlineLobbyService* Service = static_cast<UOnlineLobbyService*>(Data->ClientData);
            
            // Store the current lobby ID
            Service->ActiveLobbyId = LobbyId;
            
            // Additional success logging
            UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Setting ActiveLobbyId to: %s"), *LobbyId);
        
            
            // Set lobby attributes immediately after creation
                Service->ApplyLobbyAttributes(
                Service->TransientStringAttributes, 
                Service->TransientInt64Attributes,
                Service->TransientBoolAttributes,
                Service->TransientFloatAttributes
            );
            
            Service->SetupLobbyNotifications();
            
            FLobbyMetadata LobbyMetadata = Service->RetrieveLobbyMetadata(LobbyId, nullptr, true);
            Service->LogLobbyMetadata(LobbyMetadata);
            Service->OnLobbyCreatedDelegate.Broadcast(true, LobbyId);

        }
    } 
    else if (Data->ClientData)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Lobby creation failed"));
        
        // Get the service instance from client data
        UOnlineLobbyService* Service = static_cast<UOnlineLobbyService*>(Data->ClientData);
        
        // Clear the current lobby ID on failure
        Service->ActiveLobbyId.Empty();
        
        // Broadcast lobby creation failure event
        Service->OnLobbyCreatedDelegate.Broadcast(false, TEXT(""));
    }
}



//------------------------------------------------------------------Lobby Creation---------------------------------------------------------------//


bool UOnlineLobbyService::AuthenticateSession()
{
    // Implement session authentication if needed

    //Check 4 Cheaters 

    // Remove Banned Players 

    // This is a placeholder for now
    if (SessionAuthenticator)
    {
        return true; // For this example, assume authentication is successful
    }
    return false;
}

//-------------------------------------------------------------------------------Detroying Lobby ----------------------------------------------------------------------------//

bool UOnlineLobbyService::TerminateCurrentLobbyInstance()
{
    // Check if we have a valid lobby
     // Ensure we have valid handles
    if (!LobbyHandle)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Lobby Handle is invalid"));
        
        // Try to initialize the lobby interface
        if (!InitializeLobbyInterface())
        {
            UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to initialize Lobby Interface"));
            return false;
        }
    }
    
    // Get the local user's Product User ID
    EOS_ProductUserId LocalUserId = SessionAuthenticator->GetAuthenticatedProductUserId();
    if (!LocalUserId)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Invalid Local User ID"));
        return false;
    }
    
    // Prepare Destroy Lobby Options
    EOS_Lobby_DestroyLobbyOptions DestroyOptions = {};
    DestroyOptions.ApiVersion = EOS_LOBBY_DESTROYLOBBY_API_LATEST;
    DestroyOptions.LocalUserId = LocalUserId;
    
    // Convert stored lobby ID to const char*
    const char* LobbyIdCStr = TCHAR_TO_UTF8(*ActiveLobbyId);
    DestroyOptions.LobbyId = LobbyIdCStr;
    
    // Destroy the lobby
    EOS_Lobby_DestroyLobby(LobbyHandle, &DestroyOptions, this,  &UOnlineLobbyService::OnLobbyInstanceterminated );
    
    // Reset lobby-related members
    LobbyHandle = nullptr;
    ActiveLobbyId = FString(); // Clear the stored lobby ID

    return true;
}

void UOnlineLobbyService::OnLobbyInstanceterminated(const EOS_Lobby_DestroyLobbyCallbackInfo* Data)
{
    // First, retrieve the instance pointer from ClientData
    UOnlineLobbyService* LobbyService = static_cast<UOnlineLobbyService*>(Data->ClientData);
    if (!LobbyService)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Destroy lobby callback received invalid client data"));
        return;
    }

    if (Data->ResultCode == EOS_EResult::EOS_Success)
    {
        UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Lobby Destroyed Successfully"));
        
        // Call the instance method on the retrieved object
        LobbyService->RemoveLobbyNotifications();
    }
    else
    {
        // Convert result to string for detailed logging
        const char* ResultString = EOS_EResult_ToString(Data->ResultCode);
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Lobby Destruction Failed. Result: %s"), 
               UTF8_TO_TCHAR(ResultString));
    }
}


//---------------------------------------------------------------------------Lobby Data Retrieval ------------------------------------------------------------------------------------




// Helper function to retrieve lobby owner information
void UOnlineLobbyService::RetrieveLobbyOwnerInfo(FLobbyMetadata& LobbyData, EOS_HLobbyDetails LobbyDetailsHandle)
{
    EOS_LobbyDetails_GetLobbyOwnerOptions OwnerOptions = {};
    OwnerOptions.ApiVersion = EOS_LOBBYDETAILS_GETLOBBYOWNER_API_LATEST;
    EOS_ProductUserId LobbyOwnerProductUserId = EOS_LobbyDetails_GetLobbyOwner(LobbyDetailsHandle, &OwnerOptions);
    
    if (LobbyOwnerProductUserId)
    {
        char OwnerIdBuffer[EOS_PRODUCTUSERID_MAX_LENGTH + 1];
        int32 BufferLength = sizeof(OwnerIdBuffer);
        if (EOS_ProductUserId_ToString(LobbyOwnerProductUserId, OwnerIdBuffer, &BufferLength) == EOS_EResult::EOS_Success)
        {
            LobbyData.LobbyOwner = UTF8_TO_TCHAR(OwnerIdBuffer);
            
            // Additional owner info would require user info interface
            // This is where you would populate LobbyOwnerAccountId and LobbyOwnerDisplayName
            // if you have the corresponding functions in your user info service
        }
    }
}

// Helper function to retrieve lobby attributes
void UOnlineLobbyService::RetrieveLobbyAttributes(FLobbyMetadata& LobbyData, EOS_HLobbyDetails LobbyDetailsHandle)
{
    // Get member count for calculating available slots later
    EOS_LobbyDetails_GetMemberCountOptions MemberCountOptions = {};
    MemberCountOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERCOUNT_API_LATEST;
    uint32 MemberCount = EOS_LobbyDetails_GetMemberCount(LobbyDetailsHandle, &MemberCountOptions);
    
    // Get the number of attributes in the lobby
    EOS_LobbyDetails_GetAttributeCountOptions CountOptions = {};
    CountOptions.ApiVersion = EOS_LOBBYDETAILS_GETATTRIBUTECOUNT_API_LATEST;
    uint32 AttributeCount = EOS_LobbyDetails_GetAttributeCount(LobbyDetailsHandle, &CountOptions);
    
    // Process each attribute
    for (uint32 Index = 0; Index < AttributeCount; ++Index)
    {
        ProcessLobbyAttribute(LobbyData, LobbyDetailsHandle, Index, MemberCount);
    }
}

// Helper function to process a single lobby attribute
void UOnlineLobbyService::ProcessLobbyAttribute(FLobbyMetadata& LobbyData, EOS_HLobbyDetails LobbyDetailsHandle, uint32 Index, uint32 MemberCount)
{
    EOS_LobbyDetails_CopyAttributeByIndexOptions AttrOptions = {};
    AttrOptions.ApiVersion = EOS_LOBBYDETAILS_COPYATTRIBUTEBYINDEX_API_LATEST;
    AttrOptions.AttrIndex = Index;
    
    EOS_Lobby_Attribute* Attribute = nullptr;
    EOS_EResult Result = EOS_LobbyDetails_CopyAttributeByIndex(LobbyDetailsHandle, &AttrOptions, &Attribute);
    
    if (Result == EOS_EResult::EOS_Success && Attribute && Attribute->Data)
    {
        FString Key = UTF8_TO_TCHAR(Attribute->Data->Key);
        
        // Store attribute based on its type
        switch (Attribute->Data->ValueType)
        {
            case EOS_ELobbyAttributeType::EOS_AT_STRING:
            {
                FString Value = UTF8_TO_TCHAR(Attribute->Data->Value.AsUtf8);
                LobbyData.StringAttributes.Add(Key, Value);
                
                // Also handle specific known attributes
                if (Key == TEXT("BUCKET_ID"))
                {
                    LobbyData.BucketId = Value;
                }
                else if (Key == TEXT("RTC_ROOM_NAME") || Key == TEXT("RTCRoomName"))
                {
                    LobbyData.RTCRoomName = Value;
                }
                
                UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] String Attribute - %s: %s"), *Key, *Value);
                break;
            }
            case EOS_ELobbyAttributeType::EOS_AT_INT64:
            {
                int64 ValueInt = Attribute->Data->Value.AsInt64;
                LobbyData.Int64Attributes.Add(Key, ValueInt);
                
                // Handle specific known attributes
                if (Key == TEXT("MAX_MEMBERS") || Key == TEXT("MaxMembers"))
                {
                    LobbyData.MaxNumLobbyMembers = static_cast<int32>(ValueInt);
                    // Calculate available slots
                    LobbyData.AvailableSlots = LobbyData.MaxNumLobbyMembers - MemberCount;
                }
                else if (Key == TEXT("PERMISSION_LEVEL") || Key == TEXT("PermissionLevel"))
                {
                    LobbyData.Permission = static_cast<ELobbyPermissionLevel>(ValueInt);
                }
                
                UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Integer Attribute - %s: %lld"), *Key, ValueInt);
                break;
            }
            case EOS_ELobbyAttributeType::EOS_AT_DOUBLE:
            {
                float ValueFloat = static_cast<float>(Attribute->Data->Value.AsDouble);
                LobbyData.FloatAttributes.Add(Key, ValueFloat);
                
                UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Double/Float Attribute - %s: %f"), *Key, ValueFloat);
                break;
            }
            case EOS_ELobbyAttributeType::EOS_AT_BOOLEAN:
            {
                bool BoolVal = (Attribute->Data->Value.AsBool == EOS_TRUE);
                LobbyData.BoolAttributes.Add(Key, BoolVal);
                
                // Handle specific known attributes
                if (Key == TEXT("ALLOW_INVITES") || Key == TEXT("AllowInvites"))
                {
                    LobbyData.bAllowInvites = BoolVal;
                }
                else if (Key == TEXT("PRESENCE_ENABLED") || Key == TEXT("PresenceEnabled"))
                {
                    LobbyData.bPresenceEnabled = BoolVal;
                }
                else if (Key == TEXT("RTC_ROOM_ENABLED") || Key == TEXT("RTCRoomEnabled"))
                {
                    LobbyData.bRTCRoomEnabled = BoolVal;
                }
                else if (Key == TEXT("RTC_ROOM_CONNECTED") || Key == TEXT("RTCRoomConnected"))
                {
                    LobbyData.bRTCRoomConnected = BoolVal;
                }
                
                UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Boolean Attribute - %s: %s"), *Key, BoolVal ? TEXT("true") : TEXT("false"));
                break;
            }
            default:
            {
                UE_LOG(LogOnlineLobbyService, Warning, TEXT("[EOS Lobby] Unknown Attribute Type - %s (Type: %d)"), 
                       *Key, static_cast<int>(Attribute->Data->ValueType));
                break;
            }
        }
        
        // Release the attribute
        EOS_Lobby_Attribute_Release(Attribute);
    }
    else
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to copy attribute at index %d: %s"),
               Index, UTF8_TO_TCHAR(EOS_EResult_ToString(Result)));
    }
}

// Helper function to retrieve lobby members
void UOnlineLobbyService::RetrieveLobbyMembers(FLobbyMetadata& LobbyData, EOS_HLobbyDetails LobbyDetailsHandle)
{
    EOS_LobbyDetails_GetMemberCountOptions MemberCountOptions = {};
    MemberCountOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERCOUNT_API_LATEST;
    uint32 MemberCount = EOS_LobbyDetails_GetMemberCount(LobbyDetailsHandle, &MemberCountOptions);
    
    for (uint32 MemberIndex = 0; MemberIndex < MemberCount; ++MemberIndex)
    {
        EOS_LobbyDetails_GetMemberByIndexOptions MemberOptions = {};
        MemberOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERBYINDEX_API_LATEST;
        MemberOptions.MemberIndex = MemberIndex;
        
        EOS_ProductUserId MemberPUID = EOS_LobbyDetails_GetMemberByIndex(LobbyDetailsHandle, &MemberOptions);
        
        if (MemberPUID)
        {
            FLobbyMember NewMember;
            
            char MemberIdBuffer[EOS_PRODUCTUSERID_MAX_LENGTH + 1];
            int32 BufferLength = sizeof(MemberIdBuffer);
            
            if (EOS_ProductUserId_ToString(MemberPUID, MemberIdBuffer, &BufferLength) == EOS_EResult::EOS_Success)
            {
                NewMember.ProductUserId = UTF8_TO_TCHAR(MemberIdBuffer);
                
                // Additional member info would require your user info service
                // This is where you would populate the member's account ID and display name
                
                LobbyData.Members.Add(NewMember);
            }
        }
    }
}

FLobbyMetadata UOnlineLobbyService::RetrieveLobbyMetadata(const FString& LobbyId, EOS_HLobbyDetails ExistingLobbyDetailsHandle, bool bReleaseHandle = true)
{
    FLobbyMetadata LobbyMetadata;
    
    if (LobbyId.IsEmpty())
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Cannot retrieve metadata - invalid Lobby ID"));
        return LobbyMetadata;
    }
    
    // Set the Lobby ID in the metadata
    LobbyMetadata.Id = LobbyId;
    
    // Use provided handle or create a new one if not provided
    EOS_HLobbyDetails LobbyDetailsHandle = ExistingLobbyDetailsHandle;
    
    // If no handle was provided, we need to create one
    if (!LobbyDetailsHandle)
    {
        if (!LobbyHandle)
        {
            UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Cannot retrieve metadata - invalid Lobby Handle"));
            return LobbyMetadata;
        }
        
        EOS_ProductUserId LocalUserId = SessionAuthenticator->GetAuthenticatedProductUserId();
        if (!LocalUserId)
        {
            UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Invalid Local User ID"));
            return LobbyMetadata;
        }
        
        // Set up options to copy the lobby details handle
        EOS_Lobby_CopyLobbyDetailsHandleOptions CopyOptions = {};
        CopyOptions.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLE_API_LATEST;
        CopyOptions.LobbyId = TCHAR_TO_UTF8(*LobbyId);
        CopyOptions.LocalUserId = LocalUserId;
        
        EOS_EResult Result = EOS_Lobby_CopyLobbyDetailsHandle(LobbyHandle, &CopyOptions, &LobbyDetailsHandle);
        
        if (Result != EOS_EResult::EOS_Success)
        {
            UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to copy lobby details handle: %s"),
                   UTF8_TO_TCHAR(EOS_EResult_ToString(Result)));
            return LobbyMetadata;
        }
        
        // Force release handle since we created it internally
        bReleaseHandle = true;
    }
    
    // Get the number of attributes in the lobby
    EOS_LobbyDetails_GetAttributeCountOptions CountOptions = {};
    CountOptions.ApiVersion = EOS_LOBBYDETAILS_GETATTRIBUTECOUNT_API_LATEST;
    uint32 AttributeCount = EOS_LobbyDetails_GetAttributeCount(LobbyDetailsHandle, &CountOptions);
    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Total Number of Lobby Attributes: %d"), AttributeCount);
    
    // Loop through each attribute
    for (uint32 Index = 0; Index < AttributeCount; ++Index)
    {
        EOS_LobbyDetails_CopyAttributeByIndexOptions AttrOptions = {};
        AttrOptions.ApiVersion = EOS_LOBBYDETAILS_COPYATTRIBUTEBYINDEX_API_LATEST;
        AttrOptions.AttrIndex = Index;
        
        EOS_Lobby_Attribute* Attribute = nullptr;
        EOS_EResult Result = EOS_LobbyDetails_CopyAttributeByIndex(LobbyDetailsHandle, &AttrOptions, &Attribute);
        
        if (Result == EOS_EResult::EOS_Success && Attribute && Attribute->Data)
        {
            // Access the key and value via the Data pointer
            FString Key = UTF8_TO_TCHAR(Attribute->Data->Key);
            
            // Store attributes based on their type
            switch (Attribute->Data->ValueType)
            {
                case EOS_ELobbyAttributeType::EOS_AT_STRING:
                {
                    FString Value = UTF8_TO_TCHAR(Attribute->Data->Value.AsUtf8);
                    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] String Attribute - %s: %s"), *Key, *Value);
                    LobbyMetadata.StringAttributes.Add(Key, Value);
                    break;
                }
                case EOS_ELobbyAttributeType::EOS_AT_INT64:
                {
                    int64 ValueInt = Attribute->Data->Value.AsInt64;
                    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Integer Attribute - %s: %lld"), *Key, ValueInt);
                    LobbyMetadata.Int64Attributes.Add(Key, ValueInt);
                    break;
                }
                case EOS_ELobbyAttributeType::EOS_AT_DOUBLE:
                {
                    double ValueDouble = Attribute->Data->Value.AsDouble;
                    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Double/Float Attribute - %s: %f"), *Key, ValueDouble);
                    LobbyMetadata.FloatAttributes.Add(Key, (float)ValueDouble);
                    break;
                }
                case EOS_ELobbyAttributeType::EOS_AT_BOOLEAN:
                {
                    bool BoolVal = (Attribute->Data->Value.AsBool == EOS_TRUE);
                    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Boolean Attribute - %s: %s"), *Key, BoolVal ? TEXT("true") : TEXT("false"));
                    LobbyMetadata.BoolAttributes.Add(Key, BoolVal);
                    break;
                }
                default:
                {
                    UE_LOG(LogOnlineLobbyService, Warning, TEXT("[EOS Lobby] Unknown Attribute Type - %s (Type: %d)"), 
                           *Key, static_cast<int>(Attribute->Data->ValueType));
                    break;
                }
            }
            
            // Release the attribute once done
            EOS_Lobby_Attribute_Release(Attribute);
        }
        else
        {
            UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to copy attribute at index %d: %s"),
                   Index, UTF8_TO_TCHAR(EOS_EResult_ToString(Result)));
        }
    }
    
    // Now, get the lobby info to populate other metadata fields
    EOS_LobbyDetails_CopyInfoOptions InfoOptions = {};
    InfoOptions.ApiVersion = EOS_LOBBYDETAILS_COPYINFO_API_LATEST;
    
    EOS_LobbyDetails_Info* LobbyInfo = nullptr;
    EOS_EResult InfoResult = EOS_LobbyDetails_CopyInfo(LobbyDetailsHandle, &InfoOptions, &LobbyInfo);
    
    if (InfoResult == EOS_EResult::EOS_Success && LobbyInfo)
    {
        // Update lobby metadata from LobbyInfo
        // Convert LobbyOwnerUserId to string
        if (LobbyInfo->LobbyOwnerUserId)
        {
            char OwnerBuffer[EOS_PRODUCTUSERID_MAX_LENGTH + 1];
            int32_t OwnerBufferSize = sizeof(OwnerBuffer);
            if (EOS_ProductUserId_ToString(LobbyInfo->LobbyOwnerUserId, OwnerBuffer, &OwnerBufferSize) == EOS_EResult::EOS_Success)
            {
                LobbyMetadata.LobbyOwner = UTF8_TO_TCHAR(OwnerBuffer);
            }
        }
        
        // Set bucket ID
        if (LobbyInfo->BucketId)
        {
            LobbyMetadata.BucketId = UTF8_TO_TCHAR(LobbyInfo->BucketId);
        }
        
        // Convert EOS permission level to our enum
        switch (LobbyInfo->PermissionLevel)
        {
            case EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED:
                LobbyMetadata.Permission = ELobbyPermissionLevel::PublicAdvertised;
                break;
            case EOS_ELobbyPermissionLevel::EOS_LPL_JOINVIAPRESENCE:
                LobbyMetadata.Permission = ELobbyPermissionLevel::JoinViaPresence;
                break;
            case EOS_ELobbyPermissionLevel::EOS_LPL_INVITEONLY:
                LobbyMetadata.Permission = ELobbyPermissionLevel::InviteOnly;
                break;
            default:
                LobbyMetadata.Permission = ELobbyPermissionLevel::PublicAdvertised;
                break;
        }
        
        // Set member counts
        LobbyMetadata.MaxNumLobbyMembers = LobbyInfo->MaxMembers;
        LobbyMetadata.AvailableSlots = LobbyInfo->AvailableSlots;
        
        // Set flags
        LobbyMetadata.bAllowInvites = (LobbyInfo->bAllowInvites == EOS_TRUE);
        LobbyMetadata.bPresenceEnabled = (LobbyInfo->bPresenceEnabled == EOS_TRUE);
        LobbyMetadata.bRTCRoomEnabled = (LobbyInfo->bRTCRoomEnabled == EOS_TRUE);
        
        // Release the lobby info
        EOS_LobbyDetails_Info_Release(LobbyInfo);
    }
    else
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to copy lobby info: %s"),
               UTF8_TO_TCHAR(EOS_EResult_ToString(InfoResult)));
    }
    
    // Clean up the lobby details handle if requested or if we created it internally
    if (bReleaseHandle && LobbyDetailsHandle)
    {
        EOS_LobbyDetails_Release(LobbyDetailsHandle);
    }
    
    return LobbyMetadata;
}

void UOnlineLobbyService::LogLobbyMetadata(const FLobbyMetadata& LobbyData)
{
    // Log lobby ID once at the beginning
    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby ID: %s] ---- Lobby Information ----"), *LobbyData.Id);
    
    // Basic lobby information without repeating the ID
    UE_LOG(LogOnlineLobbyService, Log, TEXT("  Basic Info: Owner=%s, OwnerID=%s, OwnerAccountID=%s"), 
        *LobbyData.LobbyOwnerDisplayName,
        *LobbyData.LobbyOwner,
        *LobbyData.LobbyOwnerAccountId);
    
    UE_LOG(LogOnlineLobbyService, Log, TEXT("  Bucket ID: %s"), *LobbyData.BucketId);
    
    UE_LOG(LogOnlineLobbyService, Log, TEXT("  Members: %d/%d (Available: %d)"), 
        LobbyData.Members.Num(), 
        LobbyData.MaxNumLobbyMembers, 
        LobbyData.AvailableSlots);
    
    UE_LOG(LogOnlineLobbyService, Log, TEXT("  Permission: %d"), (int32)LobbyData.Permission);
    
    UE_LOG(LogOnlineLobbyService, Log, TEXT("  Flags: AllowInvites=%s, PresenceEnabled=%s, RTCEnabled=%s, RTCConnected=%s, SearchResult=%s, BeingCreated=%s"),
        LobbyData.bAllowInvites ? TEXT("True") : TEXT("False"),
        LobbyData.bPresenceEnabled ? TEXT("True") : TEXT("False"),
        LobbyData.bRTCRoomEnabled ? TEXT("True") : TEXT("False"),
        LobbyData.bRTCRoomConnected ? TEXT("True") : TEXT("False"),
        LobbyData.bSearchResult ? TEXT("True") : TEXT("False"),
        LobbyData.bBeingCreated ? TEXT("True") : TEXT("False"));
    
    if (LobbyData.bRTCRoomEnabled)
    {
        UE_LOG(LogOnlineLobbyService, Log, TEXT("  RTC Room: %s"), *LobbyData.RTCRoomName);
    }
    
    // Log all members
    if (LobbyData.Members.Num() > 0)
    {
        UE_LOG(LogOnlineLobbyService, Log, TEXT("  ---- Members (%d) ----"), LobbyData.Members.Num());
        for (int32 i = 0; i < LobbyData.Members.Num(); ++i)
        {
            const FLobbyMember& Member = LobbyData.Members[i];
            UE_LOG(LogOnlineLobbyService, Log, TEXT("    Member[%d]: Name=%s, PUID=%s, AccountID=%s"),  i, *Member.DisplayName,  *Member.ProductUserId,   *Member.AccountId);
        }
    }
    
    // Log all attributes
    bool bHasAttributes = LobbyData.StringAttributes.Num() > 0 ||  LobbyData.Int64Attributes.Num() > 0 ||  LobbyData.BoolAttributes.Num() > 0 || LobbyData.FloatAttributes.Num() > 0;
    
    if (bHasAttributes)
    {
        UE_LOG(LogOnlineLobbyService, Log, TEXT("  ---- Attributes ----"));
        
        // String attributes
        if (LobbyData.StringAttributes.Num() > 0)
        {
            for (const TPair<FString, FString>& Attr : LobbyData.StringAttributes)
            {
                UE_LOG(LogOnlineLobbyService, Log, TEXT("    String[%s]: %s"), *Attr.Key, *Attr.Value);
            }
        }
        
        // Int64 attributes
        if (LobbyData.Int64Attributes.Num() > 0)
        {
            for (const TPair<FString, int64>& Attr : LobbyData.Int64Attributes)
            {
                UE_LOG(LogOnlineLobbyService, Log, TEXT("    Int64[%s]: %lld"), *Attr.Key, Attr.Value);
            }
        }
        
        // Bool attributes
        if (LobbyData.BoolAttributes.Num() > 0)
        {
            for (const TPair<FString, bool>& Attr : LobbyData.BoolAttributes)
            {
                UE_LOG(LogOnlineLobbyService, Log, TEXT("    Bool[%s]: %s"), 
                    *Attr.Key, 
                    Attr.Value ? TEXT("True") : TEXT("False"));
            }
        }
        
        // Float attributes
        if (LobbyData.FloatAttributes.Num() > 0)
        {
            for (const TPair<FString, float>& Attr : LobbyData.FloatAttributes)
            {
                UE_LOG(LogOnlineLobbyService, Log, TEXT("    Float[%s]: %f"), *Attr.Key, Attr.Value);
            }
        }
    }
    else
    {
        UE_LOG(LogOnlineLobbyService, Log, TEXT("  No attributes"));
    }
    
    // End marker for better log readability
    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby ID: %s] ---- End Lobby Information ----"), *LobbyData.Id);
}
//---------------------------------------------------------------------------Search Lobbies------------------------------------------------------------------------------------

void UOnlineLobbyService::SearchAllLobbies(
    const FString& BucketId,
    const TMap<FString, FString>& StringAttributes,
    const TMap<FString, int64>& IntAttributes,
    const TMap<FString, bool>& BoolAttributes,
    const TMap<FString, float>& FloatAttributes,
    int32 MaxResults
)
{
    // Ensure we have valid handles
     if (!LobbyHandle)
    {
        // Try to initialize the lobby interface
        if (!InitializeLobbyInterface())
        {
            UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to initialize Lobby Interface"));
            return;
        }
    }

    // Release the previous search handle if it exists
    if (LobbySearchHandle)
    {
        EOS_LobbySearch_Release(LobbySearchHandle);
        LobbySearchHandle = nullptr;
    }
    // Get the local user's Product User ID
    EOS_ProductUserId LocalUserId = SessionAuthenticator->GetAuthenticatedProductUserId();
    if (!LocalUserId)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Invalid Local User ID"));
        return;
    }

    // Prepare lobby search creation options
    EOS_Lobby_CreateLobbySearchOptions SearchCreationOptions = {};
    SearchCreationOptions.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
    SearchCreationOptions.MaxResults = MaxResults > 0 ? MaxResults : 50;

    // Create lobby search handle
    EOS_EResult SearchCreationResult = EOS_Lobby_CreateLobbySearch(
        LobbyHandle,
        &SearchCreationOptions,
        &LobbySearchHandle
    );
    if (SearchCreationResult != EOS_EResult::EOS_Success)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to create lobby search handle"));
        return;
    }

    // Helper function to set search parameters
    auto SetSearchParameter = [this](const char* Key, EOS_EAttributeType ValueType, const void* Value) -> bool {
        EOS_Lobby_AttributeData AttributeData = {};
        AttributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
        AttributeData.Key = Key;
        AttributeData.ValueType = ValueType;

        // Set value based on type
        switch (ValueType)
        {
            case EOS_EAttributeType::EOS_AT_STRING:
                AttributeData.Value.AsUtf8 = static_cast<const char*>(Value);
                break;
            case EOS_EAttributeType::EOS_AT_INT64:
                AttributeData.Value.AsInt64 = *static_cast<const int64*>(Value);
                break;
            case EOS_EAttributeType::EOS_AT_DOUBLE:
                AttributeData.Value.AsDouble = *static_cast<const double*>(Value);
                break;
            default:
                UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Unsupported attribute type"));
                return false;
        }

        // Prepare search parameters
        EOS_LobbySearch_SetParameterOptions SearchParams = {};
        SearchParams.ApiVersion = EOS_LOBBYSEARCH_SETPARAMETER_API_LATEST;
        SearchParams.Parameter = &AttributeData;
        SearchParams.ComparisonOp = EOS_EComparisonOp::EOS_CO_EQUAL;

        // Set the search parameter
        EOS_EResult SetParameterResult = EOS_LobbySearch_SetParameter(
            LobbySearchHandle,
            &SearchParams
        );

        if (SetParameterResult != EOS_EResult::EOS_Success)
        {
            UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to set search parameter: %d"), SetParameterResult);
            return false;
        }

        return true;
    };

    // Set Bucket ID as a mandatory search parameter if provided
    if (!BucketId.IsEmpty())
    {
        std::string BucketIdStr = TCHAR_TO_UTF8(*BucketId);
        if (!SetSearchParameter(EOS_LOBBY_SEARCH_BUCKET_ID, EOS_EAttributeType::EOS_AT_STRING, BucketIdStr.c_str()))
        {
            return;
        }
    }

    // Set string attributes (skip empty keys or values)
    for (const auto& Pair : StringAttributes)
    {
        if (!Pair.Key.IsEmpty() && !Pair.Value.IsEmpty())
        {
            std::string ValueStr = TCHAR_TO_UTF8(*Pair.Value);
            SetSearchParameter(TCHAR_TO_UTF8(*Pair.Key), EOS_EAttributeType::EOS_AT_STRING, ValueStr.c_str());
        }
    }

    // Set integer attributes (skip empty keys)
    for (const auto& Pair : IntAttributes)
    {
        if (!Pair.Key.IsEmpty())
        {
            SetSearchParameter(TCHAR_TO_UTF8(*Pair.Key), EOS_EAttributeType::EOS_AT_INT64, &Pair.Value);
        }
    }

    // Set boolean attributes (skip empty keys)
    for (const auto& Pair : BoolAttributes)
    {
        if (!Pair.Key.IsEmpty())
        {
            // Convert bool to int64 since EOS SDK doesn't have a direct bool type
            int64 BoolValue = Pair.Value ? 1 : 0;
            SetSearchParameter(TCHAR_TO_UTF8(*Pair.Key), EOS_EAttributeType::EOS_AT_INT64, &BoolValue);
        }
    }

    // Set float attributes (skip empty keys)
    for (const auto& Pair : FloatAttributes)
    {
        if (!Pair.Key.IsEmpty())
        {
            double DoubleValue = static_cast<double>(Pair.Value);
            SetSearchParameter(TCHAR_TO_UTF8(*Pair.Key), EOS_EAttributeType::EOS_AT_DOUBLE, &DoubleValue);
        }
    }

    // Prepare search options
    EOS_LobbySearch_FindOptions FindOptions = {};
    FindOptions.ApiVersion = EOS_LOBBYSEARCH_FIND_API_LATEST;
    FindOptions.LocalUserId = LocalUserId;

    // Clear previous search results
    LobbySearchResults.Empty();

    // Execute the search
    EOS_LobbySearch_Find(
        LobbySearchHandle,
        &FindOptions,
        this,  // Pass the instance as client data
        &UOnlineLobbyService::OnLobbySearchFinishedCallback
    );

    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Lobby search initiated with Bucket ID: %s"), *BucketId);
}

void EOS_CALL UOnlineLobbyService::OnLobbySearchFinishedCallback(const EOS_LobbySearch_FindCallbackInfo* Data)
{
    // Retrieve the UOnlineLobbyService instance from ClientData
    UOnlineLobbyService* Service = static_cast<UOnlineLobbyService*>(Data->ClientData);
    if (!Service)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Invalid service instance in search callback"));
        return;
    }

    // Verify the search was successful
    if (Data->ResultCode != EOS_EResult::EOS_Success)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Lobby search failed: %d"), Data->ResultCode);
        
        // Broadcast empty results on failure
        Service->OnLobbySearchCompleteDelegate.Broadcast(false, TArray<FString>());
        return;
    }

    // Process the search results
    Service->ProcessLobbySearchResults();
}

void UOnlineLobbyService::ProcessLobbySearchResults()
{
    // Check if search handle is valid
    if (!LobbySearchHandle)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Invalid lobby search handle"));
        OnLobbySearchCompleteDelegate.Broadcast(false, TArray<FString>());
        return;
    }

    // Get the number of search results
    EOS_LobbySearch_GetSearchResultCountOptions CountOptions = {};
    CountOptions.ApiVersion = EOS_LOBBYSEARCH_GETSEARCHRESULTCOUNT_API_LATEST;
    
    uint32_t ResultCount = EOS_LobbySearch_GetSearchResultCount(
        LobbySearchHandle, 
        &CountOptions
    );
    
    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Found %d lobbies"), ResultCount);

    // Clear any previous results
    LobbySearchResults.Empty();
    LobbySearchResults.Reserve(ResultCount);

    // Process each search result
    for (uint32_t i = 0; i < ResultCount; ++i)
    {
        // Prepare options to get the lobby ID
        EOS_LobbySearch_CopySearchResultByIndexOptions CopyOptions = {};
        CopyOptions.ApiVersion = EOS_LOBBYSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST;
        CopyOptions.LobbyIndex = i;

        // Get the lobby details handle
        EOS_HLobbyDetails LobbyDetailsHandle = nullptr;
        EOS_EResult CopyResult = EOS_LobbySearch_CopySearchResultByIndex(
            LobbySearchHandle,
            &CopyOptions,
            &LobbyDetailsHandle
        );

        if (CopyResult != EOS_EResult::EOS_Success)
        {
            UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to copy lobby details for index %d"), i);
            continue;
        }

        // Get lobby info from the details handle
        EOS_LobbyDetails_CopyInfoOptions InfoOptions = {};
        InfoOptions.ApiVersion = EOS_LOBBYDETAILS_COPYINFO_API_LATEST;
        
        EOS_LobbyDetails_Info* LobbyInfo = nullptr;
        EOS_EResult InfoResult = EOS_LobbyDetails_CopyInfo(
            LobbyDetailsHandle,
            &InfoOptions,
            &LobbyInfo
        );

        if (InfoResult == EOS_EResult::EOS_Success && LobbyInfo)
        {
            // Add the lobby ID to the results array
            FString LobbyId = UTF8_TO_TCHAR(LobbyInfo->LobbyId);
            LobbySearchResults.Add(LobbyId);
            
            // Additional logging
            UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Found lobby: %s, Max Members: %d"), 
                *LobbyId, LobbyInfo->MaxMembers);
            
            // Release the lobby info structure
            EOS_LobbyDetails_Info_Release(LobbyInfo);
        }
        else
        {
            UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to get lobby info for index %d"), i);
        }

        // Release the lobby details handle
        EOS_LobbyDetails_Release(LobbyDetailsHandle);
    }

    // Broadcast the search results
    OnLobbySearchCompleteDelegate.Broadcast(true, LobbySearchResults);

    // Release the search handle
  //  if (LobbySearchHandle)
    //{
     //   EOS_LobbySearch_Release(LobbySearchHandle);
      //  LobbySearchHandle = nullptr;
   // }

}   

void UOnlineLobbyService::KickMember(EOS_ProductUserId MemberId)
{
    if (!MemberId)
    {
        return;
    }
    
    EOS_ProductUserId LocalUserId = SessionAuthenticator->GetAuthenticatedProductUserId();
    if (!LocalUserId)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("Lobbies - KickMember: Current player is invalid!"));
        return;
    }
    
    if (!LobbyHandle)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("Lobbies - KickMember: Lobby handle is invalid!"));
        return;
    }
    
    if (ActiveLobbyId.IsEmpty())
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("Lobbies - KickMember: No active lobby!"));
        return;
    }
    
    EOS_Lobby_KickMemberOptions KickMemberOptions = {};
    KickMemberOptions.ApiVersion = EOS_LOBBY_KICKMEMBER_API_LATEST;
    KickMemberOptions.TargetUserId = MemberId;
    KickMemberOptions.LobbyId = TCHAR_TO_UTF8(*ActiveLobbyId);
    KickMemberOptions.LocalUserId = LocalUserId;
    
    EOS_Lobby_KickMember(LobbyHandle, &KickMemberOptions, nullptr, OnKickMemberFinished);
    
    UE_LOG(LogOnlineLobbyService, Log, TEXT("Lobby kick member request sent for user ID"));
}

void UOnlineLobbyService::PromoteMember(EOS_ProductUserId MemberId)
{
    if (!MemberId)
    {
        return;
    }
    
    EOS_ProductUserId LocalUserId = SessionAuthenticator->GetAuthenticatedProductUserId();
    if (!LocalUserId)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("Lobbies - PromoteMember: Current player is invalid!"));
        return;
    }
    
    if (!LobbyHandle)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("Lobbies - PromoteMember: Lobby handle is invalid!"));
        return;
    }
    
    if (ActiveLobbyId.IsEmpty())
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("Lobbies - PromoteMember: No active lobby!"));
        return;
    }
    
    EOS_Lobby_PromoteMemberOptions PromoteMemberOptions = {};
    PromoteMemberOptions.ApiVersion = EOS_LOBBY_PROMOTEMEMBER_API_LATEST;
    PromoteMemberOptions.TargetUserId = MemberId;
    PromoteMemberOptions.LocalUserId = LocalUserId;
    PromoteMemberOptions.LobbyId = TCHAR_TO_UTF8(*ActiveLobbyId);
    
    EOS_Lobby_PromoteMember(LobbyHandle, &PromoteMemberOptions, nullptr, OnPromoteMemberFinished);
    
    UE_LOG(LogOnlineLobbyService, Log, TEXT("Lobby promote member request sent for user ID"));
}

// Implementation of callback functions

void  UOnlineLobbyService::OnKickMemberFinished(const EOS_Lobby_KickMemberCallbackInfo* Data)
{
    if (Data)
    {
        if (!EOS_EResult_IsOperationComplete(Data->ResultCode))
        {
            UE_LOG(LogOnlineLobbyService, Log, TEXT("Lobbies (OnKickMemberFinished): operation not complete: %s"), 
                UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
        }
        else if (Data->ResultCode != EOS_EResult::EOS_Success)
        {
            UE_LOG(LogOnlineLobbyService, Error, TEXT("Lobbies (OnKickMemberFinished): error code: %s"), 
                UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
        }
        else
        {
            UE_LOG(LogOnlineLobbyService, Log, TEXT("Lobbies (OnKickMemberFinished): member kicked."));
        }
    }
    else
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("Lobbies (OnKickMemberFinished): EOS_Lobby_KickMemberCallbackInfo is null"));
    }
}

void  UOnlineLobbyService::OnPromoteMemberFinished(const EOS_Lobby_PromoteMemberCallbackInfo* Data)
{
    if (Data)
    {
        if (!EOS_EResult_IsOperationComplete(Data->ResultCode))
        {
            UE_LOG(LogOnlineLobbyService, Log, TEXT("Lobbies (OnPromoteMemberFinished): operation not complete: %s"), 
                UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
        }
        else if (Data->ResultCode != EOS_EResult::EOS_Success)
        {
            UE_LOG(LogOnlineLobbyService, Error, TEXT("Lobbies (OnPromoteMemberFinished): error code: %s"), 
                UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
        }
        else
        {
            UE_LOG(LogOnlineLobbyService, Log, TEXT("Lobbies (OnPromoteMemberFinished): member promoted."));
        }
    }
    else
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("Lobbies (OnPromoteMemberFinished): EOS_Lobby_PromoteMemberCallbackInfo is null"));
    }
}

void UOnlineLobbyService::LeaveLobby()
{
    // Ensure we have valid handles and an active lobby
    if (!LobbyHandle || ActiveLobbyId.IsEmpty())
    {
        UE_LOG(LogOnlineLobbyService, Warning, TEXT("[EOS Lobby] Cannot leave lobby: No active lobby or invalid handle"));
        return;
    }
    
    // Get the local user's Product User ID
    EOS_ProductUserId LocalUserId = SessionAuthenticator->GetAuthenticatedProductUserId();
    if (!LocalUserId)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Cannot leave lobby: Invalid Local User ID"));
        return;
    }
    
    // Check if we're the lobby owner
    bool bIsLobbyOwner = false;
    bool bAllowHostMigration = false;
    uint32_t MemberCount = 0;
    
    // Get lobby details to check if host migration is enabled and if we're the owner
    EOS_HLobbyDetails LobbyDetails = nullptr;
    // You'd need to get the lobby details handle here
    // For example: LobbyDetails = GetCurrentLobbyDetails();
    
    if (LobbyDetails)
    {
        EOS_LobbyDetails_GetLobbyOwnerOptions OwnerOptions = {};
        OwnerOptions.ApiVersion = EOS_LOBBYDETAILS_GETLOBBYOWNER_API_LATEST;
        
        EOS_ProductUserId LobbyOwner = EOS_LobbyDetails_GetLobbyOwner(LobbyDetails, &OwnerOptions);
        bIsLobbyOwner = (LobbyOwner == LocalUserId);
        
        if (bIsLobbyOwner)
        {
            EOS_LobbyDetails_CopyInfoOptions InfoOptions = {};
            InfoOptions.ApiVersion = EOS_LOBBYDETAILS_COPYINFO_API_LATEST;
            
            EOS_LobbyDetails_Info* LobbyInfo = nullptr;
            EOS_EResult Result = EOS_LobbyDetails_CopyInfo(LobbyDetails, &InfoOptions, &LobbyInfo);
            
            if (Result == EOS_EResult::EOS_Success && LobbyInfo)
            {
                bAllowHostMigration = LobbyInfo->bAllowHostMigration == EOS_TRUE;
                
                EOS_LobbyDetails_GetMemberCountOptions MemberCountOptions = {};
                MemberCountOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERCOUNT_API_LATEST;
                
                MemberCount = EOS_LobbyDetails_GetMemberCount(LobbyDetails, &MemberCountOptions);
                
                EOS_LobbyDetails_Info_Release(LobbyInfo);
            }
        }
        
        // Release the lobby details if we got it
        // EOS_LobbyDetails_Release(LobbyDetails);
    }
    
    // Store whether we should destroy the lobby in a member variable
    bShouldDestroyLobbyAfterLeaving = bIsLobbyOwner && (MemberCount <= 1 || !bAllowHostMigration);
    ActiveLobbyId = ActiveLobbyId; // Store this since ActiveLobbyId will be cleared in the callback
    
    // Set up the options for leaving the lobby
    EOS_Lobby_LeaveLobbyOptions LeaveOptions = {};
    LeaveOptions.ApiVersion = EOS_LOBBY_LEAVELOBBY_API_LATEST;
    LeaveOptions.LocalUserId = LocalUserId;
    LeaveOptions.LobbyId = TCHAR_TO_UTF8(*ActiveLobbyId);
    
    // Call the EOS SDK function to leave the lobby
    EOS_Lobby_LeaveLobby(
        LobbyHandle,
        &LeaveOptions,
        this,
        &UOnlineLobbyService::ProcessLobbyLeave
    );
    
    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Leaving lobby %s..."), *ActiveLobbyId);
}

// Update the ProcessLobbyLeave callback to handle the destroy logic
void UOnlineLobbyService::ProcessLobbyLeave(const EOS_Lobby_LeaveLobbyCallbackInfo* Data)
{
    UOnlineLobbyService* LobbyService = static_cast<UOnlineLobbyService*>(Data->ClientData);
    if (!LobbyService)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Leave lobby callback received invalid client data"));
        return;
    }
    
    if (Data->ResultCode == EOS_EResult::EOS_Success)
    {
        FString LobbyId = UTF8_TO_TCHAR(Data->LobbyId);
        UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Successfully left lobby %s"), *LobbyId);
        
        // Remove lobby notifications
        LobbyService->RemoveLobbyNotifications();
        
        // Check if we should destroy the lobby before clearing the ActiveLobbyId
        bool bShouldDestroy = LobbyService->bShouldDestroyLobbyAfterLeaving;
        FString LobbyToDestroy = LobbyService->ActiveLobbyId;
        
        // Clear the active lobby ID since we've left
        LobbyService->ActiveLobbyId.Empty();
        
        // Check if we should destroy the lobby
        if (bShouldDestroy)
        {
            UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Owner leaving and conditions met. Terminating lobby %s"), *LobbyToDestroy);
            LobbyService->TerminateCurrentLobbyInstance();
        }
        
        // Reset the flag
        LobbyService->bShouldDestroyLobbyAfterLeaving = false;
        LobbyService->ActiveLobbyId = FString();
    }
    else
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to leave lobby. Error code: %d"), static_cast<int32>(Data->ResultCode));
    }
}

// In your initialization code (after getting a valid LobbyHandle)
void UOnlineLobbyService::SetupLobbyNotifications()
{
    if (!LobbyHandle)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Cannot set up notifications: Invalid lobby handle"));
        return;
    }

    // Register for leave lobby notifications from overlay
    EOS_Lobby_AddNotifyLeaveLobbyRequestedOptions Options = {};
    Options.ApiVersion = EOS_LOBBY_ADDNOTIFYLEAVELOBBYREQUESTED_API_LATEST;

    LeaveLobbyNotificationId = EOS_Lobby_AddNotifyLeaveLobbyRequested(
        LobbyHandle,
        &Options,
        this,
        &UOnlineLobbyService::OnLeaveLobbyRequested
    );

    if (LeaveLobbyNotificationId == EOS_INVALID_NOTIFICATIONID)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to register for leave lobby notifications"));
    }
    else
    {
        UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Successfully registered for leave lobby notifications"));
    }


    // Register for invite notifications
    EOS_Lobby_AddNotifyLobbyInviteReceivedOptions InviteOptions = {};
    InviteOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYINVITERECEIVED_API_LATEST;
    
    InviteNotificationId = EOS_Lobby_AddNotifyLobbyInviteReceived(
        LobbyHandle,
        &InviteOptions,
        this,
        &UOnlineLobbyService::OnLobbyInviteReceivedCallback
    );

    if (InviteNotificationId == EOS_INVALID_NOTIFICATIONID)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to register for invite notifications"));
      
    }

    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Successfully registered for invite notifications"));

}

// Static callback for when user requests to leave via the overlay
void UOnlineLobbyService::OnLeaveLobbyRequested(const EOS_Lobby_LeaveLobbyRequestedCallbackInfo* Data)
{
    UOnlineLobbyService* LobbyService = static_cast<UOnlineLobbyService*>(Data->ClientData);
    if (!LobbyService)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Leave lobby requested callback received invalid client data"));
        return;
    }

    FString LobbyId = UTF8_TO_TCHAR(Data->LobbyId);
    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Leave lobby requested from overlay for lobby %s"), *LobbyId);

    // Call your existing leave lobby function
    LobbyService->LeaveLobby();
}

// In your cleanup code (usually when leaving a lobby or during shutdown)
void UOnlineLobbyService::RemoveLobbyNotifications()
{
    if (LobbyHandle && LeaveLobbyNotificationId != EOS_INVALID_NOTIFICATIONID)
    {
        EOS_Lobby_RemoveNotifyLeaveLobbyRequested(LobbyHandle, LeaveLobbyNotificationId);
        LeaveLobbyNotificationId = EOS_INVALID_NOTIFICATIONID;
        UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Unregistered leave lobby notification"));
    }
}

// Function to join a lobby by ID
void UOnlineLobbyService::JoinLobbyById(const FString& LobbyId)
{
    if (!LobbyHandle)
    {
        // Try to initialize the lobby interface
        if (!InitializeLobbyInterface())
        {
            UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to initialize Lobby Interface"));
           // OnLobbyJoinedDelegate.Broadcast(false, TEXT("Lobby interface is not available"));
            return;
        }
    }

    // Get the local user's Product User ID
    EOS_ProductUserId LocalUserId = SessionAuthenticator->GetAuthenticatedProductUserId();
    if (!LocalUserId)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Invalid Local User ID"));
       // OnLobbyJoinedDelegate.Broadcast(false, TEXT("Local user ID is invalid"));
        return;
    }

    // Convert Lobby ID to ANSI string
    std::string LobbyIdStr = TCHAR_TO_UTF8(*LobbyId);

    // Setup the options for joining by lobby ID
    EOS_Lobby_JoinLobbyByIdOptions JoinByIdOptions = {};
    JoinByIdOptions.ApiVersion = EOS_LOBBY_JOINLOBBYBYID_API_LATEST;
    JoinByIdOptions.LobbyId = LobbyIdStr.c_str();
    JoinByIdOptions.LocalUserId = LocalUserId;
    JoinByIdOptions.bPresenceEnabled = EOS_TRUE;

    // Join the lobby by ID
    EOS_Lobby_JoinLobbyById(
        LobbyHandle,
        &JoinByIdOptions,
        this,
        &UOnlineLobbyService::OnJoinLobbyByIdCallback
    );

    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Attempting to join lobby by ID: %s"), *LobbyId);
}

void EOS_CALL UOnlineLobbyService::OnJoinLobbyByIdCallback(const EOS_Lobby_JoinLobbyByIdCallbackInfo* Data)
{
    if (!Data)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Join lobby by ID callback received invalid data"));
        return;
    }
    
    UOnlineLobbyService* LobbyService = static_cast<UOnlineLobbyService*>(Data->ClientData);
    if (!LobbyService)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Join lobby by ID callback has invalid client data"));
        return;
    }
    
    bool bSuccess = (Data->ResultCode == EOS_EResult::EOS_Success);
    FString ErrorMessage;
    
    if (bSuccess)
    {
        // Store the lobby ID that we joined
        LobbyService->ActiveLobbyId = UTF8_TO_TCHAR(Data->LobbyId);
        UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Successfully joined lobby by ID: %s"), *LobbyService->ActiveLobbyId);
    }
    else
    {
        // Convert the EOS result code to a string description
        const char* ResultString = EOS_EResult_ToString(Data->ResultCode);
        ErrorMessage = FString::Printf(TEXT("Failed to join lobby: %s (%d)"), UTF8_TO_TCHAR(ResultString), Data->ResultCode);
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] %s"), *ErrorMessage);
    }
    
    // Execute the delegate on the game thread
    // AsyncTask(ENamedThreads::GameThread, [LobbyService, bSuccess, ErrorMessage]() {
    //    LobbyService->OnLobbyJoinedDelegate.Broadcast(bSuccess, ErrorMessage);
    //});
}

// Function to join a lobby from search results
void UOnlineLobbyService::JoinLobby(int32 LobbyIndex)
{
    if (!LobbyHandle)
    {
        // Try to initialize the lobby interface
        if (!InitializeLobbyInterface())
        {
            UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to initialize Lobby Interface"));
            //OnLobbyJoinedDelegate.Broadcast(false, TEXT("Lobby interface is not available"));
            return;
        }
    }

    // Check if we have valid search results
    if (!LobbySearchHandle)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] No active lobby search"));
       // OnLobbyJoinedDelegate.Broadcast(false, TEXT("No active lobby search"));
        return;
    }

    // Verify lobby index is valid
    if (LobbyIndex < 0 || LobbyIndex >= LobbySearchResults.Num())
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Invalid lobby index: %d"), LobbyIndex);
       // OnLobbyJoinedDelegate.Broadcast(false, TEXT("Invalid lobby index"));
        return;
    }

    // Get the local user's Product User ID
    EOS_ProductUserId LocalUserId = SessionAuthenticator->GetAuthenticatedProductUserId();
    if (!LocalUserId)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Invalid Local User ID"));
       // OnLobbyJoinedDelegate.Broadcast(false, TEXT("Local user ID is invalid"));
        return;
    }

    // Get lobby details handle from search results
    EOS_LobbySearch_GetSearchResultCountOptions ResultCountOptions = {};
    ResultCountOptions.ApiVersion = EOS_LOBBYSEARCH_GETSEARCHRESULTCOUNT_API_LATEST;
    
    uint32_t ResultCount = EOS_LobbySearch_GetSearchResultCount(LobbySearchHandle, &ResultCountOptions);
    
    if (LobbyIndex >= static_cast<int32>(ResultCount))
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Lobby index out of range"));
       // OnLobbyJoinedDelegate.Broadcast(false, TEXT("Lobby index out of range"));
        return;
    }

    // Get the lobby details from the search results
    EOS_LobbySearch_CopySearchResultByIndexOptions CopyOptions = {};
    CopyOptions.ApiVersion = EOS_LOBBYSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST;
    CopyOptions.LobbyIndex = static_cast<uint32_t>(LobbyIndex);
    
    EOS_HLobbyDetails LobbyDetailsHandle = nullptr;
    EOS_EResult CopyResult = EOS_LobbySearch_CopySearchResultByIndex(LobbySearchHandle, &CopyOptions, &LobbyDetailsHandle );
    
    if (CopyResult != EOS_EResult::EOS_Success)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to copy lobby details: %d"), CopyResult);
       // OnLobbyJoinedDelegate.Broadcast(false, TEXT("Failed to retrieve lobby details"));
        return;
    }

    // Setup the options for joining the lobby
    EOS_Lobby_JoinLobbyOptions JoinOptions = {};
    JoinOptions.ApiVersion = EOS_LOBBY_JOINLOBBY_API_LATEST;
    JoinOptions.LobbyDetailsHandle = LobbyDetailsHandle;
    JoinOptions.LocalUserId = LocalUserId;
    JoinOptions.bPresenceEnabled = EOS_TRUE;

    // Join the lobby
    EOS_Lobby_JoinLobby(LobbyHandle, &JoinOptions,this, &UOnlineLobbyService::OnJoinLobbyCallback);

    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Attempting to join lobby at index: %d, ID: %s"),  LobbyIndex, *LobbySearchResults[LobbyIndex]);
}

// Callback for JoinLobby
void UOnlineLobbyService::OnJoinLobbyCallback(const EOS_Lobby_JoinLobbyCallbackInfo* Data)
{
    if (!Data)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Join lobby callback received invalid data"));
        return;
    }
    UOnlineLobbyService* LobbyService = static_cast<UOnlineLobbyService*>(Data->ClientData);
    if (!LobbyService)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Join lobby callback has invalid client data"));
        return;
    }
    bool bSuccess = (Data->ResultCode == EOS_EResult::EOS_Success);
    FString ErrorMessage;
    if (bSuccess)
    {
        // Store the lobby ID that we joined
        LobbyService->ActiveLobbyId = UTF8_TO_TCHAR(Data->LobbyId);
        UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Successfully joined lobby: %s"), *LobbyService->ActiveLobbyId);
        
        // You'll need to get the lobby details separately if needed
        // Unlike what your code tried to do, the callback doesn't give you a LobbyDetailsHandle
    }
    else
    {
        ErrorMessage = FString::Printf(TEXT("Failed to join lobby: %d"), Data->ResultCode);
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] %s"), *ErrorMessage);
    }
    
    // Remove these lines since LobbyDetailsHandle doesn't exist in the callback
    // if (Data->LobbyDetailsHandle)
    // {
    //     EOS_LobbyDetails_Release(Data->LobbyDetailsHandle);
    // }
    
    // Execute the delegate on the game thread
    // AsyncTask(ENamedThreads::GameThread, [LobbyService, bSuccess, ErrorMessage]() {
    //     LobbyService->OnLobbyJoinedDelegate.Broadcast(bSuccess, ErrorMessage);
    // });
}

bool UOnlineLobbyService::SendLobbyInvite(const FString& RecipientProductUserId)
{
    // Validate necessary components
    if (!LobbyHandle)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Cannot send invite: Lobby interface not initialized"));
        return false;
    }

    if (ActiveLobbyId.IsEmpty())
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Cannot send invite: No active lobby"));
        return false;
    }

    // Get the local user's Product User ID
    EOS_ProductUserId LocalUserId = SessionAuthenticator->GetAuthenticatedProductUserId();
    if (!LocalUserId)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Cannot send invite: Invalid local user ID"));
        return false;
    }

    // Convert recipient string to EOS_ProductUserId
    EOS_ProductUserId TargetUserId = EOS_ProductUserId_FromString(TCHAR_TO_UTF8(*RecipientProductUserId));
    if (!TargetUserId)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Cannot send invite: Invalid recipient user ID"));
        return false;
    }

    // Set up the invite options
    EOS_Lobby_SendInviteOptions InviteOptions = {};
    InviteOptions.ApiVersion = EOS_LOBBY_SENDINVITE_API_LATEST;
    InviteOptions.LobbyId = TCHAR_TO_UTF8(*ActiveLobbyId);
    InviteOptions.LocalUserId = LocalUserId;
    InviteOptions.TargetUserId = TargetUserId;

    // Send the invite
    EOS_Lobby_SendInvite(LobbyHandle, &InviteOptions,this,&UOnlineLobbyService::OnSendInviteCompleteCallback );

    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Sending invite to %s for lobby %s"), *RecipientProductUserId, *ActiveLobbyId);
    
    return true;
}

// Callback for SendInvite
void EOS_CALL UOnlineLobbyService::OnSendInviteCompleteCallback(const EOS_Lobby_SendInviteCallbackInfo* Data)
{
    UOnlineLobbyService* Service = static_cast<UOnlineLobbyService*>(Data->ClientData);
    if (!Service)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Invalid service instance in send invite callback"));
        return;
    }

    bool bSuccess = (Data->ResultCode == EOS_EResult::EOS_Success);
    
    if (bSuccess)
    {
        UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Successfully sent invite for lobby %s"), 
               UTF8_TO_TCHAR(Data->LobbyId));
    }
    else
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to send invite: %d"), Data->ResultCode);
    }

    // Execute delegate on game thread
   // AsyncTask(ENamedThreads::GameThread, [Service, bSuccess]() {
   //     Service->OnInviteSentDelegate.Broadcast(bSuccess);
   // });
}

// Callback for invite received
void UOnlineLobbyService::OnLobbyInviteReceivedCallback(const EOS_Lobby_LobbyInviteReceivedCallbackInfo* Data)
{
    UOnlineLobbyService* Service = static_cast<UOnlineLobbyService*>(Data->ClientData);
    if (!Service)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Invalid service instance in invite received callback"));
        return;
    }
    
    // Store invite information
    FString InviteId = UTF8_TO_TCHAR(Data->InviteId);
    
    // Convert ProductUserId to string properly
    char SenderUserIdBuffer[EOS_PRODUCTUSERID_MAX_LENGTH + 1];
    int32_t SenderUserIdBufferLength = sizeof(SenderUserIdBuffer);
    EOS_EResult Result = EOS_ProductUserId_ToString(Data->TargetUserId, SenderUserIdBuffer, &SenderUserIdBufferLength);
    
    FString SenderId;
    if (Result == EOS_EResult::EOS_Success)
    {
        SenderId = UTF8_TO_TCHAR(SenderUserIdBuffer);
    }
    else
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to convert sender user ID to string"));
        SenderId = TEXT("Unknown");
    }
    
    // Store the pending invite in our cache
    Service->PendingInvites.Add(InviteId, SenderId);
    
    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Received invite %s from user %s"), *InviteId, *SenderId);
    // Execute delegate on game thread
    // AsyncTask(ENamedThreads::GameThread, [Service, InviteId, SenderId]() {
    //    Service->OnLobbyInviteReceivedDelegate.Broadcast(InviteId, SenderId);
    // });
}

bool UOnlineLobbyService::AcceptLobbyInvite(const FString& InviteId)
{
    // Validate necessary components
    if (!LobbyHandle)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Cannot accept invite: Lobby interface not initialized"));
        return false;
    }

    // Get the local user's Product User ID
    EOS_ProductUserId LocalUserId = SessionAuthenticator->GetAuthenticatedProductUserId();
    if (!LocalUserId)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Cannot accept invite: Invalid local user ID"));
        return false;
    }

    // First, we need to get the LobbyDetails from the invite
    EOS_Lobby_CopyLobbyDetailsHandleByInviteIdOptions CopyOptions = {};
    CopyOptions.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLEBYINVITEID_API_LATEST;
    CopyOptions.InviteId = TCHAR_TO_UTF8(*InviteId);

    EOS_HLobbyDetails LobbyDetailsHandle = nullptr;
    EOS_EResult CopyResult = EOS_Lobby_CopyLobbyDetailsHandleByInviteId( LobbyHandle, &CopyOptions, &LobbyDetailsHandle );

    if (CopyResult != EOS_EResult::EOS_Success || !LobbyDetailsHandle)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to get lobby details from invite: %d"), CopyResult);
        return false;
    }

    // Now join the lobby using the details handle
    EOS_Lobby_JoinLobbyOptions JoinOptions = {};
    JoinOptions.ApiVersion = EOS_LOBBY_JOINLOBBY_API_LATEST;
    JoinOptions.LobbyDetailsHandle = LobbyDetailsHandle;
    JoinOptions.LocalUserId = LocalUserId;
    JoinOptions.bPresenceEnabled = EOS_TRUE;  // Enable presence for social overlay
    JoinOptions.bCrossplayOptOut = EOS_FALSE; // Allow crossplay
    
    // For RTC Rooms if your game uses them
    JoinOptions.RTCRoomJoinActionType = EOS_ELobbyRTCRoomJoinActionType::EOS_LRRJAT_AutomaticJoin;

    // Join the lobby
    EOS_Lobby_JoinLobby(LobbyHandle,&JoinOptions, this, &UOnlineLobbyService::OnJoinLobbyCallback);

    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Accepting invite %s"), *InviteId);
    
    // The details handle is now owned by the JoinLobby operation
    // We'll release it in the callback if needed
    
    return true;
}

bool UOnlineLobbyService::RejectLobbyInvite(const FString& InviteId)
{
    // Validate necessary components
    if (!LobbyHandle)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Cannot reject invite: Lobby interface not initialized"));
        return false;
    }

    // Get the local user's Product User ID
    EOS_ProductUserId LocalUserId = SessionAuthenticator->GetAuthenticatedProductUserId();
    if (!LocalUserId)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Cannot reject invite: Invalid local user ID"));
        return false;
    }

    // Set up the reject options
    EOS_Lobby_RejectInviteOptions RejectOptions = {};
    RejectOptions.ApiVersion = EOS_LOBBY_REJECTINVITE_API_LATEST;
    RejectOptions.InviteId = TCHAR_TO_UTF8(*InviteId);
    RejectOptions.LocalUserId = LocalUserId;

    // Reject the invite
    EOS_Lobby_RejectInvite(LobbyHandle, &RejectOptions, this, &UOnlineLobbyService::OnRejectInviteCompleteCallback);

    UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Rejecting invite %s"), *InviteId);
    
    return true;
}

// Callback for RejectInvite
void  UOnlineLobbyService::OnRejectInviteCompleteCallback(const EOS_Lobby_RejectInviteCallbackInfo* Data)
{
    UOnlineLobbyService* Service = static_cast<UOnlineLobbyService*>(Data->ClientData);
    if (!Service)
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Invalid service instance in reject invite callback"));
        return;
    }

    bool bSuccess = (Data->ResultCode == EOS_EResult::EOS_Success);
    FString InviteId = UTF8_TO_TCHAR(Data->InviteId);
    
    if (bSuccess)
    {
        UE_LOG(LogOnlineLobbyService, Log, TEXT("[EOS Lobby] Successfully rejected invite %s"), *InviteId);
        
        // Remove from pending invites
        Service->PendingInvites.Remove(InviteId);
    }
    else
    {
        UE_LOG(LogOnlineLobbyService, Error, TEXT("[EOS Lobby] Failed to reject invite: %d"), Data->ResultCode);
    }

    // Execute delegate on game thread
   // AsyncTask(ENamedThreads::GameThread, [Service, bSuccess, InviteId]() {
   //     Service->OnInviteRejectedDelegate.Broadcast(bSuccess, InviteId);
  //  });
}

//--------------------------------------------------------RTC Lobby ---------------------------------------------------------//

