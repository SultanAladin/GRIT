// SessionAdapter.h - Game Instance with unified theme configuration
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "UserInterface/Components/ThemeConfig.h"
#include "UserPreferences.h"
#include "SessionAdapter.generated.h"

/*====================================================================================================================================
                                                         SESSION ADAPTER
======================================================================================================================================*/

/** Game instance managing session state and UI theme */
UCLASS(BlueprintType, Blueprintable)
class GRIT_API USessionAdapter : public UGameInstance
{
    GENERATED_BODY()

public:
    USessionAdapter();

    //------------------------------------------------------------------------------
    // lifecycle
    //------------------------------------------------------------------------------
    
    virtual void Init() override;
    virtual void Shutdown() override;

    //------------------------------------------------------------------------------
    // theme access
    //------------------------------------------------------------------------------
    
    /** Retrieve active theme configuration */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    FThemeConfig GetTheme() const { return ActiveTheme; }

    /** Apply new theme configuration */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    void SetTheme(const FThemeConfig& NewTheme);

    /** Retrieve color palette */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    FPalette GetPalette() const { return ActiveTheme.Palette; }

    /** Apply color palette */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    void SetPalette(const FPalette& NewPalette);

    /** Retrieve typography scale */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    FTypeScale GetTypeScale() const { return ActiveTheme.Type; }

    /** Apply typography scale */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    void SetTypeScale(const FTypeScale& NewTypeScale);

    /** Retrieve spacing grid */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    FSpaceGrid GetSpaceGrid() const { return ActiveTheme.Space; }

    /** Apply spacing grid */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    void SetSpaceGrid(const FSpaceGrid& NewSpaceGrid);

    /** Retrieve border specification */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    FBorderSpec GetBorderSpec() const { return ActiveTheme.Border; }

    /** Apply border specification */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    void SetBorderSpec(const FBorderSpec& NewBorderSpec);

    /** Retrieve motion timing */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    FMotionTiming GetMotionTiming() const { return ActiveTheme.Motion; }

    /** Apply motion timing */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    void SetMotionTiming(const FMotionTiming& NewMotionTiming);

    /** Reset theme to defaults */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    void ResetTheme();

    //------------------------------------------------------------------------------
    // theme presets
    //------------------------------------------------------------------------------
    
    /** Apply dark theme preset */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    void LoadDarkTheme();

    /** Apply light theme preset */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    void LoadLightTheme();

    /** Apply high contrast theme preset */
    UFUNCTION(BlueprintCallable, Category = "Session|Theme")
    void LoadHighContrastTheme();

    //------------------------------------------------------------------------------
    // user preferences
    //------------------------------------------------------------------------------
    
    /** Retrieve user preferences manager */
    UFUNCTION(BlueprintCallable, Category = "Session|Preferences")
    UUserPreferencesManager* GetUserPreferencesManager() const { return PreferencesMgr; }

    /** Retrieve user preferences */
    UFUNCTION(BlueprintCallable, Category = "Session|Preferences")
    FUserPreferences GetUserPreferences() const;

    /** Apply user preferences */
    UFUNCTION(BlueprintCallable, Category = "Session|Preferences")
    void SetUserPreferences(const FUserPreferences& NewPreferences, bool bSaveImmediately = true);

    /** Load preferences from disk */
    UFUNCTION(BlueprintCallable, Category = "Session|Preferences")
    bool LoadPreferences();

    /** Save preferences to disk */
    UFUNCTION(BlueprintCallable, Category = "Session|Preferences")
    bool SavePreferences();

    //------------------------------------------------------------------------------
    // events
    //------------------------------------------------------------------------------
    
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThemeChanged, const FThemeConfig&, NewTheme);

    UPROPERTY(BlueprintAssignable, Category = "Session|Events")
    FOnThemeChanged OnThemeChanged;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session|Theme")
    FThemeConfig ActiveTheme;           // [FThemeConfig] - Active theme data

    UPROPERTY()
    UUserPreferencesManager* PreferencesMgr; // [UUserPreferencesManager*] - Preferences manager

    /** Bootstrap default theme */
    void BootstrapDefaultTheme();
};
