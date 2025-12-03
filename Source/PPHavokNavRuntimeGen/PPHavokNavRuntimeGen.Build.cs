// Copyright Pocketpair, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class PPHavokNavRuntimeGen : ModuleRules
{
	public PPHavokNavRuntimeGen(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				
				"GeometryCollectionEngine",
				"HavokCore",
				"HavokNavigation",
				"HavokNavigationGenerationCore",
				"HavokNavigationGeneration",
			}
		);

		PublicIncludePaths.AddRange(new string[]
		{
			Path.Combine(ModuleDirectory, "Public", "Components"),
			Path.Combine(ModuleDirectory, "Public", "GeometryProvider"),
		});

		
		if(Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd",
				}
			);
		}

		SetupModulePhysicsSupport(Target);
	}
}
