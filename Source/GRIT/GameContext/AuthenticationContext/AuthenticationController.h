#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "AuthenticationController.generated.h"

UCLASS()
class GRIT_API AAuthenticationController : public APlayerController
{
    GENERATED_BODY()

public:
    AAuthenticationController(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void BeginPlay() override;

    /* ----- Login widget ----- */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Access", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UUserWidget> LoginWidgetClass;

    UPROPERTY(BlueprintReadOnly, Category = "Access", meta = (AllowPrivateAccess = "true"))
    UUserWidget* LoginWidgetInstance;

    UFUNCTION(BlueprintCallable, Category = "Access")
    void ShowLoginWidget();

    UFUNCTION(BlueprintCallable, Category = "Access")
    void HideLoginWidget();
};
