// Copyright Pocketpair, Inc. All Rights Reserved.


#include "PPHkNavRuntimeGenEditorCommands.h"

#include "BlueprintEditorModule.h"
#include "ToolMenus.h"
#include "ToolMenus.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraph/EdGraph.h"
#include "Blueprint/UserWidget.h"
#include "UObject/GCObjectScopeGuard.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "FPPHkNavRuntimeGenEditorCommands"

FPPHkNavRuntimeGenEditorCommands::FPPHkNavRuntimeGenEditorCommands()
	: TCommands<FPPHkNavRuntimeGenEditorCommands>(
		TEXT("PPHkNavRuntimeGenEditor"),
		NSLOCTEXT("PPHkNavRuntimeGenEditor", "PPHkNavRuntimeGenEditor", "PPHkNav"),
		NAME_None,
		FAppStyle::GetAppStyleSetName())
{
}

void FPPHkNavRuntimeGenEditorCommands::RegisterCommands()
{
	UI_COMMAND(BakeNavMesh,
		"Bake NavMesh",
		"開いている Blueprint の Bounds から NavMesh をベイクし DataAsset を生成します",
		EUserInterfaceActionType::Button,
		FInputChord());
}
#undef LOCTEXT_NAMESPACE
