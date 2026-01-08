#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "LoginMethodTypes.generated.h"

/*====================================================================================================================================
                                                         LOGIN METHOD SYSTEM TYPES
======================================================================================================================================*/

UENUM(BlueprintType)
enum class ELoginMethod : uint8
{
    AccountPortal       UMETA(DisplayName = "Account Portal"),
    PersistentAuth      UMETA(DisplayName = "Persistent Auth"),
    ConnectInterface    UMETA(DisplayName = "Connect Interface"),
    DevAuth             UMETA(DisplayName = "Developer Auth"),
    ExchangeCode        UMETA(DisplayName = "Exchange Code")
};

// Global delegate for login method changes - any class can bind to this
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoginMethodChanged, ELoginMethod, NewLoginMethod);

/*====================================================================================================================================
                                                         LOGIN METHOD MANAGER (SINGLETON)
======================================================================================================================================*/

UCLASS(BlueprintType, Blueprintable)
class GRIT_API ULoginMethodManager : public UObject
{
    GENERATED_BODY()

public:
    /** Get the global login method manager instance */
    UFUNCTION(BlueprintCallable, Category = "LoginMethod", CallInEditor)
    static ULoginMethodManager* GetInstance();

    /** Set current login method and broadcast to all listeners */
    UFUNCTION(BlueprintCallable, Category = "LoginMethod")
    void SetCurrentLoginMethod(ELoginMethod NewLoginMethod);

    /** Get current login method */
    UFUNCTION(BlueprintPure, Category = "LoginMethod")
    ELoginMethod GetCurrentLoginMethod() const { return CurrentLoginMethod; }

    /** Get login method display name */
    UFUNCTION(BlueprintPure, Category = "LoginMethod")
    static FText GetLoginMethodDisplayName(ELoginMethod LoginMethod);

    /** Get login method description */
    UFUNCTION(BlueprintPure, Category = "LoginMethod")
    static FText GetLoginMethodDescription(ELoginMethod LoginMethod);

    /** Get all available login methods */
    UFUNCTION(BlueprintPure, Category = "LoginMethod")
    static TArray<ELoginMethod> GetAllLoginMethods();

    /** Global delegate that any class can bind to for login method changes */
    UPROPERTY(BlueprintAssignable, Category = "LoginMethod Events")
    FOnLoginMethodChanged OnLoginMethodChanged;

private:
    static ULoginMethodManager* Instance;

    UPROPERTY()
    ELoginMethod CurrentLoginMethod = ELoginMethod::AccountPortal;

    ULoginMethodManager();
};