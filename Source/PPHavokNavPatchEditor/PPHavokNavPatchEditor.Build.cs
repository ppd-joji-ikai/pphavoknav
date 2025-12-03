// Copyright Pocketpair, Inc. All Rights Reserved.

using UnrealBuildTool;
using UnrealBuildTool.Rules;

public class PPHavokNavPatchEditor : ModuleRules
{
	public PPHavokNavPatchEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrecompileForTargets = PrecompileTargetsType.Editor;

		if(!Target.bBuildEditor)
		{
			throw new System.Exception("PPHavokNavPatchEditor requires bBuildEditor=true.");
		}
		
		PublicDependencyModuleNames.AddRange(new []
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"Slate",
			"SlateCore",
			"PPHavokNavPatch",
			"HavokNavigation"
		});

		PrivateDependencyModuleNames.AddRange(new []
		{
			"InputCore",
			"EditorFramework",
			"LevelEditor",
			"RenderCore",
			"Projects"
		});
		
		this.SetupModuleHavokSupport(Target);
	}
}
