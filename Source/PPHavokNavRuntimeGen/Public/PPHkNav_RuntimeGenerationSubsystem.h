// Copyright (c) 2024 Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IPPHkNav_QueuedRequest.h"
#include "PPHkNavHavokNavMeshGeneratorFactory.h" // For FPPHkNavMeshGenerationParams
#include "PPHkNavHavokNavVolumeGeneratorFactory.h" // For FPPHkNavVolumeGenerationParams
#include "PPHkNav_RuntimeGenerationSubsystem.generated.h"

class UPPHkNav_QueuedGenerationRequest;
class UPPHkNav_QueuedVolumeGenerationRequest;
class IHavokNavNavMeshConnectionsGenerator;

/**
 * @brief ランタイム動的ナビメッシュ生成サブシステム
 * 生成リクエストのキューを管理し、優先度に基づいて順次処理します
 */
UCLASS()
class PPHKNAVRUNTIMEGEN_API UPPHkNav_RuntimeGenerationSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UPPHkNav_RuntimeGenerationSubsystem();

	// UWorldSubsystem overrides
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// End UWorldSubsystem overrides

	//~ Begin FTickableGameObject overrides
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	//~ End FTickableGameObject overrides

	/**
	 * @brief 新しいナビメッシュ生成リクエストをキューに追加します
	 * @param InParams 生成タスクのパラメータ
	 */
	void RequestGeneration(FPPHkNavMeshGenerationParams&& InParams);

	/**
	 * @brief 新しいナビボリューム生成リクエストをキューに追加します
	 * @param InParams ナビボリューム生成タスクのパラメータ
	 */
	void RequestVolumeGeneration(FPPHkNavVolumeGenerationParams&& InParams);

	/** 進行中およびキューに入っているすべての生成タスクをキャンセルします */
	void CancelAllGeneration();

	/** 現在生成中かどうかを返します */
	bool IsGenerating() const;

	/**
	 * @brief リクエストの完了を報告します（統一されたコールバック）
	 * @param Request 完了したリクエスト
	 */
	void ReportRequestFinished(IPPHkNav_QueuedRequest* Request);

	// 後方互換性のための個別メソッド（Deprecatedにする予定）
	void ReportRequestFinished(UPPHkNav_QueuedGenerationRequest* Request);
	void ReportVolumeRequestFinished(UPPHkNav_QueuedVolumeGenerationRequest* Request);

private:
	/** 次のリクエストを処理します */
	void ProcessNextRequest();

	/** 現在処理中かどうかを確認します */
	bool IsBusy() const;

	/**
	 * @brief リクエストを優先度順に挿入します
	 * @param Request 挿入するリクエスト
	 */
	void InsertRequestByPriority(TScriptInterface<IPPHkNav_QueuedRequest> Request);

private:
	UPROPERTY(Transient)
	TScriptInterface<IHavokNavNavMeshConnectionsGenerator> NavMeshConnectionsGenerator;

	/** 統一されたリクエストキュー（優先度順） */
	UPROPERTY(Transient)
	TArray<TScriptInterface<IPPHkNav_QueuedRequest>> RequestQueue{};

	/** 現在処理中のリクエスト */
	UPROPERTY(Transient)
	TScriptInterface<IPPHkNav_QueuedRequest> CurrentRequest{};
};
