// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class BA_ProjectTarget : TargetRules
{
	public BA_ProjectTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		ExtraModuleNames.Add("BA_Project");
		RegisterModulesCreatedByRider();
	}

	private void RegisterModulesCreatedByRider()
	{
		ExtraModuleNames.AddRange(new string[]
		{
			"UnrealLangChain",
			"Ba_Neo4j_Plugin",
			"BA_Neo4j",
			"LlamaCore",
			"AssetTagTreeEditor",
			"AssetTagTreeRuntime",
			"InfodemicCore",
			"InfodemicEditor",
			"InfodemicSystem",
			"InfodemicNodes",
		});
	}
}
