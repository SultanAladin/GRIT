using UnrealBuildTool;

public class TransitArchitectEditor : ModuleRules
{
	public TransitArchitectEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"TransitArchitectRuntime"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"Slate",
			"SlateCore",
			"PropertyEditor",
			"InputCore"
		});
	}
}
