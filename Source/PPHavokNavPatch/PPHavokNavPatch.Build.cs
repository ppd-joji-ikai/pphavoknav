// Copyright Pocketpair, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class PPHavokNavPatch : ModuleRules
{
	public PPHavokNavPatch(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GameplayTags",
				"HavokCore",
				"HavokNavigation",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"NavigationSystem",
				"HavokCore",
				"HavokNavigation",
			}
		);
		
		PublicIncludePaths.AddRange(new string[]
		{
			Path.Combine(ModuleDirectory, "Public", "Components"),
			Path.Combine(ModuleDirectory, "Public", "NavData"),
			Path.Combine(ModuleDirectory, "Public", "NavMesh"),
			Path.Combine(ModuleDirectory, "Public", "UserEdges"),
			Path.Combine(ModuleDirectory, "Public", "Util"),
		});

		AddEngineThirdPartyPrivateStaticDependencies(Target, "HavokSDK");
	}
}
