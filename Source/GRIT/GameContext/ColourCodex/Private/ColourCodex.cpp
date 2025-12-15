#include "ColourCodex.h"

UColourCodex* UColourCodex::SingletonInstance = nullptr;

UColourCodex::UColourCodex()
{
	// Basic ctor
}

UColourCodex* UColourCodex::Get()
{
	if (!SingletonInstance)
	{
		SingletonInstance = NewObject<UColourCodex>();
		SingletonInstance->AddToRoot();
		SingletonInstance->InitializeColourCodex();
	}
	return SingletonInstance;
}

void UColourCodex::InitializeColourCodex()
{
	ColourCodex.Empty();

	//------------------------------------------------------------------------------
	// Whites (11)
	//------------------------------------------------------------------------------
	ColourCodex.Add(EColourName::Snow, FColourDescriptor(TEXT("Snow"), TEXT("Pure white"), EColourCategory::Natural, FLinearColor(1.0f, 1.0f, 1.0f), TEXT("❄️")));
	ColourCodex.Add(EColourName::Ivory, FColourDescriptor(TEXT("Ivory"), TEXT("Off-white with cream"), EColourCategory::Natural, FLinearColor(1.0f, 1.0f, 0.94f), TEXT("🦷")));
	ColourCodex.Add(EColourName::Pearl, FColourDescriptor(TEXT("Pearl"), TEXT("Lustrous white with shimmer"), EColourCategory::Gemstone, FLinearColor(0.94f, 0.92f, 0.84f), TEXT("🤍")));
	ColourCodex.Add(EColourName::Bone, FColourDescriptor(TEXT("Bone"), TEXT("Pale yellowish white"), EColourCategory::Natural, FLinearColor(0.9f, 0.89f, 0.82f), TEXT("🦴")));
	ColourCodex.Add(EColourName::Arctic, FColourDescriptor(TEXT("Arctic"), TEXT("Icy blue-white"), EColourCategory::Natural, FLinearColor(0.7f, 0.9f, 1.0f), TEXT("❄️")));
	ColourCodex.Add(EColourName::Coconut, FColourDescriptor(TEXT("Coconut"), TEXT("Creamy coconut"), EColourCategory::Fruit, FLinearColor(0.96f, 0.96f, 0.86f), TEXT("🥥")));
	ColourCodex.Add(EColourName::Chalk, FColourDescriptor(TEXT("Chalk"), TEXT("Soft powder white"), EColourCategory::Powder, FLinearColor(0.96f, 0.96f, 0.96f), TEXT("⚪")));
	ColourCodex.Add(EColourName::Vanilla, FColourDescriptor(TEXT("Vanilla"), TEXT("Warm cream white"), EColourCategory::Pastel, FLinearColor(0.95f, 0.93f, 0.84f), TEXT("🍦")));
	ColourCodex.Add(EColourName::Cream, FColourDescriptor(TEXT("Cream"), TEXT("Rich off-white"), EColourCategory::Pastel, FLinearColor(1.0f, 0.99f, 0.82f), TEXT("🥛")));
	ColourCodex.Add(EColourName::Linen, FColourDescriptor(TEXT("Linen"), TEXT("Natural fabric white"), EColourCategory::Pastel, FLinearColor(0.98f, 0.94f, 0.9f), TEXT("🤍")));
	ColourCodex.Add(EColourName::Talc, FColourDescriptor(TEXT("Talc"), TEXT("Very pale grey-white"), EColourCategory::Powder, FLinearColor(0.93f, 0.93f, 0.93f), TEXT("⚪")));

	//------------------------------------------------------------------------------
	// Blacks (5)
	//------------------------------------------------------------------------------
	ColourCodex.Add(EColourName::Void, FColourDescriptor(TEXT("Void"), TEXT("Absolute black"), EColourCategory::Abstract, FLinearColor(0.0f, 0.0f, 0.0f), TEXT("⚫")));
	ColourCodex.Add(EColourName::Asphalt, FColourDescriptor(TEXT("Asphalt"), TEXT("Dark road surface"), EColourCategory::Material, FLinearColor(0.12f, 0.12f, 0.12f), TEXT("⬛")));
	ColourCodex.Add(EColourName::Charcoal, FColourDescriptor(TEXT("Charcoal"), TEXT("Dark grey-black"), EColourCategory::Material, FLinearColor(0.21f, 0.27f, 0.31f), TEXT("🖤")));
	ColourCodex.Add(EColourName::Midnight, FColourDescriptor(TEXT("Midnight"), TEXT("Deep navy black"), EColourCategory::Celestial, FLinearColor(0.1f, 0.1f, 0.44f), TEXT("🌌")));
	ColourCodex.Add(EColourName::Carbon, FColourDescriptor(TEXT("Carbon"), TEXT("Deep matte black"), EColourCategory::Material, FLinearColor(0.13f, 0.13f, 0.13f), TEXT("⬛")));

	//------------------------------------------------------------------------------
	// Greys (4)
	//------------------------------------------------------------------------------
	ColourCodex.Add(EColourName::Ash, FColourDescriptor(TEXT("Ash"), TEXT("Light grey"), EColourCategory::Natural, FLinearColor(0.7f, 0.72f, 0.73f), TEXT("🌫️")));
	ColourCodex.Add(EColourName::Graphite, FColourDescriptor(TEXT("Graphite"), TEXT("Dark pencil grey"), EColourCategory::Material, FLinearColor(0.25f, 0.25f, 0.28f), TEXT("✏️")));
	ColourCodex.Add(EColourName::Concrete, FColourDescriptor(TEXT("Concrete"), TEXT("Urban grey"), EColourCategory::Material, FLinearColor(0.54f, 0.54f, 0.54f), TEXT("🏢")));
	ColourCodex.Add(EColourName::Zinc, FColourDescriptor(TEXT("Zinc"), TEXT("Cool metallic grey"), EColourCategory::Metallic, FLinearColor(0.66f, 0.67f, 0.68f), TEXT("⚙️")));

	//------------------------------------------------------------------------------
	// Reds (8)
	//------------------------------------------------------------------------------
	ColourCodex.Add(EColourName::Ruby, FColourDescriptor(TEXT("Ruby"), TEXT("Deep gemstone red"), EColourCategory::Gemstone, FLinearColor(0.7f, 0.05f, 0.15f), TEXT("💎")));
	ColourCodex.Add(EColourName::Cherry, FColourDescriptor(TEXT("Cherry"), TEXT("Bright fruit red"), EColourCategory::Fruit, FLinearColor(0.87f, 0.0f, 0.13f), TEXT("🍒")));
	ColourCodex.Add(EColourName::Flame, FColourDescriptor(TEXT("Flame"), TEXT("Bright orange-red fire"), EColourCategory::Elemental, FLinearColor(1.0f, 0.27f, 0.0f), TEXT("🔥")));
	ColourCodex.Add(EColourName::Rust, FColourDescriptor(TEXT("Rust"), TEXT("Orange-brown oxidized metal"), EColourCategory::Metallic, FLinearColor(0.72f, 0.26f, 0.05f), TEXT("🟤")));
	ColourCodex.Add(EColourName::Scarlet, FColourDescriptor(TEXT("Scarlet"), TEXT("Vivid red"), EColourCategory::Vibrant, FLinearColor(1.0f, 0.14f, 0.0f), TEXT("🔴")));
	ColourCodex.Add(EColourName::Raspberry, FColourDescriptor(TEXT("Raspberry"), TEXT("Berry red-pink"), EColourCategory::Fruit, FLinearColor(0.89f, 0.04f, 0.36f), TEXT("🫐")));
	ColourCodex.Add(EColourName::Redwood, FColourDescriptor(TEXT("Redwood"), TEXT("Rich reddish-brown"), EColourCategory::Natural, FLinearColor(0.64f, 0.35f, 0.32f), TEXT("🌲")));
	ColourCodex.Add(EColourName::Bark, FColourDescriptor(TEXT("Bark"), TEXT("Tree bark brown-red"), EColourCategory::Natural, FLinearColor(0.45f, 0.29f, 0.22f), TEXT("🪵")));

	//------------------------------------------------------------------------------
	// Oranges (9)
	//------------------------------------------------------------------------------
	ColourCodex.Add(EColourName::Tangerine, FColourDescriptor(TEXT("Tangerine"), TEXT("Vibrant citrus orange"), EColourCategory::Fruit, FLinearColor(0.95f, 0.52f, 0.0f), TEXT("🍊")));
	ColourCodex.Add(EColourName::Copper, FColourDescriptor(TEXT("Copper"), TEXT("Warm metallic orange"), EColourCategory::Metallic, FLinearColor(0.72f, 0.45f, 0.2f), TEXT("🟠")));
	ColourCodex.Add(EColourName::Pumpkin, FColourDescriptor(TEXT("Pumpkin"), TEXT("Rich harvest orange"), EColourCategory::Fruit, FLinearColor(1.0f, 0.46f, 0.09f), TEXT("🎃")));
	ColourCodex.Add(EColourName::Sunset, FColourDescriptor(TEXT("Sunset"), TEXT("Warm pink-orange glow"), EColourCategory::Celestial, FLinearColor(0.99f, 0.49f, 0.31f), TEXT("🌇")));
	ColourCodex.Add(EColourName::Autumn, FColourDescriptor(TEXT("Autumn"), TEXT("Burnt orange-brown"), EColourCategory::Natural, FLinearColor(0.8f, 0.52f, 0.25f), TEXT("🍂")));
	ColourCodex.Add(EColourName::Peach, FColourDescriptor(TEXT("Peach"), TEXT("Soft pastel orange"), EColourCategory::Fruit, FLinearColor(1.0f, 0.8f, 0.64f), TEXT("🍑")));
	ColourCodex.Add(EColourName::Apricot, FColourDescriptor(TEXT("Apricot"), TEXT("Soft orange fruit"), EColourCategory::Fruit, FLinearColor(0.98f, 0.81f, 0.69f), TEXT("🍊")));
	ColourCodex.Add(EColourName::Ginger, FColourDescriptor(TEXT("Ginger"), TEXT("Warm orange-brown spice"), EColourCategory::Natural, FLinearColor(0.69f, 0.4f, 0.0f), TEXT("🫚")));
	ColourCodex.Add(EColourName::Sandstone, FColourDescriptor(TEXT("Sandstone"), TEXT("Warm desert beige"), EColourCategory::Natural, FLinearColor(0.79f, 0.62f, 0.48f), TEXT("🏜️")));

	//------------------------------------------------------------------------------
	// Yellows (9)
	//------------------------------------------------------------------------------
	ColourCodex.Add(EColourName::Honey, FColourDescriptor(TEXT("Honey"), TEXT("Rich amber yellow"), EColourCategory::Natural, FLinearColor(1.0f, 0.73f, 0.06f), TEXT("🍯")));
	ColourCodex.Add(EColourName::Brass, FColourDescriptor(TEXT("Brass"), TEXT("Dull metallic yellow"), EColourCategory::Metallic, FLinearColor(0.85f, 0.73f, 0.25f), TEXT("🔶")));
	ColourCodex.Add(EColourName::Mustard, FColourDescriptor(TEXT("Mustard"), TEXT("Deep golden yellow-brown"), EColourCategory::Natural, FLinearColor(0.88f, 0.76f, 0.18f), TEXT("🟡")));
	ColourCodex.Add(EColourName::Saffron, FColourDescriptor(TEXT("Saffron"), TEXT("Golden yellow spice"), EColourCategory::Natural, FLinearColor(0.96f, 0.77f, 0.19f), TEXT("🌼")));
	ColourCodex.Add(EColourName::Topaz, FColourDescriptor(TEXT("Topaz"), TEXT("Golden amber gemstone"), EColourCategory::Gemstone, FLinearColor(1.0f, 0.78f, 0.49f), TEXT("💎")));
	ColourCodex.Add(EColourName::Primrose, FColourDescriptor(TEXT("Primrose"), TEXT("Pale yellow flower"), EColourCategory::Pastel, FLinearColor(0.96f, 0.95f, 0.71f), TEXT("🌼")));
	ColourCodex.Add(EColourName::Buttercream, FColourDescriptor(TEXT("Buttercream"), TEXT("Pale creamy yellow"), EColourCategory::Pastel, FLinearColor(1.0f, 0.99f, 0.82f), TEXT("🧈")));
	ColourCodex.Add(EColourName::Cheese, FColourDescriptor(TEXT("Cheese"), TEXT("Rich creamy yellow"), EColourCategory::Natural, FLinearColor(1.0f, 0.91f, 0.51f), TEXT("🧀")));
	ColourCodex.Add(EColourName::Butter, FColourDescriptor(TEXT("Butter"), TEXT("Soft golden yellow"), EColourCategory::Natural, FLinearColor(1.0f, 0.89f, 0.51f), TEXT("🧈")));

	//------------------------------------------------------------------------------
	// Greens (12)
	//------------------------------------------------------------------------------
	ColourCodex.Add(EColourName::Emerald, FColourDescriptor(TEXT("Emerald"), TEXT("Brilliant green gemstone"), EColourCategory::Gemstone, FLinearColor(0.0f, 0.8f, 0.2f), TEXT("💎")));
	ColourCodex.Add(EColourName::Jade, FColourDescriptor(TEXT("Jade"), TEXT("Deep green stone"), EColourCategory::Gemstone, FLinearColor(0.0f, 0.66f, 0.42f), TEXT("💚")));
	ColourCodex.Add(EColourName::Forest, FColourDescriptor(TEXT("Forest"), TEXT("Deep woodland green"), EColourCategory::Natural, FLinearColor(0.13f, 0.55f, 0.13f), TEXT("🌲")));
	ColourCodex.Add(EColourName::Moss, FColourDescriptor(TEXT("Moss"), TEXT("Soft muted green"), EColourCategory::Organic, FLinearColor(0.45f, 0.55f, 0.34f), TEXT("🌿")));
	ColourCodex.Add(EColourName::Lime, FColourDescriptor(TEXT("Lime"), TEXT("Bright yellow-green"), EColourCategory::Fruit, FLinearColor(0.75f, 1.0f, 0.0f), TEXT("🍋‍🟩")));
	ColourCodex.Add(EColourName::Mint, FColourDescriptor(TEXT("Mint"), TEXT("Light fresh green"), EColourCategory::Organic, FLinearColor(0.62f, 0.99f, 0.6f), TEXT("🌿")));
	ColourCodex.Add(EColourName::Fern, FColourDescriptor(TEXT("Fern"), TEXT("Medium forest green"), EColourCategory::Organic, FLinearColor(0.31f, 0.68f, 0.31f), TEXT("🌿")));
	ColourCodex.Add(EColourName::Grass, FColourDescriptor(TEXT("Grass"), TEXT("Fresh spring green"), EColourCategory::Organic, FLinearColor(0.48f, 0.99f, 0.0f), TEXT("🌾")));
	ColourCodex.Add(EColourName::Pine, FColourDescriptor(TEXT("Pine"), TEXT("Deep evergreen"), EColourCategory::Natural, FLinearColor(0.0f, 0.47f, 0.44f), TEXT("🌲")));
	ColourCodex.Add(EColourName::Cactus, FColourDescriptor(TEXT("Cactus"), TEXT("Muted desert green"), EColourCategory::Organic, FLinearColor(0.53f, 0.66f, 0.42f), TEXT("🌵")));
	ColourCodex.Add(EColourName::Eucalyptus, FColourDescriptor(TEXT("Eucalyptus"), TEXT("Soft grey-green"), EColourCategory::Pastel, FLinearColor(0.69f, 0.87f, 0.78f), TEXT("🌿")));
	ColourCodex.Add(EColourName::SoftSage, FColourDescriptor(TEXT("Soft Sage"), TEXT("Muted pastel green"), EColourCategory::Pastel, FLinearColor(0.76f, 0.84f, 0.75f), TEXT("🌿")));

	//------------------------------------------------------------------------------
	// Blues (11)
	//------------------------------------------------------------------------------
	ColourCodex.Add(EColourName::Sapphire, FColourDescriptor(TEXT("Sapphire"), TEXT("Deep blue gemstone"), EColourCategory::Gemstone, FLinearColor(0.06f, 0.32f, 0.73f), TEXT("💙")));
	ColourCodex.Add(EColourName::Ocean, FColourDescriptor(TEXT("Ocean"), TEXT("Deep sea blue"), EColourCategory::Natural, FLinearColor(0.0f, 0.41f, 0.58f), TEXT("🌊")));
	ColourCodex.Add(EColourName::Denim, FColourDescriptor(TEXT("Denim"), TEXT("Medium blue fabric"), EColourCategory::Material, FLinearColor(0.26f, 0.43f, 0.71f), TEXT("👖")));
	ColourCodex.Add(EColourName::Cobalt, FColourDescriptor(TEXT("Cobalt"), TEXT("Vivid deep blue"), EColourCategory::Gemstone, FLinearColor(0.0f, 0.28f, 0.67f), TEXT("🔵")));
	ColourCodex.Add(EColourName::Teal, FColourDescriptor(TEXT("Teal"), TEXT("Blue-green balance"), EColourCategory::Natural, FLinearColor(0.0f, 0.5f, 0.5f), TEXT("🦆")));
	ColourCodex.Add(EColourName::Aquamarine, FColourDescriptor(TEXT("Aquamarine"), TEXT("Light blue-green gem"), EColourCategory::Gemstone, FLinearColor(0.25f, 0.88f, 0.82f), TEXT("💎")));
	ColourCodex.Add(EColourName::Glacier, FColourDescriptor(TEXT("Glacier"), TEXT("Ice glacier blue"), EColourCategory::Natural, FLinearColor(0.68f, 0.85f, 0.9f), TEXT("🧊")));
	ColourCodex.Add(EColourName::RoyalBlue, FColourDescriptor(TEXT("Royal Blue"), TEXT("Vibrant regal blue"), EColourCategory::Gemstone, FLinearColor(0.25f, 0.41f, 0.88f), TEXT("👑")));
	ColourCodex.Add(EColourName::Indigo, FColourDescriptor(TEXT("Indigo"), TEXT("Deep blue-purple"), EColourCategory::Vibrant, FLinearColor(0.29f, 0.0f, 0.51f), TEXT("🔵")));
	ColourCodex.Add(EColourName::Frostbite, FColourDescriptor(TEXT("Frostbite"), TEXT("Icy blue-white"), EColourCategory::Natural, FLinearColor(0.88f, 0.95f, 1.0f), TEXT("❄️")));
	ColourCodex.Add(EColourName::PowderBlue, FColourDescriptor(TEXT("Powder Blue"), TEXT("Soft pastel blue"), EColourCategory::Pastel, FLinearColor(0.69f, 0.88f, 0.9f), TEXT("💙")));

	//------------------------------------------------------------------------------
	// Purples (8)
	//------------------------------------------------------------------------------
	ColourCodex.Add(EColourName::Amethyst, FColourDescriptor(TEXT("Amethyst"), TEXT("Purple gemstone"), EColourCategory::Gemstone, FLinearColor(0.6f, 0.4f, 0.8f), TEXT("💜")));
	ColourCodex.Add(EColourName::Lavender, FColourDescriptor(TEXT("Lavender"), TEXT("Light purple flower"), EColourCategory::Organic, FLinearColor(0.71f, 0.49f, 0.86f), TEXT("💐")));
	ColourCodex.Add(EColourName::Plum, FColourDescriptor(TEXT("Plum"), TEXT("Dark reddish purple"), EColourCategory::Fruit, FLinearColor(0.56f, 0.27f, 0.52f), TEXT("🍑")));
	ColourCodex.Add(EColourName::Grape, FColourDescriptor(TEXT("Grape"), TEXT("Deep purple fruit"), EColourCategory::Fruit, FLinearColor(0.44f, 0.18f, 0.66f), TEXT("🍇")));
	ColourCodex.Add(EColourName::Blueberry, FColourDescriptor(TEXT("Blueberry"), TEXT("Rich blueberry"), EColourCategory::Fruit, FLinearColor(0.31f, 0.42f, 0.69f), TEXT("🫐")));
	ColourCodex.Add(EColourName::Violet, FColourDescriptor(TEXT("Violet"), TEXT("Vibrant purple"), EColourCategory::Vibrant, FLinearColor(0.58f, 0.0f, 0.83f), TEXT("💜")));
	ColourCodex.Add(EColourName::Mulberry, FColourDescriptor(TEXT("Mulberry"), TEXT("Dark purple-red berry"), EColourCategory::Fruit, FLinearColor(0.77f, 0.29f, 0.55f), TEXT("🫐")));
	ColourCodex.Add(EColourName::UltraViolet, FColourDescriptor(TEXT("Ultra Violet"), TEXT("Vibrant electric purple"), EColourCategory::Vibrant, FLinearColor(0.39f, 0.36f, 0.89f), TEXT("💜")));

	//------------------------------------------------------------------------------
	// Pinks (12)
	//------------------------------------------------------------------------------
	ColourCodex.Add(EColourName::Rose, FColourDescriptor(TEXT("Rose"), TEXT("Soft flower pink"), EColourCategory::Organic, FLinearColor(1.0f, 0.0f, 0.5f), TEXT("🌹")));
	ColourCodex.Add(EColourName::Blush, FColourDescriptor(TEXT("Blush"), TEXT("Light peachy pink"), EColourCategory::Natural, FLinearColor(1.0f, 0.71f, 0.76f), TEXT("😊")));
	ColourCodex.Add(EColourName::Coral, FColourDescriptor(TEXT("Coral"), TEXT("Orange-pink sea life"), EColourCategory::Natural, FLinearColor(1.0f, 0.5f, 0.31f), TEXT("🪸")));
	ColourCodex.Add(EColourName::Bubblegum, FColourDescriptor(TEXT("Bubblegum"), TEXT("Bright candy pink"), EColourCategory::Abstract, FLinearColor(1.0f, 0.76f, 0.8f), TEXT("💗")));
	ColourCodex.Add(EColourName::Watermelon, FColourDescriptor(TEXT("Watermelon"), TEXT("Bright fruit pink"), EColourCategory::Fruit, FLinearColor(0.99f, 0.36f, 0.47f), TEXT("🍉")));
	ColourCodex.Add(EColourName::Magenta, FColourDescriptor(TEXT("Magenta"), TEXT("Bright pink-purple"), EColourCategory::Vibrant, FLinearColor(1.0f, 0.0f, 1.0f), TEXT("💗")));
	ColourCodex.Add(EColourName::Flamingo, FColourDescriptor(TEXT("Flamingo"), TEXT("Bright tropical pink"), EColourCategory::Organic, FLinearColor(0.99f, 0.56f, 0.67f), TEXT("🦩")));
	ColourCodex.Add(EColourName::Petal, FColourDescriptor(TEXT("Petal"), TEXT("Delicate soft pink"), EColourCategory::Pastel, FLinearColor(1.0f, 0.91f, 0.95f), TEXT("🌸")));
	ColourCodex.Add(EColourName::BabyPink, FColourDescriptor(TEXT("Baby Pink"), TEXT("Light pastel pink"), EColourCategory::Pastel, FLinearColor(0.96f, 0.76f, 0.76f), TEXT("💗")));
	ColourCodex.Add(EColourName::SoftCoral, FColourDescriptor(TEXT("Soft Coral"), TEXT("Pale coral pink"), EColourCategory::Pastel, FLinearColor(0.97f, 0.69f, 0.64f), TEXT("🪸")));
	ColourCodex.Add(EColourName::PaleBlush, FColourDescriptor(TEXT("Pale Blush"), TEXT("Very light pink"), EColourCategory::Pastel, FLinearColor(0.98f, 0.85f, 0.87f), TEXT("💕")));
	ColourCodex.Add(EColourName::CottonCandy, FColourDescriptor(TEXT("Cotton Candy"), TEXT("Sweet pastel pink"), EColourCategory::Pastel, FLinearColor(1.0f, 0.74f, 0.85f), TEXT("🍬")));

	//------------------------------------------------------------------------------
	// Browns (7)
	//------------------------------------------------------------------------------
	ColourCodex.Add(EColourName::Chocolate, FColourDescriptor(TEXT("Chocolate"), TEXT("Rich dark brown"), EColourCategory::Natural, FLinearColor(0.48f, 0.25f, 0.0f), TEXT("🍫")));
	ColourCodex.Add(EColourName::Coffee, FColourDescriptor(TEXT("Coffee"), TEXT("Deep brown roast"), EColourCategory::Natural, FLinearColor(0.44f, 0.31f, 0.22f), TEXT("☕")));
	ColourCodex.Add(EColourName::Caramel, FColourDescriptor(TEXT("Caramel"), TEXT("Sweet golden brown"), EColourCategory::Natural, FLinearColor(0.76f, 0.6f, 0.42f), TEXT("🍬")));
	ColourCodex.Add(EColourName::Cinnamon, FColourDescriptor(TEXT("Cinnamon"), TEXT("Warm spice brown"), EColourCategory::Natural, FLinearColor(0.51f, 0.29f, 0.11f), TEXT("🍂")));
	ColourCodex.Add(EColourName::Hazel, FColourDescriptor(TEXT("Hazel"), TEXT("Light golden brown"), EColourCategory::Natural, FLinearColor(0.55f, 0.42f, 0.25f), TEXT("🌰")));
	ColourCodex.Add(EColourName::Acorn, FColourDescriptor(TEXT("Acorn"), TEXT("Rich nut brown"), EColourCategory::Natural, FLinearColor(0.53f, 0.35f, 0.16f), TEXT("🌰")));
	ColourCodex.Add(EColourName::Gobi, FColourDescriptor(TEXT("Gobi"), TEXT("Desert tan brown"), EColourCategory::Natural, FLinearColor(0.78f, 0.65f, 0.44f), TEXT("🏜️")));

	//------------------------------------------------------------------------------
	// Metallics (7)
	//------------------------------------------------------------------------------
	ColourCodex.Add(EColourName::Silver, FColourDescriptor(TEXT("Silver"), TEXT("Bright metallic grey"), EColourCategory::Metallic, FLinearColor(0.75f, 0.75f, 0.75f), TEXT("🥈")));
	ColourCodex.Add(EColourName::Platinum, FColourDescriptor(TEXT("Platinum"), TEXT("Lustrous white metal"), EColourCategory::Metallic, FLinearColor(0.9f, 0.89f, 0.89f), TEXT("⚪")));
	ColourCodex.Add(EColourName::Bronze, FColourDescriptor(TEXT("Bronze"), TEXT("Aged copper alloy"), EColourCategory::Metallic, FLinearColor(0.8f, 0.5f, 0.2f), TEXT("🥉")));
	ColourCodex.Add(EColourName::Chrome, FColourDescriptor(TEXT("Chrome"), TEXT("Reflective silver"), EColourCategory::Metallic, FLinearColor(0.8f, 0.8f, 0.8f), TEXT("✨")));
	ColourCodex.Add(EColourName::Titanium, FColourDescriptor(TEXT("Titanium"), TEXT("Dark metallic grey"), EColourCategory::Metallic, FLinearColor(0.54f, 0.53f, 0.51f), TEXT("⚔️")));
	ColourCodex.Add(EColourName::Oxidized, FColourDescriptor(TEXT("Oxidized"), TEXT("Weathered steel"), EColourCategory::Metallic, FLinearColor(0.35f, 0.35f, 0.35f), TEXT("🔩")));
	ColourCodex.Add(EColourName::Nickel, FColourDescriptor(TEXT("Nickel"), TEXT("Light metallic grey"), EColourCategory::Metallic, FLinearColor(0.56f, 0.56f, 0.56f), TEXT("⚙️")));

	//------------------------------------------------------------------------------
	// Special/Elemental (4)
	//------------------------------------------------------------------------------
	ColourCodex.Add(EColourName::Tundra, FColourDescriptor(TEXT("Tundra"), TEXT("Icy tundra white"), EColourCategory::Natural, FLinearColor(0.9f, 0.95f, 1.0f), TEXT("❄️")));
	ColourCodex.Add(EColourName::Olympus, FColourDescriptor(TEXT("Olympus"), TEXT("Divine peak white"), EColourCategory::Mythical, FLinearColor(0.98f, 0.98f, 1.0f), TEXT("⛰️")));
	ColourCodex.Add(EColourName::Savanna, FColourDescriptor(TEXT("Savanna"), TEXT("Dry grassland yellow"), EColourCategory::Natural, FLinearColor(0.89f, 0.76f, 0.49f), TEXT("🦁")));
	ColourCodex.Add(EColourName::SoftSand, FColourDescriptor(TEXT("Soft Sand"), TEXT("Pale beach sand"), EColourCategory::Pastel, FLinearColor(0.96f, 0.93f, 0.84f), TEXT("🏖️")));

	//------------------------------------------------------------------------------
	// Neutral Pastels (Cool & Unique) (10)
	//------------------------------------------------------------------------------
	ColourCodex.Add(EColourName::Porcelain, FColourDescriptor(TEXT("Porcelain"), TEXT("Delicate ceramic white"), EColourCategory::Pastel, FLinearColor(0.93f, 0.95f, 0.96f), TEXT("🏺")));
	ColourCodex.Add(EColourName::Moonstone, FColourDescriptor(TEXT("Moonstone"), TEXT("Ethereal blue-white glow"), EColourCategory::Pastel, FLinearColor(0.91f, 0.94f, 0.98f), TEXT("🌙")));
	ColourCodex.Add(EColourName::Opal, FColourDescriptor(TEXT("Opal"), TEXT("Iridescent pale shimmer"), EColourCategory::Pastel, FLinearColor(0.95f, 0.97f, 0.99f), TEXT("💎")));
	ColourCodex.Add(EColourName::Bisque, FColourDescriptor(TEXT("Bisque"), TEXT("Warm peachy cream"), EColourCategory::Pastel, FLinearColor(1.0f, 0.89f, 0.77f), TEXT("🥐")));
	ColourCodex.Add(EColourName::Alabaster, FColourDescriptor(TEXT("Alabaster"), TEXT("Smooth marble white"), EColourCategory::Pastel, FLinearColor(0.98f, 0.98f, 0.95f), TEXT("🗿")));
	ColourCodex.Add(EColourName::Champagne, FColourDescriptor(TEXT("Champagne"), TEXT("Elegant golden cream"), EColourCategory::Pastel, FLinearColor(0.97f, 0.91f, 0.81f), TEXT("🥂")));
	ColourCodex.Add(EColourName::Seafoam, FColourDescriptor(TEXT("Seafoam"), TEXT("Soft aqua green"), EColourCategory::Pastel, FLinearColor(0.82f, 0.95f, 0.91f), TEXT("🌊")));
	ColourCodex.Add(EColourName::Wisteria, FColourDescriptor(TEXT("Wisteria"), TEXT("Gentle purple-grey"), EColourCategory::Pastel, FLinearColor(0.79f, 0.75f, 0.86f), TEXT("💜")));
	ColourCodex.Add(EColourName::Lilac, FColourDescriptor(TEXT("Lilac"), TEXT("Soft violet flower"), EColourCategory::Pastel, FLinearColor(0.78f, 0.64f, 0.78f), TEXT("💜")));
	ColourCodex.Add(EColourName::Periwinkle, FColourDescriptor(TEXT("Periwinkle"), TEXT("Dreamy lavender-blue"), EColourCategory::Pastel, FLinearColor(0.8f, 0.8f, 1.0f), TEXT("🌸")));
}

//------------------------------------------------------------------------------
// Accessor methods
//------------------------------------------------------------------------------

FLinearColor UColourCodex::GetColor(EColourName Colour) const
{
	const FColourDescriptor* Desc = ColourCodex.Find(Colour);
	return Desc ? Desc->TintA : FLinearColor::White;
}

const FColourDescriptor* UColourCodex::GetDescriptor(EColourName Colour) const
{
	return ColourCodex.Find(Colour);
}

FColourDescriptor UColourCodex::GetDescriptorChecked(EColourName Colour) const
{
	const FColourDescriptor* Desc = ColourCodex.Find(Colour);
	return Desc ? *Desc : FColourDescriptor();
}

FLinearColor UColourCodex::GetTint(EColourName Colour, int32 TintIndex) const
{
	const FColourDescriptor* Desc = ColourCodex.Find(Colour);
	if (!Desc)
	{
		return FLinearColor::White;
	}

	if (TintIndex == 2)
	{
		return Desc->TintB;
	}
	else if (TintIndex == 3)
	{
		return Desc->TintC;
	}
	else
	{
		return Desc->TintA;
	}
}

void UColourCodex::AddOrUpdateColour(EColourName Colour, const FColourDescriptor& Descriptor)
{
	ColourCodex.Add(Colour, Descriptor);
}

bool UColourCodex::RemoveColour(EColourName Colour)
{
	return ColourCodex.Remove(Colour) > 0;
}