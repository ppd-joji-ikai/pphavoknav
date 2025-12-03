// Copyright Pocketpair, Inc. All Rights Reserved.

#include "PPHkNavRuntimeGenEditor.h"

#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
#include "PPHkNav_NavMeshBakeUtility.h"
#include "Editor.h"
#include "PPHkNavRuntimeGenEditorCommands.h"
#include "PPHkNavMultiUserEdgeSocketComponentVisualizer.h"
#include "PPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "Toolkits/AssetEditorToolkit.h"          // FAssetEditorToolkit
#include "Subsystems/AssetEditorSubsystem.h"      // UAssetEditorSubsystem
#include "Toolkits/IToolkitHost.h"
#include "Engine/Blueprint.h"

#define LOCTEXT_NAMESPACE "FPPHkNavRuntimeGenEditorModule"

void FPPHkNavRuntimeGenEditorModule::StartupModule()
{
#if WITH_EDITOR
    RegisterCommands();
    Handle = UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &ThisClass::RegisterMenus));

    if (GUnrealEd)
    {
        MultiSocketVisualizer = MakeShareable(new FPPHkNavMultiUserEdgeSocketComponentVisualizer());
        GUnrealEd->RegisterComponentVisualizer(UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly::StaticClass()->GetFName(), MultiSocketVisualizer);
        if (MultiSocketVisualizer.IsValid())
        {
            MultiSocketVisualizer->OnRegister();
        }
    }

    if (GEditor)
    {
        if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
        {
            AssetOpenedInEditorHandle = AssetEditorSubsystem->OnAssetOpenedInEditor().AddRaw(this, &ThisClass::OnAssetOpenedInEditor);
        }
    }
#endif
}

void FPPHkNavRuntimeGenEditorModule::ShutdownModule()
{
#if WITH_EDITOR
    UnRegisterCommands();
    UnRegisterMenus();

    if (GEditor)
    {
        if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
        {
            if (AssetOpenedInEditorHandle.IsValid())
            {
                AssetEditorSubsystem->OnAssetOpenedInEditor().Remove(AssetOpenedInEditorHandle);
                AssetOpenedInEditorHandle.Reset();
            }
        }
    }
#endif
}

IMPLEMENT_MODULE(FPPHkNavRuntimeGenEditorModule, PPHavokNavRuntimeGenEditor)

#if WITH_EDITOR
void FPPHkNavRuntimeGenEditorModule::RegisterCommands()
{
    FPPHkNavRuntimeGenEditorCommands::Register();
    EditorCommands = MakeShareable(new FUICommandList);
}

void FPPHkNavRuntimeGenEditorModule::UnRegisterCommands()
{
    EditorCommands.Reset();
    FPPHkNavRuntimeGenEditorCommands::Unregister();
}

void FPPHkNavRuntimeGenEditorModule::RegisterMenus()
{
    if (UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools"))
    {
        FToolMenuSection& Section = ToolsMenu->AddSection("PPHkNavRuntimeGenEditor", FText::FromString(TEXT("PPHkNav")));
        Section.AddMenuEntry(
            "PPHkNavBakeSelectedNavMesh",
            FText::FromString(TEXT("Bake Selected NavMesh")),
            FText::FromString(TEXT("選択した Blueprint から NavMesh をベイクし DataAsset を生成")),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateStatic(&ThisClass::ExecuteBakeSelected))
        );
    }

    if (UToolMenu* BPAssetMenu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.Blueprint"))
    {
        FToolMenuSection& Section = BPAssetMenu->AddSection("PPHkNavRuntimeGenEditor_CB", FText::FromString(TEXT("PPHkNav")));
        Section.AddMenuEntry(
            "MassPalBakeSelectedNavMesh_CB",
            FText::FromString(TEXT("PPHkNav: 選択 Blueprint のNavMeshをベイク")),
            FText::FromString(TEXT("選択 Blueprint の境界から NavMesh をベイクし DataAsset を作成")),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateStatic(&ThisClass::ExecuteBakeSelected))
        );
    }
}

void FPPHkNavRuntimeGenEditorModule::UnRegisterMenus()
{
    // Extender クリーンアップ
    for (auto& Pair : InstanceToExtender)
    {
        Pair.Value.Reset();
    }
    InstanceToExtender.Empty();
    ExtendedInstances.Empty();

    if (GUnrealEd && MultiSocketVisualizer.IsValid())
    {
        GUnrealEd->UnregisterComponentVisualizer(UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly::StaticClass()->GetFName());
        MultiSocketVisualizer.Reset();
    }
    if (Handle.IsValid())
    {
        UToolMenus::UnRegisterStartupCallback(Handle);
        Handle.Reset();
    }
}

void FPPHkNavRuntimeGenEditorModule::OnAssetOpenedInEditor(UObject* OpenedAsset, IAssetEditorInstance* Instance)
{
    if (!OpenedAsset || !Instance) { return; }
    UBlueprint* const OpenedBP = Cast<UBlueprint>(OpenedAsset);
    if (!OpenedBP) { return; } // Blueprint 以外対象外

    if (ExtendedInstances.Contains(Instance)) { return; }

    FAssetEditorToolkit* Toolkit = static_cast<FAssetEditorToolkit*>(Instance);
    if (!Toolkit) { return; }

    ExtendToolkit(Instance, Toolkit, OpenedBP);
}

void FPPHkNavRuntimeGenEditorModule::ExtendToolkit(IAssetEditorInstance* Instance, FAssetEditorToolkit* Toolkit, UBlueprint* OpenedBlueprint)
{
    if (!Instance || !Toolkit || !OpenedBlueprint) { return; }

    const TSharedPtr<FUICommandList> ToolkitCommands = Toolkit->GetToolkitCommands();
    if (!ToolkitCommands.IsValid()) { return; }

    ToolkitCommands->MapAction(
        FPPHkNavRuntimeGenEditorCommands::Get().BakeNavMesh,
        FExecuteAction::CreateStatic(&ThisClass::ExecuteBakeForInstance, MakeWeakObjectPtr(OpenedBlueprint))
    );

    const TSharedPtr<FExtender> Extender = MakeShared<FExtender>();
    Extender->AddToolBarExtension(
        "Asset",
        EExtensionHook::After,
        ToolkitCommands,
        FToolBarExtensionDelegate::CreateLambda([=](FToolBarBuilder& Builder)
        {
            Builder.AddSeparator();
            Builder.AddToolBarButton(
                FPPHkNavRuntimeGenEditorCommands::Get().BakeNavMesh,
                NAME_None,
                TAttribute<FText>(),
                TAttribute<FText>(),
                FSlateIcon()
            );
        })
    );

    Toolkit->AddToolbarExtender(Extender);

    ExtendedInstances.Add(Instance);
    InstanceToExtender.Add(Instance, Extender);
}

void FPPHkNavRuntimeGenEditorModule::ExecuteBakeSelected()
{
    UPPHkNav_NavMeshBakeUtility* Utility = NewObject<UPPHkNav_NavMeshBakeUtility>();
    Utility->AddToRoot();
    Utility->BakeSelected();
    Utility->RemoveFromRoot();
}

void FPPHkNavRuntimeGenEditorModule::ExecuteBakeForInstance(TWeakObjectPtr<UBlueprint> OpenedBlueprint)
{
    UBlueprint* BP = OpenedBlueprint.Get();
    if (!BP) { return;}
    
    UPPHkNav_NavMeshBakeUtility* BakeUtility = NewObject<UPPHkNav_NavMeshBakeUtility>(GetTransientPackage());
    BakeUtility->AddToRoot();
    BakeUtility->BakeBlueprint(BP);
    BakeUtility->RemoveFromRoot();
}
#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
