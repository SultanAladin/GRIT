//AuthenticationContext.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AuthenticationContext.generated.h"

//------------------------------------------------------------------------------
//                          AUTHENTICATION CONTEXT GAMEMODE
//------------------------------------------------------------------------------
/** Gamemode for authentication/login screen */
UCLASS(BlueprintType, Blueprintable)
class GRIT_API AAuthenticationContext : public AGameModeBase
{
    GENERATED_BODY()

public:
    AAuthenticationContext();

protected:
    virtual void BeginPlay() override;
};
