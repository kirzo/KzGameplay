// Copyright 2026 kirzo

using UnrealBuildTool;

public class KzGameplayLimbicEditor : ModuleRules
{
	public KzGameplayLimbicEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"AssetRegistry",
				"KzGameplay",
				"KzGameplayEditor",
				"KzGameplayLimbic",
				"Limbic",
				"LimbicCore",
				"LimbicEditor"
			}
			);
	}
}