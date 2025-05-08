// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BA_Project : ModuleRules
{
	public BA_Project(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] { "InfodemicCore", "InfodemicNeo4jConnector" });
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
			"BA_Neo4j",
			"InfodemicCore",
			"InfodemicNeo4jConnector"
		});
	}
}
