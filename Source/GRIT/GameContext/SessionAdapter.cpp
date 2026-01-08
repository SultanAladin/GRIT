// SessionAdapter.cpp - Game Instance implementation
#include "SessionAdapter.h"
#include "UserPreferences.h"

USessionAdapter::USessionAdapter()
{
    BootstrapDefaultTheme();
    PreferencesMgr = CreateDefaultSubobject<UUserPreferencesManager>(TEXT("PreferencesMgr"));
}

void USessionAdapter::Init()
{
    Super::Init();

    if (PreferencesMgr) // Reason: Initialize preferences manager
    {
        PreferencesMgr->Initialize();
        
        bool bLoaded = PreferencesMgr->LoadUserPreferences();
        if (bLoaded) // Reason: Apply loaded preferences
        {
            UE_LOG(LogTemp, Log, TEXT("SessionAdapter: Preferences loaded"));
            PreferencesMgr->ApplyPreferencesToGlobalManagers();
        } // End if (Loaded check)
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("SessionAdapter: Using default preferences"));
        }
    } // End if (PreferencesMgr check)

    UE_LOG(LogTemp, Log, TEXT("SessionAdapter: Initialized with default theme"));
}

void USessionAdapter::Shutdown()
{
    UE_LOG(LogTemp, Log, TEXT("SessionAdapter: Shutting down"));
    Super::Shutdown();
}

void USessionAdapter::SetTheme(const FThemeConfig& NewTheme)
{
    ActiveTheme = NewTheme;
    OnThemeChanged.Broadcast(ActiveTheme);
}

void USessionAdapter::SetPalette(const FPalette& NewPalette)
{
    ActiveTheme.Palette = NewPalette;
    OnThemeChanged.Broadcast(ActiveTheme);
}

void USessionAdapter::SetTypeScale(const FTypeScale& NewTypeScale)
{
    ActiveTheme.Type = NewTypeScale;
    OnThemeChanged.Broadcast(ActiveTheme);
}

void USessionAdapter::SetSpaceGrid(const FSpaceGrid& NewSpaceGrid)
{
    ActiveTheme.Space = NewSpaceGrid;
    OnThemeChanged.Broadcast(ActiveTheme);
}

void USessionAdapter::SetBorderSpec(const FBorderSpec& NewBorderSpec)
{
    ActiveTheme.Border = NewBorderSpec;
    OnThemeChanged.Broadcast(ActiveTheme);
}

void USessionAdapter::SetMotionTiming(const FMotionTiming& NewMotionTiming)
{
    ActiveTheme.Motion = NewMotionTiming;
    OnThemeChanged.Broadcast(ActiveTheme);
}

void USessionAdapter::ResetTheme()
{
    BootstrapDefaultTheme();
    OnThemeChanged.Broadcast(ActiveTheme);
}

//------------------------------------------------------------------------------
// theme initialization
//------------------------------------------------------------------------------

void USessionAdapter::BootstrapDefaultTheme()
{
    FPalette& P = ActiveTheme.Palette;
    
    P.SurfacePrime = FLinearColor(0.02f, 0.02f, 0.02f, 1.0f);
    P.SurfaceShift = FLinearColor(0.08f, 0.08f, 0.08f, 1.0f);
    P.SurfaceRaised = FLinearColor(0.12f, 0.12f, 0.12f, 1.0f);
    P.SurfaceInset = FLinearColor(0.01f, 0.01f, 0.01f, 1.0f);
    
    P.TextPrime = FLinearColor(0.95f, 0.95f, 0.95f, 1.0f);
    P.TextShift = FLinearColor(0.70f, 0.70f, 0.70f, 1.0f);
    P.TextMute = FLinearColor(0.45f, 0.45f, 0.45f, 1.0f);
    P.TextOnAccent = FLinearColor(0.05f, 0.05f, 0.05f, 1.0f);
    
    P.AccentCore = FLinearColor(0.10f, 0.60f, 1.00f, 1.0f);
    P.AccentSharp = FLinearColor(0.20f, 0.70f, 1.00f, 1.0f);
    P.AccentSoft = FLinearColor(0.05f, 0.50f, 0.90f, 1.0f);
    
    P.StateHover = FLinearColor(1.00f, 1.00f, 1.00f, 0.08f);
    P.StateActive = FLinearColor(1.00f, 1.00f, 1.00f, 0.12f);
    P.StateDisable = FLinearColor(0.30f, 0.30f, 0.30f, 0.50f);
    
    P.SemanticCrit = FLinearColor(1.00f, 0.20f, 0.20f, 1.0f);
    P.SemanticWarn = FLinearColor(1.00f, 0.65f, 0.00f, 1.0f);
    P.SemanticPass = FLinearColor(0.20f, 0.80f, 0.20f, 1.0f);
    P.SemanticInfo = FLinearColor(0.20f, 0.60f, 1.00f, 1.0f);
    
    P.LinePrime = FLinearColor(0.20f, 0.20f, 0.20f, 1.0f);
    P.LineShift = FLinearColor(0.15f, 0.15f, 0.15f, 1.0f);
    P.LineFocus = FLinearColor(0.10f, 0.60f, 1.00f, 1.0f);
}

//------------------------------------------------------------------------------
// theme presets
//------------------------------------------------------------------------------

void USessionAdapter::LoadDarkTheme()
{
    BootstrapDefaultTheme();
    OnThemeChanged.Broadcast(ActiveTheme);
}

void USessionAdapter::LoadLightTheme()
{
    FPalette& P = ActiveTheme.Palette;
    
    P.SurfacePrime = FLinearColor(0.95f, 0.95f, 0.95f, 1.0f);
    P.SurfaceShift = FLinearColor(0.88f, 0.88f, 0.88f, 1.0f);
    P.SurfaceRaised = FLinearColor(1.00f, 1.00f, 1.00f, 1.0f);
    P.SurfaceInset = FLinearColor(0.92f, 0.92f, 0.92f, 1.0f);
    
    P.TextPrime = FLinearColor(0.10f, 0.10f, 0.10f, 1.0f);
    P.TextShift = FLinearColor(0.35f, 0.35f, 0.35f, 1.0f);
    P.TextMute = FLinearColor(0.55f, 0.55f, 0.55f, 1.0f);
    P.TextOnAccent = FLinearColor(1.00f, 1.00f, 1.00f, 1.0f);
    
    P.AccentCore = FLinearColor(0.00f, 0.45f, 0.85f, 1.0f);
    P.AccentSharp = FLinearColor(0.10f, 0.55f, 0.95f, 1.0f);
    P.AccentSoft = FLinearColor(0.00f, 0.35f, 0.75f, 1.0f);
    
    P.StateHover = FLinearColor(0.00f, 0.00f, 0.00f, 0.05f);
    P.StateActive = FLinearColor(0.00f, 0.00f, 0.00f, 0.10f);
    P.StateDisable = FLinearColor(0.60f, 0.60f, 0.60f, 0.50f);
    
    P.SemanticCrit = FLinearColor(0.85f, 0.10f, 0.10f, 1.0f);
    P.SemanticWarn = FLinearColor(0.90f, 0.55f, 0.00f, 1.0f);
    P.SemanticPass = FLinearColor(0.10f, 0.70f, 0.10f, 1.0f);
    P.SemanticInfo = FLinearColor(0.00f, 0.45f, 0.85f, 1.0f);
    
    P.LinePrime = FLinearColor(0.75f, 0.75f, 0.75f, 1.0f);
    P.LineShift = FLinearColor(0.85f, 0.85f, 0.85f, 1.0f);
    P.LineFocus = FLinearColor(0.00f, 0.45f, 0.85f, 1.0f);
    
    OnThemeChanged.Broadcast(ActiveTheme);
}

void USessionAdapter::LoadHighContrastTheme()
{
    FPalette& P = ActiveTheme.Palette;
    
    P.SurfacePrime = FLinearColor(0.00f, 0.00f, 0.00f, 1.0f);
    P.SurfaceShift = FLinearColor(0.10f, 0.10f, 0.10f, 1.0f);
    P.SurfaceRaised = FLinearColor(0.15f, 0.15f, 0.15f, 1.0f);
    P.SurfaceInset = FLinearColor(0.00f, 0.00f, 0.00f, 1.0f);
    
    P.TextPrime = FLinearColor(1.00f, 1.00f, 1.00f, 1.0f);
    P.TextShift = FLinearColor(1.00f, 1.00f, 0.00f, 1.0f);
    P.TextMute = FLinearColor(0.85f, 0.85f, 0.85f, 1.0f);
    P.TextOnAccent = FLinearColor(0.00f, 0.00f, 0.00f, 1.0f);
    
    P.AccentCore = FLinearColor(1.00f, 1.00f, 0.00f, 1.0f);
    P.AccentSharp = FLinearColor(1.00f, 1.00f, 0.50f, 1.0f);
    P.AccentSoft = FLinearColor(0.85f, 0.85f, 0.00f, 1.0f);
    
    P.StateHover = FLinearColor(1.00f, 1.00f, 1.00f, 0.15f);
    P.StateActive = FLinearColor(1.00f, 1.00f, 1.00f, 0.25f);
    P.StateDisable = FLinearColor(0.50f, 0.50f, 0.50f, 1.0f);
    
    P.SemanticCrit = FLinearColor(1.00f, 0.00f, 0.00f, 1.0f);
    P.SemanticWarn = FLinearColor(1.00f, 1.00f, 0.00f, 1.0f);
    P.SemanticPass = FLinearColor(0.00f, 1.00f, 0.00f, 1.0f);
    P.SemanticInfo = FLinearColor(0.00f, 1.00f, 1.00f, 1.0f);
    
    P.LinePrime = FLinearColor(1.00f, 1.00f, 1.00f, 1.0f);
    P.LineShift = FLinearColor(0.75f, 0.75f, 0.75f, 1.0f);
    P.LineFocus = FLinearColor(1.00f, 1.00f, 0.00f, 1.0f);
    
    ActiveTheme.Type.BodyM.Size = 16;
    ActiveTheme.Type.BodyL.Size = 18;
    
    OnThemeChanged.Broadcast(ActiveTheme);
}

//------------------------------------------------------------------------------
// user preferences
//------------------------------------------------------------------------------

FUserPreferences USessionAdapter::GetUserPreferences() const
{
    if (PreferencesMgr) // Reason: Valid manager check
    {
        return PreferencesMgr->GetUserPreferences();
    } // End if (PreferencesMgr check)
    
    return FUserPreferences();
}

void USessionAdapter::SetUserPreferences(const FUserPreferences& NewPreferences, bool bSaveImmediately)
{
    if (PreferencesMgr) // Reason: Valid manager check
    {
        PreferencesMgr->SetUserPreferences(NewPreferences);
        
        if (bSaveImmediately) // Reason: Immediate save requested
        {
            PreferencesMgr->SaveUserPreferences();
        } // End if (Save check)
        
        UE_LOG(LogTemp, Log, TEXT("SessionAdapter: Preferences updated"));
    } // End if (PreferencesMgr check)
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SessionAdapter: PreferencesMgr is null"));
    }
}

bool USessionAdapter::LoadPreferences()
{
    if (PreferencesMgr) // Reason: Valid manager check
    {
        return PreferencesMgr->LoadUserPreferences();
    } // End if (PreferencesMgr check)
    
    return false;
}

bool USessionAdapter::SavePreferences()
{
    if (PreferencesMgr) // Reason: Valid manager check
    {
        return PreferencesMgr->SaveUserPreferences();
    } // End if (PreferencesMgr check)
    
    return false;
}
