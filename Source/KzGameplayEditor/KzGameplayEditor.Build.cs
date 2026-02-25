// Copyright 2026 kirzo

using UnrealBuildTool;

public class KzGameplayEditor : ModuleRules
{
	public KzGameplayEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"KzGameplay",
				"KzLibEditor"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"PropertyEditor",
				"InputCore",
				"Projects",
				"BlueprintGraph"
			});
	}
}