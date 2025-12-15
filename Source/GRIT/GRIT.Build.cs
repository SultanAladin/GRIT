// GRIT.Build.cs (transferred from RIFT.Build.cs)
using UnrealBuildTool;

public class GRIT : ModuleRules
{
	public GRIT(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore", "PhysicsCore", "Chaos", "ChaosCore" });

		PrivateDependencyModuleNames.AddRange(new string[] { "UMG", "Slate", "SlateCore", "Chaos" });

		PublicIncludePaths.AddRange(
			new string[]
			{
				"GRIT/VehicleFramework"
			});

		PrivateIncludePaths.AddRange(
			new string[]
			{
				"GRIT/VehicleFramework/Components",
				"GRIT/VehicleFramework/Components/Constructs",
				"GRIT/VehicleFramework/Controllers",
				"GRIT/VehicleFramework/Input",
				"GRIT/VehicleFramework",
				"GRIT/GameContext",
				"GRIT/GameContext/SpawnControl",
				"GRIT/GameContext/ColourCodex/Public",
				"GRIT/GameContext/ColourCodex/Private"
			});

		//------------------------------------------------------------------------------
		//                          P2P REPLICATION
		//------------------------------------------------------------------------------
		// Set P2P=1 to enable P2P physics replication
		// Set P2P=0 for offline/single-player mode (no replication overhead)
		PublicDefinitions.Add("P2P=1");

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}