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
				"AIModule",
				"GameplayTags",
				"GameplayAbilities",
				"GameplayTasks",
				"KzLib",
				"ScriptableFramework",
				"KzGameplay",
				"StateTreeModule",
				"GameplayStateTreeModule"
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