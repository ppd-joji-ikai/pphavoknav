// Copyright Pocketpair, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FComponentVisualizer;
class FUICommandList;
class FToolBarBuilder;
class IAssetEditorInstance; // 利用: OnAssetOpenedInEditor から渡されるインスタンス
class FAssetEditorToolkit; // ツールバー拡張用に前方宣言

class FPPHkNavRuntimeGenEditorModule : public IModuleInterface
{
public:
    using ThisClass = FPPHkNavRuntimeGenEditorModule;
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
#if WITH_EDITOR
    void RegisterCommands();
    void UnRegisterCommands();

    void RegisterMenus();
    void UnRegisterMenus();

    void OnAssetOpenedInEditor(UObject* OpenedAsset, IAssetEditorInstance* Instance);
    void ExtendToolkit(IAssetEditorInstance* Instance, FAssetEditorToolkit* Toolkit, UBlueprint* OpenedBlueprint);

    static void ExecuteBakeSelected();
    static void ExecuteBakeForInstance(TWeakObjectPtr<UBlueprint> OpenedBlueprint);

private:
    FDelegateHandle Handle{};                        // ToolMenus 登録ハンドル
    FDelegateHandle AssetOpenedInEditorHandle{};     // 資産オープンイベントハンドル
    TSharedPtr<FComponentVisualizer> MultiSocketVisualizer; // 可視化

    TSharedPtr<FUICommandList> EditorCommands{};     // コマンド定義 (登録のみ)

    // 拡張済みインスタンス追跡 (重複防止)
    TSet<IAssetEditorInstance*> ExtendedInstances;
    // 各インスタンスに付与した Extender を保持 (必要ならクリーンアップ)
    TMap<IAssetEditorInstance*, TSharedPtr<FExtender>> InstanceToExtender;
#endif
};
