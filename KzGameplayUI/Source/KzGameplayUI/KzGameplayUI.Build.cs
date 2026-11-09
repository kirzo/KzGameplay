// Copyright 2026 kirzo

using UnrealBuildTool;

public class KzGameplayUI : ModuleRules
{
	public KzGameplayUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GameplayAbilities",
				"GameplayTags",
				"KzUI",
				"UMG"
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"EnhancedInput",
				"InputCore",
				"KzGameplay",
				"KzLib"
			}
			);
	}
}