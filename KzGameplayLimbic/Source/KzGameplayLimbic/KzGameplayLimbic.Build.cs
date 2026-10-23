// Copyright 2026 kirzo

using UnrealBuildTool;

public class KzGameplayLimbic : ModuleRules
{
	public KzGameplayLimbic(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GameplayTags",
				"KzGameplay",
				"KzLib"
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Limbic"
			}
			);
	}
}