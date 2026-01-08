// Copyright 2025. All Rights Reserved.

#include "LoginForm.h"
#include "../../GameContext/SessionAdapter.h"
#include "EpicAdapter/OnlineSessionAuthenticator.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "eos_sdk.h"

void ULoginForm::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (Carousel) { UE_LOG(LogTemp, Warning, TEXT("[LoginForm] Carousel bound successfully")); } // Verify binding
	
	// Initialize EntryInterface if bound
	if (EntryInterface)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LoginForm] EntryInterface bound successfully"));
		EntryInterface->ApplyThemeStyle();
		BindEntryInterfaceDelegates();
	}
	
	// Reason: Bind to authentication events
	InitializeAuthenticator();
	BindAuthenticationDelegates();
}

void ULoginForm::BeginDestroy()
{
	UnbindAuthenticationDelegates();
	
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(LevelLoadTimer);
	}
	
	Super::BeginDestroy();
}

//------------------------------------------------------------------------------
// Entry Interface Integration
//------------------------------------------------------------------------------

void ULoginForm::InitializeAuthenticator()
{
	// Reason: Get authenticator subsystem for auto-login detection
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		AuthenticatorSubsystem = GameInstance->GetSubsystem<UOnlineSessionAuthenticator>();
		
		if (!AuthenticatorSubsystem) // Reason: Log if subsystem missing
		{
			UE_LOG(LogTemp, Error, TEXT("[LoginForm] Failed to get UOnlineSessionAuthenticator subsystem"));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[LoginForm] AuthenticatorSubsystem initialized"));
			
			// Reason: Check if already logged in (auto-login detection)
			if (AuthenticatorSubsystem->GetEOSLoginStatus() == EOS_ELoginStatus::EOS_LS_LoggedIn)
			{
				UE_LOG(LogTemp, Log, TEXT("[LoginForm] Auto-login detected"));
				
				// Reason: Check if nickname already available
				FString PlayerNickname = AuthenticatorSubsystem->GetPlayerNickname();
				if (!PlayerNickname.IsEmpty())
				{
					UE_LOG(LogTemp, Log, TEXT("[LoginForm] Username already available: %s"), *PlayerNickname);
					ProcessLoginSuccess();
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("[LoginForm] Username not yet loaded, retrieving account credentials"));
					// Reason: Trigger user details retrieval (OnUserDetailsReady will fire)
					EOS_EpicAccountId AccountId = AuthenticatorSubsystem->GetLocalEpicAccountId();
					if (AccountId != nullptr)
					{
						AuthenticatorSubsystem->RetrieveAccountCredentials(AccountId);
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[LoginForm] AccountId is null, cannot retrieve credentials"));
					}
				}
			}
		}
	} // End if (GameInstance check)
}

void ULoginForm::BindAuthenticationDelegates()
{
	// Reason: Bind to OnUserDetailsReady (fires after login with username available)
	if (AuthenticatorSubsystem)
	{
		AuthenticatorSubsystem->OnUserDetailsReady.AddDynamic(this, &ULoginForm::ProcessLoginSuccess);
		UE_LOG(LogTemp, Log, TEXT("[LoginForm] Bound to AuthenticatorSubsystem->OnUserDetailsReady delegate"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[LoginForm] AuthenticatorSubsystem not found - cannot bind delegate"));
	}
}

void ULoginForm::UnbindAuthenticationDelegates()
{
	// Reason: Clean up delegate subscriptions
	if (AuthenticatorSubsystem)
	{
		AuthenticatorSubsystem->OnUserDetailsReady.RemoveDynamic(this, &ULoginForm::ProcessLoginSuccess);
	}
}

void ULoginForm::BindEntryInterfaceDelegates()
{
	// Reason: Bind to animation complete delegate
	if (EntryInterface)
	{
		EntryInterface->OnAnimationComplete.AddDynamic(this, &ULoginForm::ProcessAnimationComplete);
		UE_LOG(LogTemp, Log, TEXT("[LoginForm] Bound to EntryInterface->OnAnimationComplete"));
	}
}

void ULoginForm::ProcessLoginSuccess()
{
	UE_LOG(LogTemp, Log, TEXT("[LoginForm] ProcessLoginSuccess called"));
	
	// Reason: Trigger entry animation
	if (EntryInterface)
	{
		if (AuthenticatorSubsystem)
		{
			FString PlayerNickname = AuthenticatorSubsystem->GetPlayerNickname();
			if (!PlayerNickname.IsEmpty())
			{
				EntryInterface->SetUsername(PlayerNickname);
				UE_LOG(LogTemp, Log, TEXT("[LoginForm] Set username: %s"), *PlayerNickname);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[LoginForm] PlayerNickname is empty"));
			}
		}
		EntryInterface->TriggerEntrySequence();
		UE_LOG(LogTemp, Log, TEXT("[LoginForm] EntryInterface animation triggered"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[LoginForm] EntryInterface not found - cannot play animation"));
	}
}

void ULoginForm::ProcessAnimationComplete()
{
	UE_LOG(LogTemp, Log, TEXT("[LoginForm] EntryInterface animation completed"));
	
	// Reason: Load lobby level after animation finishes
	LoadLobbyLevel();
}

void ULoginForm::LoadLobbyLevel()
{
	// Reason: Validate lobby level is configured
	if (LobbyLevel.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LoginForm] LobbyLevel is not set"));
		return;
	}
	
	FString LevelPath = LobbyLevel.GetLongPackageName();
	if (LevelPath.IsEmpty())
	{
		// Reason: Try to get path from soft object path directly
		FSoftObjectPath SoftPath = LobbyLevel.ToSoftObjectPath();
		LevelPath = SoftPath.GetLongPackageName();
		
		if (LevelPath.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("[LoginForm] Invalid lobby level path. SoftObjectPath: %s"), *SoftPath.ToString());
			return;
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("[LoginForm] Loading lobby level: %s"), *LevelPath);
	
	// Reason: Server travel to lobby level with listen server
	FString TravelURL = LevelPath + TEXT("?listen");
	
	if (GetWorld())
	{
		GetWorld()->ServerTravel(TravelURL);
		UE_LOG(LogTemp, Log, TEXT("[LoginForm] ServerTravel initiated to: %s"), *TravelURL);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[LoginForm] GetWorld() returned null - cannot load level"));
	}
}
