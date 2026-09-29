using UnrealBuildTool;

public class RockFormationRuntime : ModuleRules
{
	public RockFormationRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"ProceduralMeshComponent",
			"MeshDescription",
			"StaticMeshDescription"
		});
	}
}
