// Copyright Pocketpair, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PPHavokNavRuntimeGenEditor : ModuleRules
{
	public PPHavokNavRuntimeGenEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrecompileForTargets = PrecompileTargetsType.Editor;

		if(!Target.bBuildEditor)
		{
			throw new System.Exception("PPHavokNavRuntimeGenEditor requires bBuildEditor=true.");
		}
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"DeveloperSettings",
				"HavokNavigation",
				"HavokNavigationGeneration",
				"HavokNavigationGenerationCore",
				
				"PPHavokNavRuntimeGen",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"ToolMenus",
				"AssetRegistry",
				
				"PPHavokNavPatch", 
				"PPHavokNavRuntimeGen",
			}
		);
	}
}
