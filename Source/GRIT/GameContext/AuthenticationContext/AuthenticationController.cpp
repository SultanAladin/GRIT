#include "AuthenticationController.h"

#include "Blueprint/UserWidget.h"
#include "TimerManager.h"

AAuthenticationController::AAuthenticationController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , LoginWidgetInstance(nullptr)
{
}

void AAuthenticationController::BeginPlay()
{
    Super::BeginPlay();

    ShowLoginWidget();
}

void AAuthenticationController::ShowLoginWidget()
{
    if (!IsLocalController())
    {
        return;
    }

    if (LoginWidgetInstance)
    {
        return;
    }

    if (!LoginWidgetClass)
    {
        return;
    }

    LoginWidgetInstance = CreateWidget<UUserWidget>(this, LoginWidgetClass);
    if (!LoginWidgetInstance)
    {
        return;
    }

    LoginWidgetInstance->AddToViewport();

    bShowMouseCursor = true;

    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(LoginWidgetInstance->TakeWidget());
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
}

void AAuthenticationController::HideLoginWidget()
{
    if (LoginWidgetInstance)
    {
        LoginWidgetInstance->RemoveFromParent();
        LoginWidgetInstance = nullptr;
    }

    bShowMouseCursor = false;

    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);
}
