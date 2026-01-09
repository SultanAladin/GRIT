# UI Framework Updates - Implementation Guide

## Files Created

### Animation System
- **AnimUtil.h** - Animation curve utilities
- **AnimUtil.cpp** - Curve computation implementation

### Dropdown Widget
- **DropList.h** - Animated dropdown using ListView
- **DropList.cpp** - Implementation with timer-based animation (FIXED: Corrected FBorderSpec radius usage)

### Fixes
- **ButtonEntyWidget_Fixed.cpp** - Fixed button color initialization

---

## Issue 1: Button Color Fix

### Problem
Button showed 3 different colors:
1. Blueprint default color (initial)
2. Theme color after `ApplyThemeStyling()`
3. Correct color after first hover/unhover

### Root Cause
`ApplyBorderStyling()` set `Brush.TintColor`, then `SetBrushColor()` modified it later. These conflicted.

### Solution
In `ApplyThemeStyling()`:
```cpp
FSlateBrush Brush;
Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
Brush.TintColor = FSlateColor(FLinearColor::White);  // Neutral base
Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
Brush.OutlineSettings.CornerRadii = FVector4(Border.RadiusSnug, Border.RadiusSnug, Border.RadiusSnug, Border.RadiusSnug);
if (Border.ThicknessBase > 0.0f) { Brush.OutlineSettings.Width = Border.ThicknessBase; Brush.OutlineSettings.Color = FSlateColor(BaseColor); }
ButtonBorder->SetBrush(Brush);
ButtonBorder->SetBrushColor(BaseColor);  // Consistent color application
```

Now all color changes use `SetBrushColor()` consistently.

---

## Issue 2: New Dropdown System

### FBorderSpec Available Radius Options
```cpp
RadiusNone   = 0.0f    // No rounding
RadiusTight  = 4.0f    // Minimal rounding
RadiusSnug   = 8.0f    // Standard rounding
RadiusLoose  = 12.0f   // Generous rounding
RadiusRound  = 16.0f   // Highly rounded
RadiusFull   = 999.0f  // Circular
```

**Note:** There is NO `RadiusMedium` - use `RadiusSnug` or `RadiusLoose` instead.

### Animation Utilities (`AnimUtil.h/cpp`)

**Curve Types:**
```cpp
enum class EMotionCurve : uint8
{
    Linear,      // Constant velocity
    QuadIn,      // Accelerating start
    QuadOut,     // Decelerating end  
    QuadInOut,   // Smooth ease in/out
    CubicIn,     // Sharp acceleration
    CubicOut,    // Sharp deceleration
    CubicInOut,  // Dramatic ease
    ExpIn,       // Explosive end
    ExpOut,      // Explosive start
    Snap         // Instant
};
```

**Core Functions:**
```cpp
// Compute curve adjustment [0,1] → [0,1]
float ComputeCurve(float Progress, EMotionCurve Curve);

// Interpolate with curve
float LerpCurved(float Start, float Target, float Progress, EMotionCurve Curve);
FLinearColor LerpColorCurved(FLinearColor Start, FLinearColor Target, float Progress, EMotionCurve Curve);
FVector2D LerpVector2DCurved(FVector2D Start, FVector2D Target, float Progress, EMotionCurve Curve);
```

### Dropdown Widget (`DropList.h/cpp`)

**Features:**
- Header bar acts as button (Border-based input)
- ListView for content (`UEntyList`)
- Timer-based animation (no Tick)
- Chevron rotation
- Theme integration

**Widget Hierarchy:**
```
UDropList
├── HeaderBorder (clickable)
│   ├── HeaderLabel
│   └── ChevronIcon
└── ContentOverlay
    └── ContentSizeBox (animated)
        └── ContentBorder
            └── ContentList (UEntyList)
```

**Blueprint Setup:**
1. Create widget blueprint based on `UDropList`
2. Bind required widgets:
   - `HeaderBorder` (Border)
   - `HeaderLabel` (TextBlock)
   - `ChevronIcon` (Image)
   - `ContentOverlay` (Overlay)
   - `ContentSizeBox` (SizeBox)
   - `ContentBorder` (Border)
   - `ContentList` (EntyList)

**Configuration Properties:**
```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropList")
FText HeaderText;  // Header display text

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropList|Animation")
float AnimDuration;  // [s] - Animation time

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropList|Animation")
EMotionCurve AnimCurve;  // Curve type

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropList|Animation")
bool bAnimChevron;  // Rotate chevron

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropList|Appearance")
float MaxContentHeight;  // [px] - Max height
```

**Usage Example:**
```cpp
// Toggle dropdown
DropList->Toggle();

// Set state explicitly
DropList->SetExpanded(true);

// Add entries
UTextEntry* Entry = NewObject<UTextEntry>(this);
Entry->BootstrapText(0, FText::FromString("Option 1"));
DropList->AddEntry(Entry);

// Clear entries
DropList->ClearEntries();

// Check state
bool bOpen = DropList->IsExpanded();
int32 Count = DropList->GetEntryCount();
```

**Animation System:**
- Uses `GetWorld()->GetTimerManager().SetTimer()`
- Update rate: 16ms (~60fps)
- Reads time from timer, not Tick
- Applies curve using `UAnimUtil::LerpCurved()`
- Animates `ContentSizeBox->SetHeightOverride()`

**Timer Setup:**
```cpp
GetWorld()->GetTimerManager().SetTimer(
    AnimTimer,           // Timer handle
    this,               // Object
    &UDropList::TickAnim,  // Function
    0.016f,             // [s] - 16ms interval
    true                // Loop
);
```

**Animation Tick:**
```cpp
void UDropList::TickAnim()
{
    AnimTime += 0.016f;  // [s] - Frame time
    float Progress = FMath::Clamp(AnimTime / AnimDuration, 0.0f, 1.0f);
    
    float CurrentHeight = UAnimUtil::LerpCurved(StartHeight, TargetHeight, Progress, AnimCurve);
    ContentSizeBox->SetHeightOverride(CurrentHeight);
    
    if (Progress >= 1.0f)  // Animation complete
    {
        bIsAnimating = false;
        GetWorld()->GetTimerManager().ClearTimer(AnimTimer);
    }
}
```

---

## Key Differences from Old System

### Old Dropdown (`GenericDropdown`)
- Manual item creation with configs
- Overlay Z-ordering issues
- Complex item widget management

### New Dropdown (`DropList`)
- Uses ListView (`UEntyList`) - cleaner, more flexible
- Proper widget hierarchy with Overlay
- Border-based header (simpler than Button)
- Timer-based animation (no Tick overhead)
- Chevron rotation integrated
- Better theme integration

---

## Next Steps

1. **Replace ButtonEntyWidget.cpp** with fixed version
2. **Add AnimUtil** files to project
3. **Create DropList blueprint**:
   - Set widget bindings
   - Configure animation (0.3s, QuadOut recommended)
   - Set max height
4. **Test with entries**:
   - Use existing entry types (TextEntry, ImageEntry, etc.)
   - Button entries work with DropList
5. **Customize appearance**:
   - Adjust theme colors
   - Modify corner radii
   - Change animation curves

---

## Animation Curve Recommendations

- **QuadOut** - Smooth, professional (default)
- **CubicOut** - Snappy, responsive
- **ExpOut** - Dynamic, attention-grabbing
- **Linear** - Mechanical, constant speed
- **Snap** - Instant (testing only)

Avoid `ExpIn` / `CubicIn` for dropdowns - they feel sluggish.
