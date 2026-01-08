# EntryList System - Usage Guide

## Design Philosophy

**Single Universal Widget Class**
- ONE C++ class (`UEntryWidget`) handles all entry types
- Optional bindings adapt to your blueprint layout
- Create different blueprint variations from the same C++ class

## Components

### SimpleList
**Easy configuration via array in Details panel:**
- Set `Entry Configs` array with text and/or textures
- **Auto-populates automatically** (no Event Construct needed)
- Automatically selects correct entry type (Text/Image/LabeledImage)
- **NEW**: Button support with `Is Button` checkbox
  - Static entries: No interaction
  - Button entries: OnHovered, OnPressed, OnReleased, OnClicked events
  - Uses **UBorder** for maximum performance (~0.1ms overhead vs ~0.3ms for UButton)

**Blueprint Usage:**
1. Add SimpleList to your widget
2. Select SimpleList in hierarchy
3. Details panel → Entry Configs → Add elements
4. For each element:
   - Set `Entry Text` (optional)
   - Set `Entry Texture` (optional)
   - Set `Is Button` (checked = interactive, unchecked = static)
5. **Done!** List auto-populates at runtime

**For button entries**, see BUTTON_GUIDE.md for event binding.

**Manual API (optional):**
```cpp
USimpleList* List = Cast<USimpleList>(GetWidgetFromName(TEXT("MyList")));
List->PushItem(MyDataObject);
List->PopItem(MyDataObject);
List->FlushAll();
int32 Count = List->GetItemCount();
List->BuildFromConfigs(); // Manually rebuild from EntryConfigs array
```

### EntyList
Specialized list for manual C++ entry creation.

## Setup

### 1. Create Entry Widget Variations (Blueprints)

All inherit from the **same** C++ class `UEntryWidget`:

**Text-Only Widget (`WBP_TextEntry`):**
```
Canvas Panel
└── Text Block (Name: LabelText)  ← Required for text
```

**Image-Only Widget (`WBP_ImageEntry`):**
```
Canvas Panel
└── Image (Name: IconImage)  ← Required for image
```

**Combo Widget (`WBP_ComboEntry`):**
```
Canvas Panel
└── Horizontal Box
    ├── Image (Name: IconImage)
    └── Text Block (Name: LabelText)
```

**Blueprint Setup:**
1. Create Widget Blueprint
2. File → Reparent Blueprint → `EntryWidget`
3. Add `LabelText` and/or `IconImage` widgets as needed
4. Names must match exactly (bindings are optional but names must be correct)

### 2. List Setup
In your parent widget:
- Add a `ListView` component
- Set its class to `UEntryList` or `USimpleList`
- Set `Entry Widget Class` to your widget blueprint (any variation works)

## Usage Examples

### EntryList
```cpp
// Text-only entry
UTextEntry* TextEntry = NewObject<UTextEntry>();
TextEntry->SetupText(0, FText::FromString(TEXT("Text Item")));
EntryList->PushEntry(TextEntry);

// Image-only entry
UImageEntry* ImageEntry = NewObject<UImageEntry>();
ImageEntry->SetupImage(1, LoadedTexture);
EntryList->PushEntry(ImageEntry);

// Labeled image entry
ULabeledImageEntry* LabeledEntry = NewObject<ULabeledImageEntry>();
LabeledEntry->SetupLabeledImage(2, FText::FromString(TEXT("Item")), LoadedTexture);
EntryList->PushEntry(LabeledEntry);

// Clear all entries
EntryList->FlushAll();
```

## Architecture
- **EntryData.h**: Entry classes
  - Static: TextEntry, ImageEntry, LabeledImageEntry
  - Button: TextButtonEntry, ImageButtonEntry, LabeledImageButtonEntry
- **EntryWidget.h/.cpp**: Universal widget for static entries
- **ButtonEntryWidget.h/.cpp**: UBorder-based widget for button entries (high performance)
- **EntryList.h/.cpp**: List wrapper for manual entry creation
- **SimpleList.h/.cpp**: Auto-populating list with config array
- **FSimpleEntryConfig**: Config struct with `bIsButton` flag

## How It Works

**Static Entries:**
- `EntryWidget` displays content
- No interaction, lightweight

**Button Entries:**
- `ButtonEntryWidget` uses UBorder for input handling
- Overrides NativeOnMouseEnter/Leave/ButtonDown/Up
- Fires delegates on button entry data
- ~0.1ms overhead per entry (3x faster than UButton)

