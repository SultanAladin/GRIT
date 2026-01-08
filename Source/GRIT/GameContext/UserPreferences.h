#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameFramework/SaveGame.h"
#include "UserPreferences.generated.h"

// Forward declarations to avoid circular includes
enum class ELanguage : uint8;
enum class ELoginMethod : uint8;

/*====================================================================================================================================
                                                         USER PREFERENCES STRUCT
======================================================================================================================================*/

USTRUCT(BlueprintType)
struct GRIT_API FUserPreferences
{
    GENERATED_BODY()

    /** User's preferred language */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Preferences")
    ELanguage PreferredLanguage;

    /** User's preferred login method */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Preferences")
    ELoginMethod PreferredLoginMethod;

    /** Audio volume settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Preferences|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MasterVolume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Preferences|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MusicVolume = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Preferences|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SFXVolume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Preferences|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float VoiceVolume = 1.0f;

    /** Graphics settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Preferences|Graphics")
    int32 ResolutionWidth = 1920;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Preferences|Graphics")
    int32 ResolutionHeight = 1080;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Preferences|Graphics")
    bool bFullscreen = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Preferences|Graphics")
    int32 GraphicsQuality = 3; // 0=Low, 1=Medium, 2=High, 3=Epic

    /** Accessibility settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Preferences|Accessibility")
    bool bHighContrastMode = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Preferences|Accessibility")
    float UIScale = 1.0f;

    /** Gameplay settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Preferences|Gameplay")
    bool bInvertMouseY = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Preferences|Gameplay", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float MouseSensitivity = 1.0f;

    /** First time setup flags */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Preferences|Setup")
    bool bHasCompletedFirstTimeSetup = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Preferences|Setup")
    bool bShowTutorials = true;

    /** Default constructor */
    FUserPreferences();

    /** Check if preferences are valid */
    bool IsValid() const
    {
        return ResolutionWidth > 0 && ResolutionHeight > 0 && 
               MasterVolume >= 0.0f && MasterVolume <= 1.0f &&
               GraphicsQuality >= 0 && GraphicsQuality <= 3;
    }

    /** Reset to default values */
    void ResetToDefaults()
    {
        *this = FUserPreferences();
    }
};

/*====================================================================================================================================
                                                         USER PREFERENCES SAVE GAME
======================================================================================================================================*/

UCLASS(BlueprintType)
class GRIT_API UUserPreferencesSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UUserPreferencesSaveGame();

    /** The user preferences data */
    UPROPERTY(VisibleAnywhere, Category = "Save Data")
    FUserPreferences UserPreferences;

    /** Version number for save compatibility */
    UPROPERTY(VisibleAnywhere, Category = "Save Data")
    int32 SaveVersion = 1;

    /** Timestamp when preferences were last saved */
    UPROPERTY(VisibleAnywhere, Category = "Save Data")
    FDateTime LastSaved;

    /** Validate save data integrity */
    UFUNCTION(BlueprintCallable, Category = "Save Data")
    bool IsValidSaveData() const;
};

/*====================================================================================================================================
                                                         USER PREFERENCES MANAGER
======================================================================================================================================*/

UCLASS(BlueprintType)
class GRIT_API UUserPreferencesManager : public UObject
{
    GENERATED_BODY()

public:
    UUserPreferencesManager();

    /** Initialize the preferences manager */
    UFUNCTION(BlueprintCallable, Category = "User Preferences")
    void Initialize();

    /** Load user preferences from disk */
    UFUNCTION(BlueprintCallable, Category = "User Preferences")
    bool LoadUserPreferences();

    /** Save user preferences to disk */
    UFUNCTION(BlueprintCallable, Category = "User Preferences")
    bool SaveUserPreferences();

    /** Get current user preferences */
    UFUNCTION(BlueprintPure, Category = "User Preferences")
    FUserPreferences GetUserPreferences() const { return CurrentPreferences; }

    /** Set user preferences (does not auto-save) */
    UFUNCTION(BlueprintCallable, Category = "User Preferences")
    void SetUserPreferences(const FUserPreferences& NewPreferences);

    /** Apply preferences to global managers */
    UFUNCTION(BlueprintCallable, Category = "User Preferences")
    void ApplyPreferencesToGlobalManagers();

    /** Reset preferences to defaults */
    UFUNCTION(BlueprintCallable, Category = "User Preferences")
    void ResetToDefaults();

    /** Get the save file path */
    UFUNCTION(BlueprintPure, Category = "User Preferences")
    FString GetSaveFilePath() const;

    /** Get the save file path as absolute Windows path for debugging */
    UFUNCTION(BlueprintCallable, Category = "User Preferences")
    FString GetAbsoluteSaveFilePath() const;

    /** Check if save file exists */
    UFUNCTION(BlueprintPure, Category = "User Preferences")
    bool DoesSaveFileExist() const;

    //--------------------------------------------------------------------------
    // INDIVIDUAL PREFERENCE SETTERS (with auto-save)
    //--------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "User Preferences|Language")
    void SetPreferredLanguage(ELanguage NewLanguage, bool bSaveImmediately = true);

    UFUNCTION(BlueprintCallable, Category = "User Preferences|Login")
    void SetPreferredLoginMethod(ELoginMethod NewLoginMethod, bool bSaveImmediately = true);

    UFUNCTION(BlueprintCallable, Category = "User Preferences|Audio")
    void SetMasterVolume(float NewVolume, bool bSaveImmediately = true);

    UFUNCTION(BlueprintCallable, Category = "User Preferences|Graphics")
    void SetResolution(int32 Width, int32 Height, bool bSaveImmediately = true);

    UFUNCTION(BlueprintCallable, Category = "User Preferences|Graphics")
    void SetFullscreen(bool bNewFullscreen, bool bSaveImmediately = true);

    //--------------------------------------------------------------------------
    // EVENTS
    //--------------------------------------------------------------------------

    /** Broadcast when preferences change */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUserPreferencesChanged, const FUserPreferences&, NewPreferences);

    UPROPERTY(BlueprintAssignable, Category = "User Preferences Events")
    FOnUserPreferencesChanged OnUserPreferencesChanged;

    /** Broadcast when preferences are loaded */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUserPreferencesLoaded, bool, bLoadedSuccessfully);

    UPROPERTY(BlueprintAssignable, Category = "User Preferences Events")
    FOnUserPreferencesLoaded OnUserPreferencesLoaded;

    /** Broadcast when preferences are saved */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUserPreferencesSaved, bool, bSavedSuccessfully);

    UPROPERTY(BlueprintAssignable, Category = "User Preferences Events")
    FOnUserPreferencesSaved OnUserPreferencesSaved;

protected:
    /** Current user preferences */
    UPROPERTY()
    FUserPreferences CurrentPreferences;

    /** Project name for creating save directory */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save Settings")
    FString ProjectName = TEXT("GRIT");

    /** Save file name (without extension) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save Settings")
    FString SaveFileName = TEXT("UserPreferences");

    /** File extension for save file */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save Settings")
    FString FileExtension = TEXT(".md");

    /** User index for save system */
    UPROPERTY()
    int32 UserIndex = 0;

    /** Internal method to broadcast preference changes */
    void BroadcastPreferencesChanged();

    /** Create the save directory if it doesn't exist */
    bool EnsureSaveDirectoryExists() const;

    /** Convert current preferences to string format */
    FString ConvertPreferencesToString() const;

    /** Parse preferences from string content */
    bool ParsePreferencesFromString(const FString& Content);

    /** Helper methods for parsing enum values */
    ELanguage ParseLanguageFromString(const FString& LanguageString) const;
    ELoginMethod ParseLoginMethodFromString(const FString& LoginMethodString) const;
};