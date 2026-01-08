#include "AuthenticationInterface.h"
#include "EntryInterface.h"
#include "EpicAdapter/OnlineSessionAuthenticator.h"
#include "EpicAdapter/OnlineLobbyService.h"
#include "SessionAdapter.h"
#include "../UserPreferences.h"
#include "EOSErrorTranslator.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/World.h"
#include "TimerManager.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

//------------------------------------------------------------------------------
// NOTE: NativeConstruct explicitly overwrites blueprint widget text values
// with C++ configuration values (LabelContent, SignInButtonText).
// LoginStatus text is set by ProcessLoginState based on InitialLoginState.
//
// Post-login: If bAutoCreateLobbyOnLogin is enabled, lobby creation is
// automatically triggered after successful authentication.
//------------------------------------------------------------------------------

UAuthenticationInterface::UAuthenticationInterface(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    CurrentLoginState = ELoginState::None;
    bAuthenticationInProgress = false;
}

//------------------------------------------------------------------------------
//                                  LIFECYCLE
//------------------------------------------------------------------------------

void UAuthenticationInterface::NativeConstruct()
{
    Super::NativeConstruct();

    InitializeAuthenticationSystem();
    InitializeSessionAdapter();
    InitializeLobbyService();
    InitializeMaterials();
    InitializeDefaultUITexts();
    InitializeLanguageSelector();
    BindButtonEvents();
    BindAuthenticatorDelegates();
    BindLobbyDelegates();

    // Set initial state
    ProcessLoginState(InitialLoginState);

    // Reason: Check if already logged in (auto-login detection)
    if (AuthenticatorSubsystem && AuthenticatorSubsystem->GetEOSLoginStatus() == EOS_ELoginStatus::EOS_LS_LoggedIn)
    {
        SetLoginState(ELoginState::LoggedIn);
        
        // Reason: Trigger entry animation for auto-login
        if (EntryInterface)
        {
            if (AuthenticatorSubsystem)
            {
                FString PlayerNickname = AuthenticatorSubsystem->GetPlayerNickname();
                if (!PlayerNickname.IsEmpty())
                {
                    EntryInterface->SetUsername(PlayerNickname);
                }
            }
            EntryInterface->TriggerEntrySequence();
            UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Auto-login detected - EntryInterface animation triggered"));
        }
    } // End if (auto-login check)
    
    // IMPORTANT: Force initial color update to ensure both elements start with correct colors
    ProcessIndicatorAndStatus();
}

void UAuthenticationInterface::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
}

void UAuthenticationInterface::BeginDestroy()
{
    UnbindAuthenticatorDelegates();
    UnbindLobbyDelegates();
    Super::BeginDestroy();
}

//------------------------------------------------------------------------------
//                           INITIALIZATION PIPELINE
//------------------------------------------------------------------------------

void UAuthenticationInterface::InitializeAuthenticationSystem()
{
    // Reason: Get authenticator subsystem reference
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        AuthenticatorSubsystem = GameInstance->GetSubsystem<UOnlineSessionAuthenticator>();
        
        if (!AuthenticatorSubsystem) // Reason: Log if subsystem missing
        {
            UE_LOG(LogTemp, Error, TEXT("[AuthInterface] Failed to get UOnlineSessionAuthenticator subsystem"));
        }
    } // End if (GameInstance check)
}

void UAuthenticationInterface::InitializeSessionAdapter()
{
    // Reason: Get session adapter for theme configuration
    SessionAdapter = Cast<USessionAdapter>(GetGameInstance());
    
    if (!SessionAdapter) // Reason: Log if session adapter missing
    {
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] SessionAdapter not found - theme features disabled"));
    }
} // End InitializeSessionAdapter

void UAuthenticationInterface::InitializeLobbyService()
{
    // Reason: Get lobby service subsystem reference
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        LobbyService = GameInstance->GetSubsystem<UOnlineLobbyService>();
        
        if (!LobbyService) // Reason: Log if subsystem missing
        {
            UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Failed to get UOnlineLobbyService subsystem"));
        }
    } // End if (GameInstance check)
} // End InitializeLobbyService

void UAuthenticationInterface::InitializeMaterials()
{
    // Reason: Create dynamic material for logo
    if (Logo && LogoSpec.BaseMaterial)
    {
        LogoDynamicMaterial = UMaterialInstanceDynamic::Create(LogoSpec.BaseMaterial, this);
        
        if (LogoDynamicMaterial) // Reason: Set logo texture and color parameters
        {
            if (LogoSpec.Texture)
            {
                for (const FName& ParamName : LogoSpec.ParameterNames)
                {
                    LogoDynamicMaterial->SetTextureParameterValue(ParamName, LogoSpec.Texture);
                } // End for (LogoSpec.ParameterNames)
            }
            
            LogoDynamicMaterial->SetVectorParameterValue(FName("Color"), LogoSpec.Color);
            Logo->SetBrushFromMaterial(LogoDynamicMaterial);
        } // End if (LogoDynamicMaterial created)
    } // End if (Logo and base material exist)

    // Reason: Create dynamic material for login indicator
    if (LoginIndicator)
    {
        // SIMPLIFIED: Always use direct image tinting for reliable color control
        // This works regardless of whether a material is assigned or not
        FLinearColor InitialColor = RetrieveIndicatorColor();
        LoginIndicator->SetColorAndOpacity(InitialColor);
        
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] LoginIndicator initialized with direct tinting: (%.2f,%.2f,%.2f,%.2f)"), 
               InitialColor.R, InitialColor.G, InitialColor.B, InitialColor.A);
        
        // If there's a base material configured, we can still try to create the dynamic material
        // but we'll rely on direct tinting for color control
        if (IndicatorSpec.BaseMaterial)
        {
            IndicatorDynamicMaterial = UMaterialInstanceDynamic::Create(IndicatorSpec.BaseMaterial, this);
            if (IndicatorDynamicMaterial)
            {
                // Set texture parameters if available
                if (IndicatorSpec.Texture)
                {
                    for (const FName& ParamName : IndicatorSpec.ParameterNames)
                    {
                        IndicatorDynamicMaterial->SetTextureParameterValue(ParamName, IndicatorSpec.Texture);
                    }
                }
                
                // Apply the material but rely on image tinting for color
                LoginIndicator->SetBrushFromMaterial(IndicatorDynamicMaterial);
                UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Dynamic material created but using direct tinting for color"));
            }
        }
    }
} // End InitializeMaterials

void UAuthenticationInterface::InitializeDefaultUITexts()
{
    // Reason: Overwrite blueprint text with C++ config values
    if (Label)
    {
        Label->SetText(LabelContent);
        UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Label text set to: %s"), *LabelContent.ToString());
    }
    
    if (SignInText)
    {
        SignInText->SetText(SignInButtonText);
        UE_LOG(LogTemp, Log, TEXT("[AuthInterface] SignInText set to: %s"), *SignInButtonText.ToString());
    }
    
    // Note: LoginStatus text is set by ProcessLoginState
} // End InitializeDefaultUITexts

void UAuthenticationInterface::InitializeLanguageSelector()
{
    // Reason: Language selector is custom widget - initialized by blueprint
    if (LanguageSelector)
    {
        UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Language selector widget found"));
    }
} // End InitializeLanguageSelector

void UAuthenticationInterface::BindButtonEvents()
{
    // Reason: Bind sign-in button click event
    if (SignInButton)
    {
        SignInButton->OnClicked.AddDynamic(this, &UAuthenticationInterface::ProcessSignInClicked);
    }
} // End BindButtonEvents

void UAuthenticationInterface::BindAuthenticatorDelegates()
{
    // Reason: Subscribe to authentication events
    if (AuthenticatorSubsystem)
    {
        AuthenticatorSubsystem->OnLoginSuccess.AddDynamic(this, &UAuthenticationInterface::ProcessLoginSuccess);
        AuthenticatorSubsystem->OnLoginFailure.AddDynamic(this, &UAuthenticationInterface::ProcessLoginFailure);
    }
} // End BindAuthenticatorDelegates

void UAuthenticationInterface::UnbindAuthenticatorDelegates()
{
    // Reason: Clean up authentication event subscriptions
    if (AuthenticatorSubsystem)
    {
        AuthenticatorSubsystem->OnLoginSuccess.RemoveDynamic(this, &UAuthenticationInterface::ProcessLoginSuccess);
        AuthenticatorSubsystem->OnLoginFailure.RemoveDynamic(this, &UAuthenticationInterface::ProcessLoginFailure);
    }
} // End UnbindAuthenticatorDelegates

void UAuthenticationInterface::BindLobbyDelegates()
{
    // Reason: Subscribe to lobby events
    if (LobbyService)
    {
        LobbyService->OnLobbyCreatedDelegate.AddDynamic(this, &UAuthenticationInterface::ProcessLobbyCreated);
    }
} // End BindLobbyDelegates

void UAuthenticationInterface::UnbindLobbyDelegates()
{
    // Reason: Clean up lobby event subscriptions
    if (LobbyService)
    {
        LobbyService->OnLobbyCreatedDelegate.RemoveDynamic(this, &UAuthenticationInterface::ProcessLobbyCreated);
    }
} // End UnbindLobbyDelegates

//------------------------------------------------------------------------------
//                              UI UPDATE PIPELINE
//------------------------------------------------------------------------------

void UAuthenticationInterface::ProcessLoginState(ELoginState NewState)
{
    CurrentLoginState = NewState;
    
    ProcessIndicatorAndStatus();
    ProcessButtonStates();
}

void UAuthenticationInterface::ProcessIndicatorAndStatus()
{
    // SIMPLIFIED: Always use direct image tinting for reliable color synchronization
    FLinearColor StateColor = RetrieveIndicatorColor();
    FText StatusText = RetrieveDefaultStatusText();
    
    // Update indicator with direct tinting (works with any material)
    if (LoginIndicator)
    {
        LoginIndicator->SetColorAndOpacity(StateColor);
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Indicator direct tint updated - Color: (%.2f,%.2f,%.2f,%.2f)"), 
               StateColor.R, StateColor.G, StateColor.B, StateColor.A);
    }
    
    // Update status text with same color
    if (LoginStatus)
    {
        LoginStatus->SetText(StatusText);
        LoginStatus->SetColorAndOpacity(FSlateColor(StateColor));
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Status updated - Text: %s, Color: (%.2f,%.2f,%.2f,%.2f)"), 
               *StatusText.ToString(), StateColor.R, StateColor.G, StateColor.B, StateColor.A);
    }
}

void UAuthenticationInterface::ProcessButtonStates()
{
    // Reason: Enable/disable sign-in button based on state
    if (SignInButton)
    {
        bool bShouldEnable = (CurrentLoginState == ELoginState::None || 
                             CurrentLoginState == ELoginState::LoggedOut || 
                             CurrentLoginState == ELoginState::Error) && 
                            !bAuthenticationInProgress;
        
        SignInButton->SetIsEnabled(bShouldEnable);
    }
} // End ProcessButtonStates

void UAuthenticationInterface::ProcessStatusColors(const FString& StatusMessage, const FLinearColor& StatusColor)
{
    // SIMPLIFIED: Always use direct image tinting for reliable synchronization
    if (LoginIndicator)
    {
        LoginIndicator->SetColorAndOpacity(StatusColor);
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Indicator direct tint updated - Color: (%.2f,%.2f,%.2f,%.2f)"), 
               StatusColor.R, StatusColor.G, StatusColor.B, StatusColor.A);
    }
    
    if (LoginStatus)
    {
        LoginStatus->SetText(FText::FromString(StatusMessage));
        LoginStatus->SetColorAndOpacity(FSlateColor(StatusColor));
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Status updated - Text: %s, Color: (%.2f,%.2f,%.2f,%.2f)"), 
               *StatusMessage, StatusColor.R, StatusColor.G, StatusColor.B, StatusColor.A);
    }
} // End ProcessStatusColors

//------------------------------------------------------------------------------
//                               PUBLIC API
//------------------------------------------------------------------------------

void UAuthenticationInterface::SetLoginState(ELoginState NewState)
{
    ProcessLoginState(NewState);
}

void UAuthenticationInterface::SetStatusText(const FText& NewText)
{
    // Reason: Set custom status text
    if (LoginStatus)
    {
        LoginStatus->SetText(NewText);
    }
} // End SetStatusText

void UAuthenticationInterface::SetLabelText(const FText& NewText)
{
    // Reason: Set custom label text
    if (Label)
    {
        Label->SetText(NewText);
    }
} // End SetLabelText

void UAuthenticationInterface::RefreshVisuals()
{
    ProcessIndicatorAndStatus();
    ProcessButtonStates();
}

ELoginMethod UAuthenticationInterface::GetPreferredLoginMethod() const
{
    // Get preferred login method from user preferences
    if (SessionAdapter)
    {
        if (UUserPreferencesManager* PrefsManager = SessionAdapter->GetUserPreferencesManager())
        {
            FUserPreferences CurrentPrefs = PrefsManager->GetUserPreferences();
            UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Using preferred login method: %s"), 
                   *ULoginMethodManager::GetLoginMethodDisplayName(CurrentPrefs.PreferredLoginMethod).ToString());
            return CurrentPrefs.PreferredLoginMethod;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] UserPreferencesManager not found, using default login method"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] SessionAdapter not found, using default login method"));
    }
    
    // Default fallback
    return ELoginMethod::AccountPortal;
}

FString UAuthenticationInterface::GetErrorMessage(int32 ErrorCode) const
{
    return UEOSErrorTranslator::GetUserFriendlyMessage(ErrorCode);
}

FString UAuthenticationInterface::GetDetailedErrorDescription(int32 ErrorCode) const
{
    return UEOSErrorTranslator::GetDetailedErrorDescription(ErrorCode);
}

bool UAuthenticationInterface::IsAuthenticationError(int32 ErrorCode) const
{
    return UEOSErrorTranslator::IsAuthenticationError(ErrorCode);
}

//------------------------------------------------------------------------------
//                          POST-LOGIN PIPELINE
//------------------------------------------------------------------------------

void UAuthenticationInterface::ProcessPostLoginActions()
{
    // Reason: Execute post-login workflow
    if (bAutoCreateLobbyOnLogin) // Reason: Auto-create lobby if enabled
    {
        CreateLobbySession();
    } // End if (auto-create lobby check)
} // End ProcessPostLoginActions

void UAuthenticationInterface::CreateLobbySession()
{
    // Reason: Prevent multiple simultaneous lobby creations
    if (bLobbyCreationInProgress)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Lobby creation already in progress"));
        return;
    } // End if (lobby creation already in progress)

    if (!LobbyService) // Reason: Validate lobby service exists
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthInterface] Cannot create lobby - LobbyService is null"));
        return;
    } // End if (LobbyService validation)

    bLobbyCreationInProgress = true;
    
    UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Creating lobby - MaxMembers: %d, Bucket: %s"), MaxLobbyMembers, *LobbyBucketId);
    
    // Map simplified CreateLobby request onto OpenLobbyInstance API
    const ELobbyPermissionLevel PermissionLevel = ELobbyPermissionLevel::PublicAdvertised;
    const bool bEnablePresence = true;
    const bool bAllowInvites = true;
    const bool bAllowHostMigration = false;
    const bool bEnableRTCRoom = false;
    const TMap<FString, FString> StringAttributes; // No extra attributes from this UI
    const TMap<FString, int64> IntAttributes;
    const TMap<FString, bool> BoolAttributes;
    const TMap<FString, float> FloatAttributes;

    LobbyService->OpenLobbyInstance(
        MaxLobbyMembers,
        PermissionLevel,
        bEnablePresence,
        bAllowInvites,
        LobbyBucketId,
        bAllowHostMigration,
        bEnableRTCRoom,
        StringAttributes,
        IntAttributes,
        BoolAttributes,
        FloatAttributes);
} // End CreateLobbySession

//------------------------------------------------------------------------------
//                              HELPER FUNCTIONS
//------------------------------------------------------------------------------

FText UAuthenticationInterface::RetrieveDefaultStatusText() const
{
    // Reason: Get status text based on current login state
    switch (CurrentLoginState)
    {
        case ELoginState::None:
            return FText::FromString("Not Connected");
        case ELoginState::LoggedOut:
            return FText::FromString("Signed Out");
        case ELoginState::LoggingIn:
            return FText::FromString("Signing In...");
        case ELoginState::LoggedIn:
            return FText::FromString("Connected");
        case ELoginState::Offline:
            return FText::FromString("Offline");
        case ELoginState::Error:
            return FText::FromString("Connection Error");
        default:
            return FText::FromString("Unknown");
    } // End switch (CurrentLoginState)
} // End RetrieveDefaultStatusText

FLinearColor UAuthenticationInterface::RetrieveIndicatorColor() const
{
    // Reason: Get indicator color based on current state
    switch (CurrentLoginState)
    {
        case ELoginState::None:
            return StateColors.NoneColor;
        case ELoginState::LoggedOut:
            return StateColors.LoggedOutColor;
        case ELoginState::LoggingIn:
            return StateColors.LoggingInColor;
        case ELoginState::LoggedIn:
            return StateColors.LoggedInColor;
        case ELoginState::Offline:
            return StateColors.OfflineColor;
        case ELoginState::Error:
            return StateColors.ErrorColor;
        default:
            return StateColors.NoneColor;
    } // End switch (CurrentLoginState)
} // End RetrieveIndicatorColor

FLinearColor UAuthenticationInterface::RetrieveStatusColor(ELoginState State) const
{
    // FIXED: Ensure status text uses exact same color as indicator
    return RetrieveIndicatorColor();
}

//------------------------------------------------------------------------------
//                            DELEGATE HANDLERS
//------------------------------------------------------------------------------

void UAuthenticationInterface::ProcessSignInClicked()
{
    // Reason: User initiated sign-in
    if (bAuthenticationInProgress) // Reason: Prevent double-click
    {
        return;
    } // End if (authentication already in progress)

    if (AuthenticatorSubsystem) // Reason: Trigger EOS login flow
    {
        bAuthenticationInProgress = true;
        
        // Start authentication timeout timer
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(AuthTimeoutHandle, this, 
                &UAuthenticationInterface::HandleAuthenticationTimeout, 
                AuthTimeoutSeconds, false);
        }
        
        ProcessLoginState(ELoginState::LoggingIn);
        
        // Get preferred login method from user preferences
        ELoginMethod PreferredMethod = GetPreferredLoginMethod();
        
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Starting login with method: %s"), 
               *ULoginMethodManager::GetLoginMethodDisplayName(PreferredMethod).ToString());
        
        // Use the preferred login method
        switch (PreferredMethod)
        {
            case ELoginMethod::AccountPortal:
                UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Using Account Portal login"));
                AuthenticatorSubsystem->LoginWithAccountPortal();
                break;
            case ELoginMethod::PersistentAuth:
                UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Using Persistent Auth login"));
                AuthenticatorSubsystem->LoginWithPersistentAuth();
                break;
            case ELoginMethod::ConnectInterface:
                UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Using Connect Interface login"));
                AuthenticatorSubsystem->LoginWithConnectInterface();
                break;
            case ELoginMethod::DevAuth:
                UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] DevAuth not implemented in authenticator, falling back to PersistentAuth"));
                AuthenticatorSubsystem->LoginWithPersistentAuth();
                break;
            case ELoginMethod::ExchangeCode:
                UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] ExchangeCode not implemented in authenticator, falling back to PersistentAuth"));
                AuthenticatorSubsystem->LoginWithPersistentAuth();
                break;
            default:
                UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Unknown login method, falling back to PersistentAuth"));
                AuthenticatorSubsystem->LoginWithPersistentAuth();
                break;
        }
        
        OnSignInClicked.Broadcast();
    }
    else // Reason: No authenticator available
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthInterface] Cannot sign in - AuthenticatorSubsystem is null"));
        ProcessLoginState(ELoginState::Error);
    } // End if (AuthenticatorSubsystem check)
} // End ProcessSignInClicked

void UAuthenticationInterface::ProcessLoginSuccess()
{
    // Clear authentication timeout timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AuthTimeoutHandle);
    }
    
    bAuthenticationInProgress = false;
    ProcessLoginState(ELoginState::LoggedIn);
    
    OnLoginComplete.Broadcast();
    
    UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Login successful"));
    
    // Reason: Trigger entry animation for manual login
    if (EntryInterface)
    {
        if (AuthenticatorSubsystem)
        {
            FString PlayerNickname = AuthenticatorSubsystem->GetPlayerNickname();
            if (!PlayerNickname.IsEmpty())
            {
                EntryInterface->SetUsername(PlayerNickname);
            }
        }
        EntryInterface->TriggerEntrySequence();
        UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Manual login success - EntryInterface animation triggered"));
    } // End if (EntryInterface exists)
    
    // Reason: Execute post-login actions
    ProcessPostLoginActions();
}

void UAuthenticationInterface::ProcessLoginFailure(int32 ErrorCode)
{
    // Clear authentication timeout timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AuthTimeoutHandle);
    }
    
    bAuthenticationInProgress = false;
    ProcessLoginState(ELoginState::Error);
    
    UE_LOG(LogTemp, Error, TEXT("[AuthInterface] Login failed with error code: %d"), ErrorCode);
    
    if (LoginStatus) // Reason: Show translated error message
    {
        // Get user-friendly error message
        FString UserFriendlyMessage = UEOSErrorTranslator::GetUserFriendlyMessage(ErrorCode);
        FString DetailedMessage = UEOSErrorTranslator::GetDetailedErrorDescription(ErrorCode);
        
        // Log both messages for debugging
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Error %d - User Message: %s"), ErrorCode, *UserFriendlyMessage);
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Error %d - Detailed: %s"), ErrorCode, *DetailedMessage);
        
        // Display user-friendly message with error code
        FString DisplayMessage = FString::Printf(TEXT("%s (Error %d)"), *UserFriendlyMessage, ErrorCode);
        ProcessStatusColors(DisplayMessage, StateColors.ErrorColor);
    } // End if (LoginStatus exists)
} // End ProcessLoginFailure

void UAuthenticationInterface::ProcessLobbyCreated(bool bSuccess, const FString& LobbyId)
{
    bLobbyCreationInProgress = false;
    
    if (bSuccess) // Reason: Log successful lobby creation
    {
        UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Lobby created successfully - ID: %s"), *LobbyId);
    }
    else // Reason: Log lobby creation failure
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthInterface] Lobby creation failed"));
    } // End if (lobby creation success check)
} // End ProcessLobbyCreated

void UAuthenticationInterface::HandleAuthenticationTimeout()
{
    UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Authentication timeout after %.1f seconds - resetting login state"), AuthTimeoutSeconds);
    
    // Reset authentication state
    bAuthenticationInProgress = false;
    ProcessLoginState(ELoginState::Error);
    
    // Show timeout message
    if (LoginStatus)
    {
        FString TimeoutMessage = FString::Printf(TEXT("Login timeout (%.0fs) - please try again"), AuthTimeoutSeconds);
        ProcessStatusColors(TimeoutMessage, StateColors.ErrorColor);
    }
}

