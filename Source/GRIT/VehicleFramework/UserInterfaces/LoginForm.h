// Copyright 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Engine/World.h"
#include "GenericCarousel.h"
#include "../../GameContext/AuthenticationContext/EntryInterface.h"
#include "LoginForm.generated.h"

//------------------------------------------------------------------------------
//                                  login form widget
//------------------------------------------------------------------------------

UCLASS()
class GRIT_API ULoginForm : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Widget initialization */
	virtual void NativeConstruct() override;

	virtual void BeginDestroy() override;

	//------------------------------------------------------------------------------
	// Entry Interface Integration
	//------------------------------------------------------------------------------



protected:
	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* CanvasPanel_0;

	UPROPERTY(meta = (BindWidget))
	UGenericCarousel* Carousel; // Bind directly from viewport

	//------------------------------------------------------------------------------
	// Entry Interface Components
	//------------------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UEntryInterface* EntryInterface;

	//------------------------------------------------------------------------------
	// Level Configuration
	//------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels")
	TSoftObjectPtr<UWorld> LobbyLevel;

private:
	//------------------------------------------------------------------------------
	// Runtime state
	//------------------------------------------------------------------------------

	UPROPERTY()
	class UOnlineSessionAuthenticator* AuthenticatorSubsystem;

	FTimerHandle LevelLoadTimer;

	//------------------------------------------------------------------------------
	// Internal helpers
	//------------------------------------------------------------------------------

	void InitializeAuthenticator();
	void BindAuthenticationDelegates();
	void UnbindAuthenticationDelegates();
	void BindEntryInterfaceDelegates();

	UFUNCTION()
	void ProcessLoginSuccess();

	UFUNCTION()
	void ProcessAnimationComplete();

	void LoadLobbyLevel();

};