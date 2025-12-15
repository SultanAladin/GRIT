// SessionAdapter.cpp - Game Instance implementation

#include "SessionAdapter.h"
#include "ColourCodex/Public/ColourCodex.h"

USessionAdapter::USessionAdapter()
{
    // Initialize with default theme
    InitializeDefaultTheme();
}

void USessionAdapter::Init()
{
    Super::Init();

    // Ensure ColourCodex is initialized
    UColourCodex::Get();

    UE_LOG(LogTemp, Log, TEXT("SessionAdapter initialized with default theme"));
}

void USessionAdapter::Shutdown()
{
    UE_LOG(LogTemp, Log, TEXT("SessionAdapter shutting down"));
    Super::Shutdown();
}

void USessionAdapter::SetThemeConfiguration(const FThemeConfiguration& NewTheme)
{
    PlayerTheme = NewTheme;
    OnThemeChanged.Broadcast(PlayerTheme);
}

void USessionAdapter::SetColorProfile(const FColorProfile& NewColorProfile)
{
    PlayerTheme.ColorProfile = NewColorProfile;
    OnThemeChanged.Broadcast(PlayerTheme);
}

void USessionAdapter::SetFontProfile(const FFontProfile& NewFontProfile)
{
    PlayerTheme.FontProfile = NewFontProfile;
    OnThemeChanged.Broadcast(PlayerTheme);
}

void USessionAdapter::ResetThemeToDefaults()
{
    InitializeDefaultTheme();
    OnThemeChanged.Broadcast(PlayerTheme);
}

void USessionAdapter::InitializeDefaultTheme()
{
    // Default dark theme
    PlayerTheme.ColorProfile.AccentColor = FLinearColor(0.1f, 0.6f, 1.0f, 1.0f);       // Blue accent
    PlayerTheme.ColorProfile.BackgroundColor = FLinearColor(0.02f, 0.02f, 0.02f, 1.0f); // Near black
    PlayerTheme.ColorProfile.SecondaryBackgroundColor = FLinearColor(0.08f, 0.08f, 0.08f, 1.0f);
    PlayerTheme.ColorProfile.TextColor = FLinearColor(0.95f, 0.95f, 0.95f, 1.0f);       // Off-white
    PlayerTheme.ColorProfile.SecondaryTextColor = FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);
    PlayerTheme.ColorProfile.HighlightColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);     // Orange
    PlayerTheme.ColorProfile.AlertColor = FLinearColor(1.0f, 0.2f, 0.2f, 1.0f);         // Red
    PlayerTheme.ColorProfile.DisabledColor = FLinearColor(0.3f, 0.3f, 0.3f, 0.5f);
    PlayerTheme.ColorProfile.SuccessColor = FLinearColor(0.2f, 0.8f, 0.2f, 1.0f);       // Green
    PlayerTheme.ColorProfile.InfoColor = FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);          // Light blue

    // Default font settings
    PlayerTheme.FontProfile.PrimaryFont = nullptr;  // Will use engine default
    PlayerTheme.FontProfile.SecondaryFont = nullptr;
    PlayerTheme.FontProfile.PrimaryFontSize = 14;
    PlayerTheme.FontProfile.SecondaryFontSize = 10;
    PlayerTheme.FontProfile.bUseBoldForHeaders = true;
    PlayerTheme.FontProfile.bUseItalicForEmphasis = false;
    PlayerTheme.FontProfile.LineHeight = 1.2f;
    PlayerTheme.FontProfile.LetterSpacing = 0.0f;
    PlayerTheme.FontProfile.HeadingFontColor = FLinearColor::White;
    PlayerTheme.FontProfile.BodyFontColor = FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
    PlayerTheme.FontProfile.LinkFontColor = FLinearColor(0.1f, 0.6f, 1.0f, 1.0f);
}

void USessionAdapter::ApplyDarkTheme()
{
    InitializeDefaultTheme(); // Default is already dark
    OnThemeChanged.Broadcast(PlayerTheme);
}

void USessionAdapter::ApplyLightTheme()
{
    PlayerTheme.ColorProfile.AccentColor = FLinearColor(0.0f, 0.4f, 0.8f, 1.0f);        // Darker blue
    PlayerTheme.ColorProfile.BackgroundColor = FLinearColor(0.95f, 0.95f, 0.95f, 1.0f); // Light grey
    PlayerTheme.ColorProfile.SecondaryBackgroundColor = FLinearColor(0.88f, 0.88f, 0.88f, 1.0f);
    PlayerTheme.ColorProfile.TextColor = FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);          // Near black
    PlayerTheme.ColorProfile.SecondaryTextColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);
    PlayerTheme.ColorProfile.HighlightColor = FLinearColor(0.9f, 0.4f, 0.0f, 1.0f);     // Orange
    PlayerTheme.ColorProfile.AlertColor = FLinearColor(0.8f, 0.1f, 0.1f, 1.0f);         // Red
    PlayerTheme.ColorProfile.DisabledColor = FLinearColor(0.6f, 0.6f, 0.6f, 0.5f);
    PlayerTheme.ColorProfile.SuccessColor = FLinearColor(0.1f, 0.6f, 0.1f, 1.0f);       // Green
    PlayerTheme.ColorProfile.InfoColor = FLinearColor(0.0f, 0.4f, 0.8f, 1.0f);          // Blue

    PlayerTheme.FontProfile.HeadingFontColor = FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
    PlayerTheme.FontProfile.BodyFontColor = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f);
    PlayerTheme.FontProfile.LinkFontColor = FLinearColor(0.0f, 0.4f, 0.8f, 1.0f);

    OnThemeChanged.Broadcast(PlayerTheme);
}

void USessionAdapter::ApplyHighContrastTheme()
{
    PlayerTheme.ColorProfile.AccentColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);        // Yellow
    PlayerTheme.ColorProfile.BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);    // Pure black
    PlayerTheme.ColorProfile.SecondaryBackgroundColor = FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
    PlayerTheme.ColorProfile.TextColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);          // Pure white
    PlayerTheme.ColorProfile.SecondaryTextColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
    PlayerTheme.ColorProfile.HighlightColor = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f);     // Cyan
    PlayerTheme.ColorProfile.AlertColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);         // Pure red
    PlayerTheme.ColorProfile.DisabledColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
    PlayerTheme.ColorProfile.SuccessColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);       // Pure green
    PlayerTheme.ColorProfile.InfoColor = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f);          // Cyan

    PlayerTheme.FontProfile.PrimaryFontSize = 16;  // Larger for accessibility
    PlayerTheme.FontProfile.SecondaryFontSize = 12;
    PlayerTheme.FontProfile.HeadingFontColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);    // Yellow
    PlayerTheme.FontProfile.BodyFontColor = FLinearColor::White;
    PlayerTheme.FontProfile.LinkFontColor = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f);       // Cyan

    OnThemeChanged.Broadcast(PlayerTheme);
}
