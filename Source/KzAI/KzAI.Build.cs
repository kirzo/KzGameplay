// Copyright 2026 kirzo

using UnrealBuildTool;

public class KzAI : ModuleRules
{
	public KzAI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GameplayTags",
				"GameplayAbilities",
				"GameplayTasks",
				"KzLib",
				"ScriptableFramework"
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
			}
			);
	}
}