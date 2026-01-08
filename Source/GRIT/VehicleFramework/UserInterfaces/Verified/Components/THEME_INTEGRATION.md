# Theme System Integration Guide

## Overview

Unified theme system with automatic styling for all UI widgets. Widgets fetch theme from `SessionAdapter` (GameInstance) without storing references.

## Architecture

```
SessionAdapter (GameInstance)
    └── FThemeConfig ActiveTheme
        ├── FPalette (Colors)
        ├── FTypeScale (Typography)
        ├── FSpaceGrid (Spacing)
        ├── FBorderSpec (Borders)
        └── FMotionTiming (Animation)

ThemeUtil (Static Helper)
    └── Fetches theme from SessionAdapter
    └── Applies styling to widgets

Widgets
    └── Call ThemeUtil in NativeConstruct
    └── Apply theme-based styling
```

## Core Components

### ThemeConfig.h
Single header with all theme systems:

**FPalette** - Semantic color naming
- Surface colors: `SurfacePrime`, `SurfaceShift`, `SurfaceRaised`, `SurfaceInset`
- Text colors: `TextPrime`, `TextShift`, `TextMute`, `TextOnAccent`
- Accent colors: `AccentCore`, `AccentSharp`, `AccentSoft`
- State colors: `StateHover`, `StateActive`, `StateDisable`
- Semantic colors: `SemanticCrit`, `SemanticWarn`, `SemanticPass`, `SemanticInfo`
- Line colors: `LinePrime`, `LineShift`, `LineFocus`

**FTypeScale** - Typography hierarchy
- Display: `DisplayXL`, `DisplayL`, `DisplayM`
- Heading: `HeadingXL`, `HeadingL`, `HeadingM`, `HeadingS`
- Body: `BodyL`, `BodyM`, `BodyS`
- Label: `LabelM`, `LabelS`
- Caption: `CaptionM`, `CaptionS`

**FSpaceGrid** - Spacing system
- `BaseUnit`: 8px foundation
- `Micro`: 2px (0.25x)
- `Tiny`: 4px (0.5x)
- `Small`: 8px (1x)
- `Medium`: 16px (2x)
- `Large`: 24px (3x)
- `XLarge`: 32px (4x)
- `Massive`: 48px (6x)

**FBorderSpec** - Border system
- Thickness: `ThicknessThin` (1px), `ThicknessBase` (2px), `ThicknessThick` (4px)
- Radius: `RadiusNone` (0), `RadiusTight` (4px), `RadiusSnug` (8px), `RadiusLoose` (12px), `RadiusRound` (16px), `RadiusFull` (999px)

**FMotionTiming** - Animation durations
- `Instant`: 0.1s
- `Swift`: 0.15s
- `Brisk`: 0.25s
- `Smooth`: 0.35s
- `Gentle`: 0.5s

### SessionAdapter.h/.cpp
Game instance managing theme:

```cpp
// Access theme
USessionAdapter* Session = Cast<USessionAdapter>(GetGameInstance());
FThemeConfig Theme = Session->GetTheme();

// Apply presets
Session->LoadDarkTheme();
Session->LoadLightTheme();
Session->LoadHighContrastTheme();

// Listen for changes
Session->OnThemeChanged.AddDynamic(this, &UMyWidget::HandleThemeChanged);
```

### ThemeUtil.h/.cpp
Static helper for widget styling:

```cpp
// Fetch theme components
FPalette Palette = UThemeUtil::FetchPalette(this);
FTypeScale Type = UThemeUtil::FetchTypeScale(this);
FBorderSpec Border = UThemeUtil::FetchBorderSpec(this);

// Apply styling
UThemeUtil::ApplyTypeSpec(MyTextBlock, Type.BodyM);
UThemeUtil::ApplyBorderStyling(MyBorder, Palette.SurfaceShift, Border.RadiusSnug);

// Blend colors for hover states
FLinearColor Hover = UThemeUtil::BlendOverlay(BaseColor, Palette.StateHover);
```

## Widget Integration

### ButtonEntryWidget
Interactive button with theme-based hover states:

**Styling Applied:**
- `NativeConstruct()`: Fetches theme, applies border/text styling
- `ApplyThemeStyling()`: Sets base colors, border radius, text style
- `ApplyHoverState()`: Blends `StateHover` overlay on mouse enter
- `RestoreDefaultState()`: Restores base color on mouse leave

**Visual States:**
- Default: `SurfaceShift` background
- Hover: `SurfaceShift` + `StateHover` blend
- Text: `BodyM` typography spec

### EntryWidget
Static display widget:

**Styling Applied:**
- `NativeConstruct()`: Fetches theme, applies text styling
- `ApplyThemeStyling()`: Sets text to `BodyM` spec

**Visual States:**
- Text: `BodyM` typography with theme colors

### SimpleList & EntryList
Container widgets (minimal styling):
- Entries handle their own styling
- List background can be styled in Blueprint if needed

## Usage Examples

### Creating Custom Themed Widget

```cpp
// YourWidget.h
UCLASS()
class UYourWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(meta = (BindWidget))
    UTextBlock* TitleText;

    UPROPERTY(meta = (BindWidget))
    UBorder* ContainerBorder;

protected:
    virtual void NativeConstruct() override;

private:
    void ApplyTheme();
};

// YourWidget.cpp
void UYourWidget::NativeConstruct()
{
    Super::NativeConstruct();
    ApplyTheme();
}

void UYourWidget::ApplyTheme()
{
    FPalette P = UThemeUtil::FetchPalette(this);
    FTypeScale T = UThemeUtil::FetchTypeScale(this);
    FBorderSpec B = UThemeUtil::FetchBorderSpec(this);

    UThemeUtil::ApplyTypeSpec(TitleText, T.HeadingL);
    UThemeUtil::ApplyBorderStyling(ContainerBorder, P.SurfacePrime, B.RadiusSnug);
}
```

### Responding to Theme Changes

```cpp
// Listen for theme changes in SessionAdapter
USessionAdapter* Session = Cast<USessionAdapter>(GetGameInstance());
Session->OnThemeChanged.AddDynamic(this, &UYourWidget::HandleThemeChanged);

void UYourWidget::HandleThemeChanged(const FThemeConfig& NewTheme)
{
    ApplyTheme(); // Reapply theme with new values
}
```

### Creating Custom Theme Preset

```cpp
void UYourGameMode::ApplyCustomTheme()
{
    USessionAdapter* Session = Cast<USessionAdapter>(GetGameInstance());
    FThemeConfig CustomTheme = Session->GetTheme();
    
    // Customize palette
    CustomTheme.Palette.AccentCore = FLinearColor(1.0f, 0.0f, 0.5f, 1.0f); // Pink
    CustomTheme.Palette.SurfacePrime = FLinearColor(0.1f, 0.0f, 0.1f, 1.0f); // Dark purple
    
    // Customize typography
    CustomTheme.Type.BodyM.Size = 16;
    
    // Apply
    Session->SetTheme(CustomTheme);
}
```

## Best Practices

1. **Never store theme data** - Always fetch from `UThemeUtil` when needed
2. **Apply in NativeConstruct** - Initialize theme on widget creation
3. **Use semantic colors** - `AccentCore` not "blue", `SemanticCrit` not "red"
4. **Follow spacing grid** - Use `FSpaceGrid` values for consistent layout
5. **Blend overlays for states** - Use `BlendOverlay()` for hover/active states
6. **Cache computed colors** - Store blended colors to avoid recalculation
7. **Listen to OnThemeChanged** - Update widgets when theme changes dynamically

## Theme Presets

### Dark Theme (Default)
- Surface: Near-black (#020202)
- Text: Off-white (#F2F2F2)
- Accent: Blue (#1A99FF)

### Light Theme
- Surface: Light grey (#F2F2F2)
- Text: Near-black (#1A1A1A)
- Accent: Dark blue (#0072D9)

### High Contrast
- Surface: Pure black (#000000)
- Text: Pure white (#FFFFFF)
- Accent: Yellow (#FFFF00)
- Semantic colors: Pure saturated

## Files Structure

```
ThemeConfig.h          - Unified theme structs (Palette/Type/Space/Border/Motion)
ThemeUtil.h/.cpp       - Static helper functions for styling
SessionAdapter.h/.cpp  - Game instance with theme management
ButtonEntryWidget       - Button with theme + hover states
EntryWidget             - Static entry with theme
SimpleList             - List container
EntryList               - List container
```

## Migration from Old System

Replace old structs:
- `FColorProfile` → `FPalette`
- `FFontProfile` → `FTypeScale`
- `FInterfaceStyle` → Removed (use FTypeScale directly)
- `FThemeConfiguration` → `FThemeConfig`

Update SessionAdapter:
- Replace `PlayerTheme` → `ActiveTheme`
- Use new getter/setter methods

Update widgets:
- Add `NativeConstruct()` override
- Call `UThemeUtil::FetchX()` instead of storing theme
- Use `ApplyTypeSpec()` for text styling
- Use `ApplyBorderStyling()` for border styling
