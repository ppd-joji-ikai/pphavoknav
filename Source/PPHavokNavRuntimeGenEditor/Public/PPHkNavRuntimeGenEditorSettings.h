// Copyright Pocketpair, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "HavokNavNavMeshGenerationComponent.h"
#include "Engine/DeveloperSettings.h"
#include "PPHkNavRuntimeGenEditorSettings.generated.h"

class UHavokNavNavMeshGenerationSettings;
class UHavokNavNavMeshLayer;

/**
 * エディタ用 NavMesh 事前ベイク既定値設定
 */
UCLASS(Config=Editor, DefaultConfig, DisplayName="PPHkNav NavGen Editor")
class PPHAVOKNAVRUNTIMEGENEDITOR_API UPPHkNavRuntimeGenEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 既定の NavMesh Layer (未指定時に使用) */
	UPROPERTY(Config, EditAnywhere, Category="PPHkNav|NavMeshBake")
	TSoftClassPtr<UHavokNavNavMeshLayer> DefaultNavMeshLayer;

	/** 生成コントローラのオーバーライド */
	UPROPERTY(Config, EditAnywhere, Category="PPHkNav|NavMeshBake")
	FHavokNavNavMeshGenerationControllerOverride DefaultControllerOverride{};

	/** 生成設定のオーバーライド
	 * 未設定のときHavok Navigationのデフォルト設定が採用されます
	 */
	UPROPERTY(Config, EditAnywhere, Category = "PPHkNav|NavMeshBake")
	TSoftObjectPtr<UHavokNavNavMeshGenerationSettings> DefaultGenerationSettingsOverride{};

	/** 既定の保存先フォルダ (/Game/...) */
	UPROPERTY(Config, EditAnywhere, Category="PPHkNav|NavMeshBake")
	FString DefaultTargetFolderPath{TEXT("/Game/NavMeshes")};

	/** 既定のアセット名接頭辞 */
	UPROPERTY(Config, EditAnywhere, Category="PPHkNav|NavMeshBake")
	FString DefaultAssetNamePrefix{TEXT("PBK_")};
};