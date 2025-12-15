// SessionAdapter.h - Game Instance with player theme configuration
// Manages per-session player state including UI theme preferences

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ColourCodex/Public/ThemeConfiguration.h"
#include "SessionAdapter.generated.h"

/**
 * @brief USessionAdapter
 * Game Instance that persists across level transitions.
 * Holds the player's copy of the ThemeConfiguration for UI customization.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIT_API USessionAdapter : public UGameInstance
{
    GENERATED_BODY()

public:
    USessionAdapter();

    //--------------------------------------------------------------------------
    // LIFECYCLE
    //--------------------------------------------------------------------------

    /** Called when the game instance is created */
    virtual void Init() override;

    /** Called when the game instance is shutting down */
    virtual void Shutdown() override;

    //--------------------------------------------------------------------------
    // THEME CONFIGURATION
    //--------------------------------------------------------------------------

    /** Get the player's current theme configuration */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    FThemeConfiguration GetThemeConfiguration() const { return PlayerTheme; }

    /** Set the player's theme configuration */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    void SetThemeConfiguration(const FThemeConfiguration& NewTheme);

    /** Get the color profile from the current theme */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    FColorProfile GetColorProfile() const { return PlayerTheme.ColorProfile; }

    /** Set just the color profile */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    void SetColorProfile(const FColorProfile& NewColorProfile);

    /** Get the font profile from the current theme */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    FFontProfile GetFontProfile() const { return PlayerTheme.FontProfile; }

    /** Set just the font profile */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    void SetFontProfile(const FFontProfile& NewFontProfile);

    /** Reset theme to defaults */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    void ResetThemeToDefaults();

    //--------------------------------------------------------------------------
    // THEME PRESETS
    //--------------------------------------------------------------------------

    /** Apply a dark theme preset */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme|Presets")
    void ApplyDarkTheme();

    /** Apply a light theme preset */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme|Presets")
    void ApplyLightTheme();

    /** Apply a high contrast theme preset (accessibility) */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme|Presets")
    void ApplyHighContrastTheme();

    //--------------------------------------------------------------------------
    // EVENTS
    //--------------------------------------------------------------------------

    /** Broadcast when theme configuration changes */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThemeChanged, const FThemeConfiguration&, NewTheme);

    UPROPERTY(BlueprintAssignable, Category = "Session|Events")
    FOnThemeChanged OnThemeChanged;

protected:
    /** Player's theme configuration - persists across levels */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session|Theme")
    FThemeConfiguration PlayerTheme;

    /** Initialize default theme settings */
    void InitializeDefaultTheme();
};
