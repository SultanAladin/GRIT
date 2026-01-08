#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "../UserInterfaces/UIToolkit.h"
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

    /** Event delegates */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSignInClicked OnSignInClicked;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnLoginComplete OnLoginComplete;

    /** Set current login state and refresh UI */
    UFUNCTION(BlueprintCallable, Category = "EOS Auth")
    void SetLoginState(ELoginState NewState);

    /** Retrieve current login state */
    UFUNCTION(BlueprintCallable, Category = "EOS Auth")
    ELoginState RetrieveLoginState() const { return CurrentLoginState; }

    /** Set status text with color update */
    UFUNCTION(BlueprintCallable, Category = "EOS Auth")
    void SetStatusText(const FText& NewText);

    /** Set label text */
    UFUNCTION(BlueprintCallable, Category = "EOS Auth")
    void SetLabelText(const FText& NewText);

    /** Force visual refresh */
    UFUNCTION(BlueprintCallable, Category = "EOS Auth")
    void RefreshVisuals();

    /** Get preferred login method from user preferences */
    UFUNCTION(BlueprintCallable, Category = "EOS Auth")
    ELoginMethod GetPreferredLoginMethod() const;

    /** Get human-readable error message for error code */
    UFUNCTION(BlueprintCallable, Category = "EOS Auth")
    FString GetErrorMessage(int32 ErrorCode) const;

    /** Get detailed error description for error code */
    UFUNCTION(BlueprintCallable, Category = "EOS Auth")
    FString GetDetailedErrorDescription(int32 ErrorCode) const;

    /** Check if error code is authentication-related */
    UFUNCTION(BlueprintCallable, Category = "EOS Auth")
    bool IsAuthenticationError(int32 ErrorCode) const;

    /** Authentication timeout in seconds (configurable) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authentication", meta = (ClampMin = "5.0", ClampMax = "120.0"))
    float AuthTimeoutSeconds = 30.0f; // Default 30 seconds

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

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UUserWidget* LanguageSelector;


    //------------------------------------------------------------------------------
    // Configuration
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Colors")
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby Settings")
    bool bAutoCreateLobbyOnLogin = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby Settings")
    int32 MaxLobbyMembers = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby Settings")
    FString LobbyBucketId = TEXT("DefaultBucket");

    //------------------------------------------------------------------------------
    // Material configuration
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Config")
    FMaterialSpec LogoSpec;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Config")
    FMaterialSpec IndicatorSpec;

    //------------------------------------------------------------------------------
    // Entry Interface Components
    //------------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    class UEntryInterface* EntryInterface;

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

    // Authentication timeout handling
    FTimerHandle AuthTimeoutHandle;

    //------------------------------------------------------------------------------
    // Initialization pipeline
    //------------------------------------------------------------------------------

    void InitializeAuthenticationSystem();
    void InitializeSessionAdapter();
    void InitializeLobbyService();
    void InitializeMaterials();
    void InitializeDefaultUITexts();
    void InitializeLanguageSelector();
    void BindButtonEvents();
    void BindAuthenticatorDelegates();
    void UnbindAuthenticatorDelegates();
    void BindLobbyDelegates();
    void UnbindLobbyDelegates();

    //------------------------------------------------------------------------------
    // Post-login pipeline
    //------------------------------------------------------------------------------

    void ProcessPostLoginActions();
    void CreateLobbySession();

    //------------------------------------------------------------------------------
    // UI update pipeline
    //------------------------------------------------------------------------------

    void ProcessLoginState(ELoginState NewState);
    void ProcessButtonStates();
    void ProcessStatusColors(const FString& StatusMessage, const FLinearColor& StatusColor);
    
    /** Update both indicator and status text with synchronized colors */
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
    void ProcessLoginSuccess();

    UFUNCTION()
    void ProcessLoginFailure(int32 ErrorCode);

    UFUNCTION()
    void ProcessLobbyCreated(bool bSuccess, const FString& LobbyId);

    /** Handle authentication timeout */
    UFUNCTION()
    void HandleAuthenticationTimeout();

};
