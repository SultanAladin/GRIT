#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "../../VehicleFramework/UserInterfaces/UIToolkit.h"
#include "../../VehicleFramework/UserInterfaces/LoginMethodTypes.h"
#include "EOSErrorTranslator.h"
#include "AuthenticationInterface.generated.h"

UENUM(BlueprintType)
enum class ELoginState : uint8
{
    None        UMETA(DisplayName = "None"),
    LoggedOut   UMETA(DisplayName = "Logged Out"),
    LoggingIn   UMETA(DisplayName = "Logging In"),
    LoggedIn    UMETA(DisplayName = "Logged In"),
    Offline     UMETA(DisplayName = "Offline"),
    Error       UMETA(DisplayName = "Error")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSignInClicked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoginComplete);

//------------------------------------------------------------------------------
//                           AUTHENTICATION STATE COLORS
//------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FAuthenticationStateColors
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Colors")
    FLinearColor NoneColor = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);  // [RGBA] - Dark Gray

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Colors")
    FLinearColor LoggedOutColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);  // [RGBA] - Gray

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Colors")
    FLinearColor LoggingInColor = FLinearColor(1.0f, 1.0f, 0.3f, 1.0f);  // [RGBA] - Yellow

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Colors")
    FLinearColor LoggedInColor = FLinearColor(0.0f, 0.8f, 0.2f, 1.0f);  // [RGBA] - Green

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Colors")
    FLinearColor OfflineColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);  // [RGBA] - Orange

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Colors")
    FLinearColor ErrorColor = FLinearColor(0.9f, 0.1f, 0.1f, 1.0f);  // [RGBA] - Red
};

//------------------------------------------------------------------------------
//                           AUTHENTICATION INTERFACE
//------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable)
class GRIT_API UAuthenticationInterface : public UUserWidget
{
    GENERATED_BODY()

public:
    UAuthenticationInterface(const FObjectInitializer& ObjectInitializer);

    /** Set current login state and refresh UI */
    void SetLoginState(ELoginState NewState);

    /** Retrieve current login state */
    ELoginState RetrieveLoginState() const { return CurrentLoginState; }

    /** Set status text with color update */
    void SetStatusText(const FText& NewText);

    /** Set label text */
    void SetLabelText(const FText& NewText);

    /** Force visual refresh */
    void RefreshVisuals();

    /** Get preferred login method from user preferences */
    ELoginMethod GetPreferredLoginMethod() const;

    /** Get human-readable error message for error code */
    FString GetErrorMessage(int32 ErrorCode) const;

    /** Get detailed error description for error code */
    FString GetDetailedErrorDescription(int32 ErrorCode) const;

    /** Check if error code is authentication-related */
    bool IsAuthenticationError(int32 ErrorCode) const;

    /** Authentication timeout in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    float AuthTimeoutSeconds = 30.0f;  // [s] - Default 30 seconds

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual void BeginDestroy() override;

    //------------------------------------------------------------------------------
    // Widget bindings
    //------------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UBorder* ContentBorder;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage* Logo;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* Label;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* SignInButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* SignInText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage* LoginIndicator;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* LoginStatus;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* ExitButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* Label_1;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* Label_2;

    //------------------------------------------------------------------------------
    // Configuration
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
    FAuthenticationStateColors StateColors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    FText LabelContent = FText::FromString("Sign in to access your games and content");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    FText SignInButtonText = FText::FromString("Login");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    ELoginState InitialLoginState = ELoginState::None;

    //------------------------------------------------------------------------------
    // Lobby configuration
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    TSoftObjectPtr<UWorld> LobbyLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/GTX/WorldArchives/AccessPoint/AccessPoint")));

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    bool bAutoCreateLobbyOnLogin = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    int32 MaxLobbyMembers = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    FString LobbyBucketId = TEXT("DefaultBucket");

    //------------------------------------------------------------------------------
    // Material configuration
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    FMaterialSpec LogoSpec;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    FMaterialSpec IndicatorSpec;

private:
    //------------------------------------------------------------------------------
    // Runtime state
    //------------------------------------------------------------------------------

    UPROPERTY()
    class UOnlineSessionAuthenticator* AuthenticatorSubsystem;

    UPROPERTY()
    class UOnlineLobbyService* LobbyService;

    UPROPERTY()
    class USessionAdapter* SessionAdapter;

    UPROPERTY()
    UMaterialInstanceDynamic* IndicatorDynamicMaterial;

    UPROPERTY()
    UMaterialInstanceDynamic* LogoDynamicMaterial;

    ELoginState CurrentLoginState = ELoginState::None;
    bool bAuthenticationInProgress = false;
    bool bLobbyCreationInProgress = false;
    FTimerHandle AuthTimeoutHandle;

    //------------------------------------------------------------------------------
    // Delegate management
    //------------------------------------------------------------------------------

    void InvalidateDelegates();

    //------------------------------------------------------------------------------
    // Post-login pipeline
    //------------------------------------------------------------------------------

    void ProcessPostLoginActions();
    void ProcessOfflineMode();
    void CreateLobbySession();

    //------------------------------------------------------------------------------
    // UI update pipeline
    //------------------------------------------------------------------------------

    void ProcessLoginState(ELoginState NewState);
    void ProcessButtonStates();
    void ProcessStatusColors(const FString& StatusMessage, const FLinearColor& StatusColor);
    void ProcessIndicatorAndStatus();

    //------------------------------------------------------------------------------
    // Helper functions
    //------------------------------------------------------------------------------

    FText RetrieveDefaultStatusText() const;
    FLinearColor RetrieveIndicatorColor() const;
    FLinearColor RetrieveStatusColor(ELoginState State) const;

    //------------------------------------------------------------------------------
    // Delegate handlers
    //------------------------------------------------------------------------------

    UFUNCTION()
    void ProcessSignInClicked();
    
    UFUNCTION()
    void ProcessExitClicked();

    UFUNCTION()
    void ProcessLoginSuccess();

    UFUNCTION()
    void ProcessLoginFailure(int32 ErrorCode);

    UFUNCTION()
    void ProcessLobbyCreated(bool bSuccess, const FString& LobbyId);

    UFUNCTION()
    void ProcessAuthenticationTimeout();

    UFUNCTION()
    void ProcessLoginMethodChanged(ELoginMethod NewLoginMethod);

    void RefreshLoginMethodLabel();
};
