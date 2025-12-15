#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ColourCodex.generated.h"


/** Categories for colours */
UENUM(BlueprintType)
enum class EColourCategory : uint8
{
    Gemstone,
    Metallic,
    Fruit,
    Celestial,
    Nature,
    Mythical,
    Vibrant,
    Pastel,
    Powder,
    Dark,
    Light,
    Elemental,
    Natural,
    Organic,
    Material,
    Abstract
};

/** Named colour keys to use as lookup keys in the codex */
UENUM(BlueprintType)
enum class EColourName : uint8
{
    // Whites
    Snow,
    Ivory,
    Pearl,
    Bone,
    Arctic,
    Coconut,
    Chalk,
    Vanilla,
    Cream,
    Linen,
    Talc,
    
    // Blacks
    Void,
    Asphalt,
    Charcoal,
    Midnight,
    Carbon,
    
    // Greys
    Ash,
    Graphite,
    Concrete,
    Zinc,
    
    // Reds
    Ruby,
    Cherry,
    Flame,
    Rust,
    Scarlet,
    Raspberry,
    Redwood,
    Bark,
    
    // Oranges
    Tangerine,
    Copper,
    Pumpkin,
    Sunset,
    Autumn,
    Peach,
    Apricot,
    Ginger,
    Sandstone,
    
    // Yellows
    Honey,
    Brass,
    Mustard,
    Saffron,
    Topaz,
    Primrose,
    Buttercream,
    Cheese,
    Butter,
    
    // Greens
    Emerald,
    Jade,
    Forest,
    Moss,
    Lime,
    Mint,
    Fern,
    Grass,
    Pine,
    Cactus,
    Eucalyptus,
    SoftSage,
    
    // Blues
    Sapphire,
    Ocean,
    Denim,
    Cobalt,
    Teal,
    Aquamarine,
    Glacier,
    RoyalBlue,
    Indigo,
    Frostbite,
    PowderBlue,
    
    // Purples
    Amethyst,
    Lavender,
    Plum,
    Grape,
    Blueberry,
    Violet,
    Mulberry,
    UltraViolet,
    
    // Pinks
    Rose,
    Blush,
    Coral,
    Bubblegum,
    Watermelon,
    Magenta,
    Flamingo,
    Petal,
    BabyPink,
    SoftCoral,
    PaleBlush,
    CottonCandy,
    
    // Browns
    Chocolate,
    Coffee,
    Caramel,
    Cinnamon,
    Hazel,
    Acorn,
    Gobi,
    
    // Metallics
    Silver,
    Platinum,
    Bronze,
    Chrome,
    Titanium,
    Oxidized,
    Nickel,
    
    // Special/Elemental
    Tundra,
    Olympus,
    Savanna,
    SoftSand,
    
    // Neutral Pastels (Cool & Unique)
    Porcelain,
    Moonstone,
    Opal,
    Bisque,
    Alabaster,
    Champagne,
    Seafoam,
    Wisteria,
    Lilac,
    Periwinkle
};

/**
 * @brief FColourDescriptor
 * Holds metadata for a named colour in the codex including main tint (TintA),
 * two derived tints (TintB at 50%, TintC at 25%), a human name, description,
 * category and an emoji (for quick visual reference in editor/UI).
 */
USTRUCT(BlueprintType)
struct GRIT_API FColourDescriptor
{
    GENERATED_BODY()

    // Friendly name (human readable)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colour Info")
    FString ColourName;

    // Short description of the colour
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colour Info")
    FString Description;

    // Category used for grouping/filtering in UI
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colour Info")
    EColourCategory Category;

    // Primary tint (the actual colour)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colour Tints")
    FLinearColor TintA;

    // 50% version of TintA
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colour Tints")
    FLinearColor TintB;

    // 25% version of TintA
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colour Tints")
    FLinearColor TintC;

    // An optional emoji / icon string (e.g., 🟠 🔵 ⚪) for quick visual reference
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colour Info")
    FString Emoji;

    FColourDescriptor()
    {
        ColourName = TEXT("");
        Description = TEXT("");
        Category = EColourCategory::Vibrant;
        TintA = FLinearColor::White;
        TintB = FLinearColor::White;
        TintC = FLinearColor::White;
        Emoji = TEXT("");
    }

    FColourDescriptor(const FString& InName, const FString& InDescription, EColourCategory InCategory, const FLinearColor& InColor, const FString& InEmoji = TEXT(""))
    {
        ColourName = InName;
        Description = InDescription;
        Category = InCategory;
        TintA = InColor;
        TintB = FLinearColor(InColor.R * 0.5f, InColor.G * 0.5f, InColor.B * 0.5f, InColor.A);
        TintC = FLinearColor(InColor.R * 0.25f, InColor.G * 0.25f, InColor.B * 0.25f, InColor.A);
        Emoji = InEmoji;
    }
};

/**
 * @brief UColourCodex
 * Singleton UObject that holds the global colour codex mapping EColourName -> FColourDescriptor.
 * Initialize once via InitializeColourCodex() (internal Get() will ensure it's initialized).
 */
UCLASS(BlueprintType)
class GRIT_API UColourCodex : public UObject
{
    GENERATED_BODY()

public:
    UColourCodex();

    /** Get global singleton instance (creates and initializes if necessary) */
    UFUNCTION(BlueprintCallable, Category = "ColourCodex")
    static UColourCodex* Get();

    /** Initializes the codex. Safe to call multiple times (clears and rebuilds). */
    UFUNCTION(BlueprintCallable, Category = "ColourCodex")
    void InitializeColourCodex();

    /** Returns the main tint (TintA) for the given colour; returns white if not found. */
    UFUNCTION(BlueprintCallable, Category = "ColourCodex")
    FLinearColor GetColor(EColourName Colour) const;

    /** Returns the descriptor pointer for the colour or nullptr if missing. */
    const FColourDescriptor* GetDescriptor(EColourName Colour) const;

    /** Returns descriptor by value; if not found returns a default descriptor. */
    UFUNCTION(BlueprintCallable, Category = "ColourCodex")
    FColourDescriptor GetDescriptorChecked(EColourName Colour) const;

    /** Returns TintA/B/C by index (1..3). If not found returns white. */
    UFUNCTION(BlueprintCallable, Category = "ColourCodex")
    FLinearColor GetTint(EColourName Colour, int32 TintIndex = 1) const;

    /** Adds or replaces an entry (runtime helper) */
    void AddOrUpdateColour(EColourName Colour, const FColourDescriptor& Descriptor);

    /** Remove an entry */
    bool RemoveColour(EColourName Colour);

    /** Returns a const reference to the internal TMap (useful for iteration) */
    const TMap<EColourName, FColourDescriptor>& GetAllColours() const { return ColourCodex; }

protected:
    /** Internal map */
    UPROPERTY(VisibleAnywhere, Category = "ColourCodex")
    TMap<EColourName, FColourDescriptor> ColourCodex;

private:
    /** internal singleton pointer */
    static UColourCodex* SingletonInstance;
};