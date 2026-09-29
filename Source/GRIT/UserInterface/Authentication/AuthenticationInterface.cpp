// CRITICAL CHANGES in ProcessPostLoginActions():
// 1. REMOVED direct World->ServerTravel() call (line 451 in original)
// 2. ADDED AAuthenticationController->RequestLobbyTravel() call
// 3. Travel now goes through server RPC in PlayerController
//
// This fixes the GameMode not changing issue because:
// - Widget no longer calls ServerTravel (widgets are client-only)
// - PlayerController has authority to execute server travel
// - Server properly evaluates new level's GameMode

#include "AuthenticationInterface.h"
#include "AuthenticationController.h"
#include "EpicAdapter/OnlineSessionAuthenticator.h"
#include "EpicAdapter/OnlineLobbyService.h"
#include "../../GameContext/SessionAdapter.h"
#include "../../GameContext/UserPreferences.h"
#include "EOSErrorTranslator.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "HAL/PlatformProcess.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

//------------------------------------------------------------------------------
// NOTE: NativeConstruct explicitly overwrites blueprint widget text values
// with C++ configuration values (LabelContent, SignInButtonText).
// LoginStatus text is set by ProcessLoginState based on InitialLoginState.
//
// Post-login: Travel now uses AAuthenticationController->RequestLobbyTravel()
// instead of direct ServerTravel() to ensure proper GameMode evaluation.
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

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        AuthenticatorSubsystem = GameInstance->GetSubsystem<UOnlineSessionAuthenticator>();
        if (!AuthenticatorSubsystem) // Reason: Critical subsystem check
        {
            UE_LOG(LogTemp, Error, TEXT("[AuthInterface] Failed to get UOnlineSessionAuthenticator subsystem"));
        } // End if (AuthenticatorSubsystem)
    }

    SessionAdapter = Cast<USessionAdapter>(GetGameInstance());
    if (!SessionAdapter) // Reason: Theme features require SessionAdapter
    {
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] SessionAdapter not found - theme features disabled"));
    } // End if (SessionAdapter)

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        LobbyService = GameInstance->GetSubsystem<UOnlineLobbyService>();
        if (!LobbyService) // Reason: Lobby features require LobbyService
        {
            UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Failed to get UOnlineLobbyService subsystem"));
        } // End if (LobbyService)
    }

    if (Logo && LogoSpec.BaseMaterial)
    {
        LogoDynamicMaterial = UMaterialInstanceDynamic::Create(LogoSpec.BaseMaterial, this);
        if (LogoDynamicMaterial) // Reason: Material created successfully
        {
            if (LogoSpec.Texture) // Reason: Apply texture if provided
            {
                for (const FName& ParamName : LogoSpec.ParameterNames) LogoDynamicMaterial->SetTextureParameterValue(ParamName, LogoSpec.Texture);
            } // End if (LogoSpec.Texture)
            LogoDynamicMaterial->SetVectorParameterValue(FName("Color"), LogoSpec.Color);
            Logo->SetBrushFromMaterial(LogoDynamicMaterial);
        } // End if (LogoDynamicMaterial)
    }

    if (LoginIndicator)
    {
        FLinearColor InitialColor = RetrieveIndicatorColor();
        LoginIndicator->SetColorAndOpacity(InitialColor);
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] LoginIndicator initialized with direct tinting: (%.2f,%.2f,%.2f,%.2f)"), InitialColor.R, InitialColor.G, InitialColor.B, InitialColor.A);
        
        if (IndicatorSpec.BaseMaterial) // Reason: Create dynamic material if spec provided
        {
            IndicatorDynamicMaterial = UMaterialInstanceDynamic::Create(IndicatorSpec.BaseMaterial, this);
            if (IndicatorDynamicMaterial) // Reason: Material created successfully
            {
                if (IndicatorSpec.Texture) // Reason: Apply texture if provided
                {
                    for (const FName& ParamName : IndicatorSpec.ParameterNames) IndicatorDynamicMaterial->SetTextureParameterValue(ParamName, IndicatorSpec.Texture);
                } // End if (IndicatorSpec.Texture)
                LoginIndicator->SetBrushFromMaterial(IndicatorDynamicMaterial);
                UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Dynamic material created but using direct tinting for color"));
            } // End if (IndicatorDynamicMaterial)
        } // End if (IndicatorSpec.BaseMaterial)
    }

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

    if (SignInButton) SignInButton->OnClicked.AddDynamic(this, &UAuthenticationInterface::ProcessSignInClicked);
    if (ExitButton) ExitButton->OnClicked.AddDynamic(this, &UAuthenticationInterface::ProcessExitClicked);

    // Reason: Bind to global LoginMethodManager for label updates
    if (ULoginMethodManager* LoginMethodManager = ULoginMethodManager::GetInstance())
    {
        LoginMethodManager->OnLoginMethodChanged.AddDynamic(this, &UAuthenticationInterface::ProcessLoginMethodChanged);
        RefreshLoginMethodLabel();
        UE_LOG(LogTemp, Log, TEXT("[AuthInterface] LoginMethodManager bound for label updates"));
    } // End if (LoginMethodManager exists)

    if (AuthenticatorSubsystem)
    {
        AuthenticatorSubsystem->OnLoginSuccess.AddDynamic(this, &UAuthenticationInterface::ProcessLoginSuccess);
        AuthenticatorSubsystem->OnLoginFailure.AddDynamic(this, &UAuthenticationInterface::ProcessLoginFailure);
    }

    if (LobbyService) LobbyService->OnLobbyCreatedDelegate.AddDynamic(this, &UAuthenticationInterface::ProcessLobbyCreated);

    ProcessLoginState(InitialLoginState);

    // Reason: Check if already logged in (auto-login detection)
    if (AuthenticatorSubsystem && AuthenticatorSubsystem->GetEOSLoginStatus() == EOS_ELoginStatus::EOS_LS_LoggedIn)
    {
        SetLoginState(ELoginState::LoggedIn);
        
        if (SessionAdapter && SessionAdapter->HasValidLobbyState())
        {
            FLobbyState StoredLobby = SessionAdapter->GetLobbyState();
            UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Found stored lobby - ID: %s, attempting rejoin"), *StoredLobby.LobbyId);
            
            if (LobbyService) // Reason: Attempt to rejoin existing lobby
            {
                LobbyService->JoinLobbyById(StoredLobby.LobbyId);
            } // End if (LobbyService)
        } // End if (HasValidLobbyState)
        else
        {
            ProcessPostLoginActions();
        } // End else (No stored lobby)
    } // End if (already logged in)
    
    ProcessIndicatorAndStatus();
}

void UAuthenticationInterface::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
}

void UAuthenticationInterface::BeginDestroy()
{
    InvalidateDelegates();
    Super::BeginDestroy();
}

//------------------------------------------------------------------------------
//                           DELEGATE MANAGEMENT
//------------------------------------------------------------------------------

void UAuthenticationInterface::InvalidateDelegates()
{
    if (AuthenticatorSubsystem)
    {
        AuthenticatorSubsystem->OnLoginSuccess.RemoveDynamic(this, &UAuthenticationInterface::ProcessLoginSuccess);
        AuthenticatorSubsystem->OnLoginFailure.RemoveDynamic(this, &UAuthenticationInterface::ProcessLoginFailure);
    } // End if (AuthenticatorSubsystem)
    
    if (LobbyService) LobbyService->OnLobbyCreatedDelegate.RemoveDynamic(this, &UAuthenticationInterface::ProcessLobbyCreated);
    
    if (ULoginMethodManager* LoginMethodManager = ULoginMethodManager::GetInstance()) LoginMethodManager->OnLoginMethodChanged.RemoveDynamic(this, &UAuthenticationInterface::ProcessLoginMethodChanged);
} // End InvalidateDelegates

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
    FLinearColor StateColor = RetrieveIndicatorColor();
    FText StatusText = RetrieveDefaultStatusText();
    
    if (LoginIndicator)
    {
        LoginIndicator->SetColorAndOpacity(StateColor);
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Indicator direct tint updated - Color: (%.2f,%.2f,%.2f,%.2f)"), StateColor.R, StateColor.G, StateColor.B, StateColor.A);
    }
    
    if (LoginStatus)
    {
        LoginStatus->SetText(StatusText);
        LoginStatus->SetColorAndOpacity(FSlateColor(StateColor));
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Status updated - Text: %s, Color: (%.2f,%.2f,%.2f,%.2f)"), *StatusText.ToString(), StateColor.R, StateColor.G, StateColor.B, StateColor.A);
    }
}

void UAuthenticationInterface::ProcessButtonStates()
{
    if (SignInButton)
    {
        bool bShouldEnable = (CurrentLoginState == ELoginState::None || CurrentLoginState == ELoginState::LoggedOut || CurrentLoginState == ELoginState::Error) && !bAuthenticationInProgress;
        SignInButton->SetIsEnabled(bShouldEnable);
        
        if (SignInText)
        {
            switch (CurrentLoginState)
            {
                case ELoginState::LoggingIn:  SignInText->SetText(FText::FromString("Signing In...")); break;
                case ELoginState::LoggedIn:   SignInText->SetText(FText::FromString("Connected")); break;
                case ELoginState::Offline:    SignInText->SetText(FText::FromString("Offline Mode")); break;
                default:                      SignInText->SetText(SignInButtonText); break;
            } // End switch (CurrentLoginState)
        } // End if (SignInText)
    } // End if (SignInButton)
} // End ProcessButtonStates

void UAuthenticationInterface::ProcessStatusColors(const FString& StatusMessage, const FLinearColor& StatusColor)
{
    if (LoginIndicator) LoginIndicator->SetColorAndOpacity(StatusColor);
    if (LoginStatus)
    {
        LoginStatus->SetText(FText::FromString(StatusMessage));
        LoginStatus->SetColorAndOpacity(FSlateColor(StatusColor));
    }
}

//------------------------------------------------------------------------------
//                              PUBLIC API
//------------------------------------------------------------------------------

void UAuthenticationInterface::SetLoginState(ELoginState NewState)
{
    ProcessLoginState(NewState);
}

void UAuthenticationInterface::SetStatusText(const FText& NewText)
{
    if (LoginStatus) LoginStatus->SetText(NewText);
}

void UAuthenticationInterface::SetLabelText(const FText& NewText)
{
    if (Label) Label->SetText(NewText);
}

void UAuthenticationInterface::RefreshVisuals()
{
    ProcessIndicatorAndStatus();
    ProcessButtonStates();
}

ELoginMethod UAuthenticationInterface::GetPreferredLoginMethod() const
{
    if (SessionAdapter)
    {
        if (UUserPreferencesManager* PrefsManager = SessionAdapter->GetUserPreferencesManager())
        {
            FUserPreferences Prefs = PrefsManager->GetUserPreferences();
            return Prefs.PreferredLoginMethod;
        } // End if (PrefsManager)
    } // End if (SessionAdapter)
    
    return ELoginMethod::AccountPortal;
}

FString UAuthenticationInterface::GetErrorMessage(int32 ErrorCode) const
{
    return UEOSErrorTranslator::TranslateErrorCode(ErrorCode);
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
    UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Processing post-login actions"));

    FString LevelPath;
    if (LevelPath.IsEmpty())
    {
        FSoftObjectPath SoftPath = LobbyLevel.ToSoftObjectPath();
        LevelPath = SoftPath.GetLongPackageName();
        
        if (LevelPath.IsEmpty()) // Reason: Invalid path validation
        {
            UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Invalid lobby level path"));
            ProcessStatusColors(TEXT("Error: Invalid lobby level path"), StateColors.ErrorColor);
            return;
        } // End if (LevelPath empty check)
    } // End if (LevelPath validation)

    UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Loading lobby level: %s"), *LevelPath);
    
    if (bAutoCreateLobbyOnLogin) ProcessStatusColors(TEXT("Setting up lobby..."), StateColors.LoggingInColor);
    else ProcessStatusColors(TEXT("Loading lobby level..."), StateColors.LoggingInColor);

    //--------------------------------------------------------------------------
    // CRITICAL FIX: Use AAuthenticationController for server travel
    //--------------------------------------------------------------------------

    AAuthenticationController* AuthController = Cast<AAuthenticationController>(GetOwningPlayer());
    if (!AuthController) // Reason: PlayerController cast failed
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthInterface] Failed to get AAuthenticationController"));
        ProcessStatusColors(TEXT("Error: PlayerController not found"), StateColors.ErrorColor);
        return;
    } // End if (AuthController validation)

    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] REQUESTING LOBBY TRAVEL"));
    UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Target Level: %s"), *LevelPath);
    UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Using AAuthenticationController RPC"));
    UE_LOG(LogTemp, Warning, TEXT("========================================"));

    AuthController->RequestLobbyTravel(LevelPath);
    UE_LOG(LogTemp, Log, TEXT("[AuthInterface] RequestLobbyTravel called"));

    // Reason: Create lobby session after delay if enabled
    if (bAutoCreateLobbyOnLogin)
    {
        if (UWorld* World = GetWorld()) // Reason: World required for timer
        {
            FTimerHandle LobbyCreationTimer;
            World->GetTimerManager().SetTimer(LobbyCreationTimer, this, &UAuthenticationInterface::CreateLobbySession, 2.0f, false); // [s] - 2 second delay
            UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Lobby session will be created in 2 seconds"));
        } // End if (World)
    } // End if (bAutoCreateLobbyOnLogin)
} // End ProcessPostLoginActions

void UAuthenticationInterface::ProcessOfflineMode()
{
    UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Processing offline mode"));

    // Reason: Get computer username for offline mode
    FString ComputerUsername = FPlatformProcess::UserName();
    UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Offline username: %s"), *ComputerUsername);
    
    ProcessLoginState(ELoginState::Offline);
    ProcessStatusColors(FString::Printf(TEXT("Offline - %s"), *ComputerUsername), StateColors.OfflineColor);

    FString LevelPath;
    FSoftObjectPath SoftPath = LobbyLevel.ToSoftObjectPath();
    LevelPath = SoftPath.GetLongPackageName();
    
    if (LevelPath.IsEmpty()) // Reason: Invalid path validation
    {
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Invalid level path for offline mode"));
        ProcessStatusColors(TEXT("Error: Invalid level path"), StateColors.ErrorColor);
        return;
    } // End if (LevelPath empty check)

    UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Loading level in offline mode: %s"), *LevelPath);
    ProcessStatusColors(TEXT("Loading level (Offline)..."), StateColors.OfflineColor);

    AAuthenticationController* AuthController = Cast<AAuthenticationController>(GetOwningPlayer());
    if (!AuthController) // Reason: PlayerController cast failed
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthInterface] Failed to get AAuthenticationController for offline mode"));
        ProcessStatusColors(TEXT("Error: PlayerController not found"), StateColors.ErrorColor);
        return;
    } // End if (AuthController validation)

    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] OFFLINE MODE - DIRECT TRAVEL"));
    UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Target Level: %s"), *LevelPath);
    UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Username: %s"), *ComputerUsername);
    UE_LOG(LogTemp, Warning, TEXT("========================================"));

    AuthController->RequestLobbyTravel(LevelPath);
    UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Offline travel request sent"));
}

void UAuthenticationInterface::CreateLobbySession()
{
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
    
    const ELobbyPermissionLevel PermissionLevel = ELobbyPermissionLevel::PublicAdvertised;
    const bool bEnablePresence = true;
    const bool bAllowInvites = true;
    const bool bAllowHostMigration = false;
    const bool bEnableRTCRoom = false;
    const TMap<FString, FString> StringAttributes;
    const TMap<FString, int64> IntAttributes;
    const TMap<FString, bool> BoolAttributes;
    const TMap<FString, float> FloatAttributes;

    LobbyService->OpenLobbyInstance(MaxLobbyMembers, PermissionLevel, bEnablePresence, bAllowInvites, LobbyBucketId, bAllowHostMigration, bEnableRTCRoom, StringAttributes, IntAttributes, BoolAttributes, FloatAttributes);
} // End CreateLobbySession

//------------------------------------------------------------------------------
//                              HELPER FUNCTIONS
//------------------------------------------------------------------------------

FText UAuthenticationInterface::RetrieveDefaultStatusText() const
{
    switch (CurrentLoginState)
    {
        case ELoginState::None:       return FText::FromString("Not Connected");
        case ELoginState::LoggedOut:  return FText::FromString("Signed Out");
        case ELoginState::LoggingIn:  return FText::FromString("Signing In...");
        case ELoginState::LoggedIn:   return FText::FromString("Connected");
        case ELoginState::Offline:    return FText::FromString("Offline");
        case ELoginState::Error:      return FText::FromString("Connection Error");
        default:                      return FText::FromString("Unknown");
    } // End switch (CurrentLoginState)
} // End RetrieveDefaultStatusText

FLinearColor UAuthenticationInterface::RetrieveIndicatorColor() const
{
    switch (CurrentLoginState)
    {
        case ELoginState::None:       return StateColors.NoneColor;
        case ELoginState::LoggedOut:  return StateColors.LoggedOutColor;
        case ELoginState::LoggingIn:  return StateColors.LoggingInColor;
        case ELoginState::LoggedIn:   return StateColors.LoggedInColor;
        case ELoginState::Offline:    return StateColors.OfflineColor;
        case ELoginState::Error:      return StateColors.ErrorColor;
        default:                      return StateColors.NoneColor;
    } // End switch (CurrentLoginState)
} // End RetrieveIndicatorColor

FLinearColor UAuthenticationInterface::RetrieveStatusColor(ELoginState State) const
{
    return RetrieveIndicatorColor();
}

//------------------------------------------------------------------------------
//                            DELEGATE HANDLERS
//------------------------------------------------------------------------------

void UAuthenticationInterface::ProcessSignInClicked()
{
    // Reason: Check if already logged in - transfer to lobby directly
    if (CurrentLoginState == ELoginState::LoggedIn)
    {
        UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Already logged in - transferring to lobby"));
        ProcessPostLoginActions();
        return;
    } // End if (already logged in)

    // Reason: Prevent multiple login attempts
    if (bAuthenticationInProgress) return;

    // Reason: Get preferred login method from user preferences
    ELoginMethod PreferredMethod = GetPreferredLoginMethod();
    UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Starting login with method: %s"), *ULoginMethodManager::GetLoginMethodDisplayName(PreferredMethod).ToString());

    // Reason: Handle Offline mode separately
    if (PreferredMethod == ELoginMethod::Offline)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Offline mode selected - skipping authentication"));
        ProcessOfflineMode();
        return;
    } // End if (Offline mode)

    if (AuthenticatorSubsystem)
    {
        bAuthenticationInProgress = true;
        
        // Reason: Start authentication timeout timer
        if (UWorld* World = GetWorld()) World->GetTimerManager().SetTimer(AuthTimeoutHandle, this, &UAuthenticationInterface::ProcessAuthenticationTimeout, AuthTimeoutSeconds, false);
        
        ProcessLoginState(ELoginState::LoggingIn);
        
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
                UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] DevAuth not implemented, fallback to PersistentAuth"));
                AuthenticatorSubsystem->LoginWithPersistentAuth();
                break;
            case ELoginMethod::ExchangeCode:
                UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] ExchangeCode not implemented, fallback to PersistentAuth"));
                AuthenticatorSubsystem->LoginWithPersistentAuth();
                break;
            default:
                UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Unknown login method, fallback to PersistentAuth"));
                AuthenticatorSubsystem->LoginWithPersistentAuth();
                break;
        } // End switch (PreferredMethod)
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthInterface] Cannot sign in - AuthenticatorSubsystem is null"));
        ProcessLoginState(ELoginState::Error);
    } // End if (AuthenticatorSubsystem)
} // End ProcessSignInClicked

void UAuthenticationInterface::ProcessLoginSuccess()
{
    if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(AuthTimeoutHandle);
    
    bAuthenticationInProgress = false;
    ProcessLoginState(ELoginState::LoggedIn);
    
    UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Login successful"));
    
    if (SessionAdapter && SessionAdapter->HasValidLobbyState())
    {
        FLobbyState StoredLobby = SessionAdapter->GetLobbyState();
        UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Found stored lobby - ID: %s, attempting rejoin"), *StoredLobby.LobbyId);
        
        if (LobbyService) LobbyService->JoinLobbyById(StoredLobby.LobbyId);
    } // End if (HasValidLobbyState)
    else
    {
        ProcessPostLoginActions();
    } // End else (No stored lobby)
}

void UAuthenticationInterface::ProcessLoginFailure(int32 ErrorCode)
{
    if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(AuthTimeoutHandle);
    
    bAuthenticationInProgress = false;
    ProcessLoginState(ELoginState::Error);
    
    UE_LOG(LogTemp, Error, TEXT("[AuthInterface] Login failed with error code: %d"), ErrorCode);
    
    if (LoginStatus)
    {
        FString UserFriendlyMessage = UEOSErrorTranslator::GetUserFriendlyMessage(ErrorCode);
        FString DetailedMessage = UEOSErrorTranslator::GetDetailedErrorDescription(ErrorCode);
        
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Error %d - User Message: %s"), ErrorCode, *UserFriendlyMessage);
        UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Error %d - Detailed: %s"), ErrorCode, *DetailedMessage);
        
        FString DisplayMessage = FString::Printf(TEXT("%s (Error %d)"), *UserFriendlyMessage, ErrorCode);
        ProcessStatusColors(DisplayMessage, StateColors.ErrorColor);
    }
}

void UAuthenticationInterface::ProcessLobbyCreated(bool bSuccess, const FString& LobbyId)
{
    bLobbyCreationInProgress = false;
    
    if (bSuccess) // Reason: Lobby created successfully
    {
        UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Lobby created successfully - ID: %s"), *LobbyId);
        ProcessStatusColors(TEXT("Lobby created successfully!"), StateColors.LoggedInColor);
        
        if (SessionAdapter && AuthenticatorSubsystem) // Reason: Save lobby state to disk
        {
            FString OwnerProductUserId = FString();
            SessionAdapter->UpdateLobbyStateFromMetadata(LobbyId, TEXT("My Lobby"), OwnerProductUserId, LobbyBucketId, MaxLobbyMembers, true);
            UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Lobby state saved to preferences"));
        } // End if (SessionAdapter)
    } // End if (bSuccess)
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthInterface] Lobby creation failed"));
        ProcessStatusColors(TEXT("Failed to create lobby"), StateColors.ErrorColor);
    } // End else (Lobby creation failed)
}

void UAuthenticationInterface::ProcessAuthenticationTimeout()
{
    UE_LOG(LogTemp, Warning, TEXT("[AuthInterface] Authentication timeout after %.1f seconds - resetting login state"), AuthTimeoutSeconds);
    
    bAuthenticationInProgress = false;
    ProcessLoginState(ELoginState::Error);
    
    if (LoginStatus)
    {
        FString TimeoutMessage = FString::Printf(TEXT("Login timeout (%.0fs) - please try again"), AuthTimeoutSeconds);
        ProcessStatusColors(TimeoutMessage, StateColors.ErrorColor);
    }
}

void UAuthenticationInterface::ProcessExitClicked()
{
    UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Exit button clicked"));

#if WITH_EDITOR
    if (GEditor)
    {
        GEditor->RequestEndPlayMap();
        UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Ending PIE session"));
    } // End if (GEditor)
#else
    if (APlayerController* PC = GetOwningPlayer())
    {
        UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
        UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Quitting application"));
    } // End if (PlayerController)
#endif
} // End ProcessExitClicked

//------------------------------------------------------------------------------
//                        LOGIN METHOD LABEL MANAGEMENT
//------------------------------------------------------------------------------

void UAuthenticationInterface::ProcessLoginMethodChanged(ELoginMethod NewLoginMethod)
{
    UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Login method changed to: %s"), *ULoginMethodManager::GetLoginMethodDisplayName(NewLoginMethod).ToString());
    RefreshLoginMethodLabel();
}

void UAuthenticationInterface::RefreshLoginMethodLabel()
{
    if (Label_2)
    {
        if (ULoginMethodManager* LoginMethodManager = ULoginMethodManager::GetInstance())
        {
            ELoginMethod CurrentMethod = LoginMethodManager->GetCurrentLoginMethod();
            FText MethodDisplayName = ULoginMethodManager::GetLoginMethodDisplayName(CurrentMethod);
            Label_2->SetText(MethodDisplayName);
            
            UE_LOG(LogTemp, Log, TEXT("[AuthInterface] Login method label updated to: %s"), *MethodDisplayName.ToString());
        } // End if (LoginMethodManager exists)
    } // End if (Label_2 exists)
}
