// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BA_Project : ModuleRules
{
	public BA_Project(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"LlamaCore",
			"HTTP",
			"BA_Neo4j"
		});
	}
}
