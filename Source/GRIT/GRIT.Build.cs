// GRIT.Build.cs (transferred from RIFT.Build.cs)
using System.IO;
using System.Linq;
using UnrealBuildTool;

public class GRIT : ModuleRules
{
	public GRIT(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore", "GameplayTags", "PhysicsCore", "Chaos", "ChaosCore", "EpicAdapter" });

		PrivateDependencyModuleNames.AddRange(new string[] { "UMG", "Slate", "SlateCore", "GameplayTags", "Chaos" });

		// Editor-only dependencies
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}

		var publicIncludePaths = new[]
		{
			ModuleDirectory,
			Path.Combine(ModuleDirectory, "VehicleFramework"),
			Path.Combine(ModuleDirectory, "GameContext"),
			Path.Combine(ModuleDirectory, "GameContext", "AuthenticationContext"),
			Path.Combine(ModuleDirectory, "UserInterface"),
			Path.Combine(ModuleDirectory, "UserInterface", "Components")
		}
		.Where(Directory.Exists)
		.ToArray();

		if (publicIncludePaths.Length > 0)
		{
			PublicIncludePaths.AddRange(publicIncludePaths);
		}

		var privateIncludePaths = new[]
		{
			Path.Combine(ModuleDirectory, "VehicleFramework"),
			Path.Combine(ModuleDirectory, "VehicleFramework", "Components"),
			Path.Combine(ModuleDirectory, "VehicleFramework", "Components", "Constructs"),
			Path.Combine(ModuleDirectory, "VehicleFramework", "Controllers"),
			Path.Combine(ModuleDirectory, "VehicleFramework", "Input"),
			Path.Combine(ModuleDirectory, "VehicleFramework", "UserInterfaces"),
			Path.Combine(ModuleDirectory, "GameContext"),
			Path.Combine(ModuleDirectory, "GameContext", "SpawnControl"),
			Path.Combine(ModuleDirectory, "GameContext", "ColourCodex", "Public"),
			Path.Combine(ModuleDirectory, "GameContext", "ColourCodex", "Private"),
			Path.Combine(ModuleDirectory, "GameContext", "AuthenticationContext"),
			Path.Combine(ModuleDirectory, "UserInterface"),
			Path.Combine(ModuleDirectory, "UserInterface", "Components")
		}
		.Where(Directory.Exists)
		.ToArray();

		if (privateIncludePaths.Length > 0)
		{
			PrivateIncludePaths.AddRange(privateIncludePaths);
		}

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