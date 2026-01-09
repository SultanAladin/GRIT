# SimpleList Quick Start

## Setup in 3 Steps

### Step 1: Add SimpleList to Widget
- Open your widget blueprint
- Add a **List View** component
- In Details panel, set **Class** to `SimpleList`
- Set **Entry Widget Class** to your `WBP_EntryWidget`

### Step 2: Configure Entries Array
- Select the SimpleList in hierarchy
- In Details panel, find **Entry Configs** array
- Click **+** to add entries

**For each entry:**
```
Entry Configs [0]
├── Entry Text: "My Item Name"     ← Optional - leave empty for image-only
└── Entry Texture: [Your Texture]  ← Optional - leave null for text-only
```

### Step 3: Done!
- List **auto-populates** at runtime
- **No Event Construct needed**
- Uses `SynchronizeProperties()` in C++

## Entry Type Selection (Automatic)

SimpleList automatically creates the correct entry type:

| Has Text | Has Texture | Creates          |
|----------|-------------|------------------|
| ✓        | ✓           | LabeledImageEntry|
| ✓        | ✗           | TextEntry        |
| ✗        | ✓           | ImageEntry       |
| ✗        | ✗           | (skipped)        |

## Example Configuration

```
Entry Configs
├── [0] Entry Text: "Sword"        Entry Texture: sword_icon.png
├── [1] Entry Text: "Shield"       Entry Texture: shield_icon.png
├── [2] Entry Text: "Text Only"    Entry Texture: None
└── [3] Entry Text: ""             Entry Texture: logo.png
```

Result:
- Entry 0: Labeled image (sword icon + "Sword" text)
- Entry 1: Labeled image (shield icon + "Shield" text)
- Entry 2: Text only ("Text Only")
- Entry 3: Image only (logo)

## Runtime Updates

```cpp
// C++ - Add new entry at runtime
FSimpleEntryConfig NewConfig;
NewConfig.EntryText = FText::FromString(TEXT("New Item"));
NewConfig.EntryTexture = LoadedTexture;
MySimpleList->EntryConfigs.Add(NewConfig);
MySimpleList->BuildFromConfigs();

// C++ - Clear and rebuild
MySimpleList->FlushAll();
MySimpleList->BuildFromConfigs();
```

## vs EntryList

| Feature         | SimpleList              | EntryList           |
|-----------------|-------------------------|--------------------|
| Configuration   | Array in Details        | Manual C++ code    |
| Use Case        | Designer-friendly       | Dynamic/procedural |
| Auto-populate   | ✓ Automatic (C++)       | Manual PushEntry   |
| Runtime editing | Via array + rebuild     | Direct PushEntry   |
