#include "UserPreferences.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Engine/Engine.h"
#include "../VehicleFramework/UserInterfaces/LanguageTypes.h"
#include "../VehicleFramework/UserInterfaces/LoginMethodTypes.h"

/*====================================================================================================================================
                                                         USER PREFERENCES STRUCT IMPLEMENTATION
======================================================================================================================================*/

FUserPreferences::FUserPreferences()
{
    // Set default values
    PreferredLanguage = ELanguage::English;
    PreferredLoginMethod = ELoginMethod::AccountPortal;
    MasterVolume = 1.0f;
    MusicVolume = 0.8f;
    SFXVolume = 1.0f;
    VoiceVolume = 1.0f;
    ResolutionWidth = 1920;
    ResolutionHeight = 1080;
    bFullscreen = true;
    GraphicsQuality = 3;
    bHighContrastMode = false;
    UIScale = 1.0f;
    bInvertMouseY = false;
    MouseSensitivity = 1.0f;
    bHasCompletedFirstTimeSetup = false;
    bShowTutorials = true;
}

/*====================================================================================================================================
                                                         USER PREFERENCES SAVE GAME
======================================================================================================================================*/

UUserPreferencesSaveGame::UUserPreferencesSaveGame()
{
    SaveVersion = 1;
    LastSaved = FDateTime::Now();
}

bool UUserPreferencesSaveGame::IsValidSaveData() const
{
    return UserPreferences.IsValid() && SaveVersion > 0;
}

/*====================================================================================================================================
                                                         USER PREFERENCES MANAGER
======================================================================================================================================*/

UUserPreferencesManager::UUserPreferencesManager()
{
    ProjectName = TEXT("GRIT");
    SaveFileName = TEXT("UserPreferences");
    FileExtension = TEXT(".md");
    UserIndex = 0;
}

void UUserPreferencesManager::Initialize()
{
    UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Initializing..."));
    
    // Try to load existing preferences
    bool bLoadedSuccessfully = LoadUserPreferences();
    
    if (!bLoadedSuccessfully)
    {
        UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: No existing preferences found, using defaults"));
        CurrentPreferences.ResetToDefaults();
        
        // Save default preferences
        SaveUserPreferences();
    }
    
    // Apply loaded/default preferences to global managers
    ApplyPreferencesToGlobalManagers();
    
    UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Initialization complete"));
}

bool UUserPreferencesManager::LoadUserPreferences()
{
    UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Loading preferences from '%s'"), *GetSaveFilePath());
    
    if (!DoesSaveFileExist())
    {
        UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Save file does not exist"));
        OnUserPreferencesLoaded.Broadcast(false);
        return false;
    }
    
    FString FileContent;
    if (!FFileHelper::LoadFileToString(FileContent, *GetSaveFilePath()))
    {
        UE_LOG(LogTemp, Error, TEXT("UserPreferencesManager: Failed to load file content"));
        OnUserPreferencesLoaded.Broadcast(false);
        return false;
    }
    
    // Parse the preferences from the file content
    if (!ParsePreferencesFromString(FileContent))
    {
        UE_LOG(LogTemp, Error, TEXT("UserPreferencesManager: Failed to parse preferences from file"));
        OnUserPreferencesLoaded.Broadcast(false);
        return false;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Successfully loaded preferences"));
    UE_LOG(LogTemp, Log, TEXT("  - Language: %s"), *ULanguageManager::GetLanguageDisplayName(CurrentPreferences.PreferredLanguage).ToString());
    UE_LOG(LogTemp, Log, TEXT("  - Login Method: %s"), *ULoginMethodManager::GetLoginMethodDisplayName(CurrentPreferences.PreferredLoginMethod).ToString());
    UE_LOG(LogTemp, Log, TEXT("  - Resolution: %dx%d"), CurrentPreferences.ResolutionWidth, CurrentPreferences.ResolutionHeight);
    UE_LOG(LogTemp, Log, TEXT("  - Master Volume: %.2f"), CurrentPreferences.MasterVolume);
    
    OnUserPreferencesLoaded.Broadcast(true);
    BroadcastPreferencesChanged();
    return true;
}

bool UUserPreferencesManager::SaveUserPreferences()
{
    UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Saving preferences to '%s'"), *GetSaveFilePath());
    UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Absolute path: '%s'"), *GetAbsoluteSaveFilePath());
    
    // Ensure save directory exists
    if (!EnsureSaveDirectoryExists())
    {
        UE_LOG(LogTemp, Error, TEXT("UserPreferencesManager: Failed to create save directory"));
        OnUserPreferencesSaved.Broadcast(false);
        return false;
    }
    
    // Convert preferences to string format
    FString PreferencesString = ConvertPreferencesToString();
    
    // Save to file
    bool bSaveSuccessful = FFileHelper::SaveStringToFile(PreferencesString, *GetSaveFilePath());
    
    if (bSaveSuccessful)
    {
        UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Successfully saved preferences"));
        UE_LOG(LogTemp, Log, TEXT("  - Save location: %s"), *GetSaveFilePath());
        UE_LOG(LogTemp, Log, TEXT("  - File size: %d bytes"), PreferencesString.Len());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UserPreferencesManager: Failed to save preferences"));
    }
    
    OnUserPreferencesSaved.Broadcast(bSaveSuccessful);
    return bSaveSuccessful;
}

void UUserPreferencesManager::SetUserPreferences(const FUserPreferences& NewPreferences)
{
    if (!NewPreferences.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("UserPreferencesManager: Attempted to set invalid preferences"));
        return;
    }
    
    CurrentPreferences = NewPreferences;
    BroadcastPreferencesChanged();
    
    UE_LOG(LogTemp, Log, TEXT("UserPreferencesManager: User preferences updated"));
}

void UUserPreferencesManager::ApplyPreferencesToGlobalManagers()
{
    UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Applying preferences to global managers"));
    
    // Apply language preference
    ULanguageManager* LanguageManager = ULanguageManager::GetInstance();
    if (LanguageManager)
    {
        LanguageManager->SetCurrentLanguage(CurrentPreferences.PreferredLanguage);
        UE_LOG(LogTemp, Log, TEXT("  - Applied language: %s"), 
               *ULanguageManager::GetLanguageDisplayName(CurrentPreferences.PreferredLanguage).ToString());
    }
    
    // Apply login method preference
    ULoginMethodManager* LoginMethodManager = ULoginMethodManager::GetInstance();
    if (LoginMethodManager)
    {
        LoginMethodManager->SetCurrentLoginMethod(CurrentPreferences.PreferredLoginMethod);
        UE_LOG(LogTemp, Log, TEXT("  - Applied login method: %s"), 
               *ULoginMethodManager::GetLoginMethodDisplayName(CurrentPreferences.PreferredLoginMethod).ToString());
    }
    
    // TODO: Apply other preferences (audio, graphics, etc.) to their respective systems
    // This would involve calling engine functions or other manager classes
}

void UUserPreferencesManager::ResetToDefaults()
{
    UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Resetting preferences to defaults"));
    
    CurrentPreferences.ResetToDefaults();
    BroadcastPreferencesChanged();
    
    // Apply defaults to global managers
    ApplyPreferencesToGlobalManagers();
    
    // Auto-save defaults
    SaveUserPreferences();
}

FString UUserPreferencesManager::GetSaveFilePath() const
{
    // Create path: SaveGames/ProjectName/SaveFileName.Extension
    // Example: SaveGames/GRIT/UserPreferences.md
    FString SaveGamesDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
    FString ProjectDir = SaveGamesDir / ProjectName;
    FString FullFileName = SaveFileName + FileExtension;
    return ProjectDir / FullFileName;
}

bool UUserPreferencesManager::DoesSaveFileExist() const
{
    return FPaths::FileExists(*GetSaveFilePath());
}

FString UUserPreferencesManager::GetAbsoluteSaveFilePath() const
{
    FString RelativePath = GetSaveFilePath();
    FString AbsolutePath = FPaths::ConvertRelativePathToFull(RelativePath);
    
    UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Absolute save path: %s"), *AbsolutePath);
    UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: File exists: %s"), DoesSaveFileExist() ? TEXT("YES") : TEXT("NO"));
    
    return AbsolutePath;
}

/*====================================================================================================================================
                                                         INDIVIDUAL PREFERENCE SETTERS
======================================================================================================================================*/

void UUserPreferencesManager::SetPreferredLanguage(ELanguage NewLanguage, bool bSaveImmediately)
{
    if (CurrentPreferences.PreferredLanguage == NewLanguage) return;
    
    CurrentPreferences.PreferredLanguage = NewLanguage;
    
    UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Set preferred language to %s"), 
           *ULanguageManager::GetLanguageDisplayName(NewLanguage).ToString());
    
    // Don't apply to global manager here to avoid circular calls
    // The UI component handles the global manager update
    
    BroadcastPreferencesChanged();
    
    if (bSaveImmediately)
    {
        UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Attempting to save preferences immediately..."));
        UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Save path will be: %s"), *GetSaveFilePath());
        bool bSaved = SaveUserPreferences();
        UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Save result: %s"), bSaved ? TEXT("SUCCESS") : TEXT("FAILED"));
    }
}

void UUserPreferencesManager::SetPreferredLoginMethod(ELoginMethod NewLoginMethod, bool bSaveImmediately)
{
    if (CurrentPreferences.PreferredLoginMethod == NewLoginMethod) return;
    
    CurrentPreferences.PreferredLoginMethod = NewLoginMethod;
    
    UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Set preferred login method to %s"), 
           *ULoginMethodManager::GetLoginMethodDisplayName(NewLoginMethod).ToString());
    
    // Don't apply to global manager here to avoid circular calls
    // The UI component handles the global manager update
    
    BroadcastPreferencesChanged();
    
    if (bSaveImmediately)
    {
        UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Attempting to save preferences immediately..."));
        UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Save path will be: %s"), *GetSaveFilePath());
        bool bSaved = SaveUserPreferences();
        UE_LOG(LogTemp, Warning, TEXT("UserPreferencesManager: Save result: %s"), bSaved ? TEXT("SUCCESS") : TEXT("FAILED"));
    }
}

void UUserPreferencesManager::SetMasterVolume(float NewVolume, bool bSaveImmediately)
{
    NewVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
    
    if (FMath::IsNearlyEqual(CurrentPreferences.MasterVolume, NewVolume, 0.01f)) return;
    
    CurrentPreferences.MasterVolume = NewVolume;
    
    UE_LOG(LogTemp, Log, TEXT("UserPreferencesManager: Set master volume to %.2f"), NewVolume);
    
    // TODO: Apply to audio system
    
    BroadcastPreferencesChanged();
    
    if (bSaveImmediately)
    {
        SaveUserPreferences();
    }
}

void UUserPreferencesManager::SetResolution(int32 Width, int32 Height, bool bSaveImmediately)
{
    if (Width <= 0 || Height <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("UserPreferencesManager: Invalid resolution %dx%d"), Width, Height);
        return;
    }
    
    if (CurrentPreferences.ResolutionWidth == Width && CurrentPreferences.ResolutionHeight == Height) return;
    
    CurrentPreferences.ResolutionWidth = Width;
    CurrentPreferences.ResolutionHeight = Height;
    
    UE_LOG(LogTemp, Log, TEXT("UserPreferencesManager: Set resolution to %dx%d"), Width, Height);
    
    // TODO: Apply to graphics system
    
    BroadcastPreferencesChanged();
    
    if (bSaveImmediately)
    {
        SaveUserPreferences();
    }
}

void UUserPreferencesManager::SetFullscreen(bool bNewFullscreen, bool bSaveImmediately)
{
    if (CurrentPreferences.bFullscreen == bNewFullscreen) return;
    
    CurrentPreferences.bFullscreen = bNewFullscreen;
    
    UE_LOG(LogTemp, Log, TEXT("UserPreferencesManager: Set fullscreen to %s"), bNewFullscreen ? TEXT("true") : TEXT("false"));
    
    // TODO: Apply to graphics system
    
    BroadcastPreferencesChanged();
    
    if (bSaveImmediately)
    {
        SaveUserPreferences();
    }
}

/*====================================================================================================================================
                                                         INTERNAL METHODS
======================================================================================================================================*/

void UUserPreferencesManager::BroadcastPreferencesChanged()
{
    OnUserPreferencesChanged.Broadcast(CurrentPreferences);
}

bool UUserPreferencesManager::EnsureSaveDirectoryExists() const
{
    FString SaveDir = FPaths::GetPath(GetSaveFilePath());
    
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*SaveDir))
    {
        UE_LOG(LogTemp, Log, TEXT("UserPreferencesManager: Creating save directory: %s"), *SaveDir);
        return PlatformFile.CreateDirectoryTree(*SaveDir);
    }
    
    return true;
}

FString UUserPreferencesManager::ConvertPreferencesToString() const
{
    FString Result;
    FDateTime Now = FDateTime::Now();
    
    Result += FString::Printf(TEXT("# %s User Preferences\n\n"), *ProjectName);
    Result += FString::Printf(TEXT("**Last Updated**: %s\n\n"), *Now.ToString());
    
    Result += TEXT("## Language & Authentication\n");
    Result += FString::Printf(TEXT("- **Preferred Language**: %s\n"), *ULanguageManager::GetLanguageDisplayName(CurrentPreferences.PreferredLanguage).ToString());
    Result += FString::Printf(TEXT("- **Preferred Login Method**: %s\n\n"), *ULoginMethodManager::GetLoginMethodDisplayName(CurrentPreferences.PreferredLoginMethod).ToString());
    
    Result += TEXT("## Audio Settings\n");
    Result += FString::Printf(TEXT("- **Master Volume**: %.2f\n"), CurrentPreferences.MasterVolume);
    Result += FString::Printf(TEXT("- **Music Volume**: %.2f\n"), CurrentPreferences.MusicVolume);
    Result += FString::Printf(TEXT("- **SFX Volume**: %.2f\n"), CurrentPreferences.SFXVolume);
    Result += FString::Printf(TEXT("- **Voice Volume**: %.2f\n\n"), CurrentPreferences.VoiceVolume);
    
    Result += TEXT("## Graphics Settings\n");
    Result += FString::Printf(TEXT("- **Resolution**: %dx%d\n"), CurrentPreferences.ResolutionWidth, CurrentPreferences.ResolutionHeight);
    Result += FString::Printf(TEXT("- **Fullscreen**: %s\n"), CurrentPreferences.bFullscreen ? TEXT("Yes") : TEXT("No"));
    Result += FString::Printf(TEXT("- **Graphics Quality**: %d\n\n"), CurrentPreferences.GraphicsQuality);
    
    Result += TEXT("## Accessibility Settings\n");
    Result += FString::Printf(TEXT("- **High Contrast Mode**: %s\n"), CurrentPreferences.bHighContrastMode ? TEXT("Yes") : TEXT("No"));
    Result += FString::Printf(TEXT("- **UI Scale**: %.2f\n\n"), CurrentPreferences.UIScale);
    
    Result += TEXT("## Gameplay Settings\n");
    Result += FString::Printf(TEXT("- **Invert Mouse Y**: %s\n"), CurrentPreferences.bInvertMouseY ? TEXT("Yes") : TEXT("No"));
    Result += FString::Printf(TEXT("- **Mouse Sensitivity**: %.2f\n\n"), CurrentPreferences.MouseSensitivity);
    
    Result += TEXT("## Setup Flags\n");
    Result += FString::Printf(TEXT("- **First Time Setup Complete**: %s\n"), CurrentPreferences.bHasCompletedFirstTimeSetup ? TEXT("Yes") : TEXT("No"));
    Result += FString::Printf(TEXT("- **Show Tutorials**: %s\n\n"), CurrentPreferences.bShowTutorials ? TEXT("Yes") : TEXT("No"));
    
    Result += TEXT("---\n");
    Result += TEXT("*This file is automatically generated. Manual edits may be overwritten.*\n");
    
    return Result;
}

bool UUserPreferencesManager::ParsePreferencesFromString(const FString& Content)
{
    // Simple parsing - look for key-value pairs
    TArray<FString> Lines;
    Content.ParseIntoArrayLines(Lines);
    
    FUserPreferences ParsedPrefs;
    bool bFoundValidData = false;
    
    for (const FString& Line : Lines)
    {
        FString TrimmedLine = Line.TrimStartAndEnd();
        
        // Skip empty lines and headers
        if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")) || TrimmedLine.StartsWith(TEXT("*"))) continue;
        
        // Look for markdown list items with values
        if (TrimmedLine.StartsWith(TEXT("- **")))
        {
            FString KeyValue = TrimmedLine.Mid(4); // Remove "- **"
            int32 ColonIndex;
            if (KeyValue.FindChar(':', ColonIndex))
            {
                FString Key = KeyValue.Left(ColonIndex - 2).TrimStartAndEnd(); // Remove "**:"
                FString Value = KeyValue.Mid(ColonIndex + 1).TrimStartAndEnd();
                
                bFoundValidData = true;
                
                // Parse specific values
                if (Key == TEXT("Preferred Language"))
                {
                    ParsedPrefs.PreferredLanguage = ParseLanguageFromString(Value);
                }
                else if (Key == TEXT("Preferred Login Method"))
                {
                    ParsedPrefs.PreferredLoginMethod = ParseLoginMethodFromString(Value);
                }
                else if (Key == TEXT("Master Volume"))
                {
                    ParsedPrefs.MasterVolume = FCString::Atof(*Value);
                }
                else if (Key == TEXT("Music Volume"))
                {
                    ParsedPrefs.MusicVolume = FCString::Atof(*Value);
                }
                else if (Key == TEXT("SFX Volume"))
                {
                    ParsedPrefs.SFXVolume = FCString::Atof(*Value);
                }
                else if (Key == TEXT("Voice Volume"))
                {
                    ParsedPrefs.VoiceVolume = FCString::Atof(*Value);
                }
                else if (Key == TEXT("Resolution"))
                {
                    FString Width, Height;
                    if (Value.Split(TEXT("x"), &Width, &Height))
                    {
                        ParsedPrefs.ResolutionWidth = FCString::Atoi(*Width);
                        ParsedPrefs.ResolutionHeight = FCString::Atoi(*Height);
                    }
                }
                else if (Key == TEXT("Fullscreen"))
                {
                    ParsedPrefs.bFullscreen = (Value == TEXT("Yes"));
                }
                else if (Key == TEXT("Graphics Quality"))
                {
                    ParsedPrefs.GraphicsQuality = FCString::Atoi(*Value);
                }
                else if (Key == TEXT("High Contrast Mode"))
                {
                    ParsedPrefs.bHighContrastMode = (Value == TEXT("Yes"));
                }
                else if (Key == TEXT("UI Scale"))
                {
                    ParsedPrefs.UIScale = FCString::Atof(*Value);
                }
                else if (Key == TEXT("Invert Mouse Y"))
                {
                    ParsedPrefs.bInvertMouseY = (Value == TEXT("Yes"));
                }
                else if (Key == TEXT("Mouse Sensitivity"))
                {
                    ParsedPrefs.MouseSensitivity = FCString::Atof(*Value);
                }
                else if (Key == TEXT("First Time Setup Complete"))
                {
                    ParsedPrefs.bHasCompletedFirstTimeSetup = (Value == TEXT("Yes"));
                }
                else if (Key == TEXT("Show Tutorials"))
                {
                    ParsedPrefs.bShowTutorials = (Value == TEXT("Yes"));
                }
            }
        }
    }
    
    if (bFoundValidData && ParsedPrefs.IsValid())
    {
        CurrentPreferences = ParsedPrefs;
        return true;
    }
    
    return false;
}

ELanguage UUserPreferencesManager::ParseLanguageFromString(const FString& LanguageString) const
{
    TArray<ELanguage> AllLanguages = ULanguageManager::GetAllLanguages();
    for (ELanguage Language : AllLanguages)
    {
        if (ULanguageManager::GetLanguageDisplayName(Language).ToString() == LanguageString)
        {
            return Language;
        }
    }
    return ELanguage::English; // Default fallback
}

ELoginMethod UUserPreferencesManager::ParseLoginMethodFromString(const FString& LoginMethodString) const
{
    TArray<ELoginMethod> AllLoginMethods = ULoginMethodManager::GetAllLoginMethods();
    for (ELoginMethod LoginMethod : AllLoginMethods)
    {
        if (ULoginMethodManager::GetLoginMethodDisplayName(LoginMethod).ToString() == LoginMethodString)
        {
            return LoginMethod;
        }
    }
    return ELoginMethod::AccountPortal; // Default fallback
}