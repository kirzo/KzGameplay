// Copyright 2026 kirzo

using UnrealBuildTool;

public class KzLighting : ModuleRules
{
	public KzLighting(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"KzLib"
			}
			);
	}
}