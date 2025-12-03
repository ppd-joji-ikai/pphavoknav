// Copyright (c) 2025 Pocketpair, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PPHkNavNavMeshMultiPatchComponent.generated.h"

class UHavokNavNavMesh;
class UHavokNavNavMeshInstance;
class UHavokNavNavMeshConnection;
class UHavokNavWorldSubsystem;
class UHavokNavWorld;
class UHavokNavClusterGraph;

USTRUCT()
struct FPPHkNavPatchEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UHavokNavNavMesh> NavMesh{}; // UAsset NavMesh
	UPROPERTY()
	TObjectPtr<UHavokNavNavMeshInstance> Instance{}; // ランタイム Instance
	UPROPERTY()
	FTransform Transform{};
	UPROPERTY()
	FGuid Guid{}; // キャッシュ
	/** AddPatch で LevelInstanceId を外部指定した場合のオーバーライド値。未使用時 0 */
	UPROPERTY(Transient)
	uint64 LevelInstanceIdOverride{}; // 0 で未設定扱い
	/** 外部指定 LevelInstanceId を使用するか */
	UPROPERTY(Transient)
	bool bUseLevelInstanceIdOverride{ false };
};

/**
 * 個別 Patch(複数) の NavMesh を 1 つのコンポーネントで管理し、インクリメンタルにロード/アンロードするためのコンポーネント。
 * 既存 UHavokNavNavMeshInstanceSetComponent の "Layer 毎 1 枚" 制約を回避し、同一 Layer 複数 NavMesh を保持可能にする。
 * Havok Navigation 本体コードへ改造を加えず、必要最低限のロード/接続処理のみ実装。
 */
UCLASS(ClassGroup=(PPHkNav), meta=(BlueprintSpawnableComponent), BlueprintType)
class PPHKNAVPATCH_API UPPHkNavNavMeshMultiPatchComponent : public USceneComponent
{
	GENERATED_BODY()
public:
	UPPHkNavNavMeshMultiPatchComponent();

	/** 単一 Patch を追加しロード*/
	void AddPatchNavMesh(UHavokNavNavMesh* NavMesh, const FTransform& PatchTransform);

	/** 単一 Patch を追加しロード (LevelInstanceId を外部指定) */
	void AddPatchNavMeshWithLevelInstanceId(UHavokNavNavMesh* NavMesh, const FTransform& PatchTransform, uint64 LevelInstanceId);

	/** NavMesh GUID で Patch をアンロード/削除 */
	bool RemovePatchByGuid(const FGuid& NavMeshGuid);

	/** NavMesh 参照で Patch をアンロード/削除 */
	bool RemovePatchByNavMesh(UHavokNavNavMesh* NavMesh);

	/** 全 Patch アンロード */
	void ResetPatches();

	/** 現在ロード済み NavMesh 配列を取得 (弱参照解決済み有効分) */
	TArray<UHavokNavNavMesh*> GetLoadedNavMeshes() const;

	/** ロード済み Patch NavMeshInstances に速度適用 */
	void SetPatchInstancesVelocity(FVector LinearVelocity, FVector AngularVelocity);

protected:
	// USceneComponent
	virtual void Activate(bool bReset) override;
	virtual void Deactivate() override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) override;

private:
	/** NavMesh ポインタ or Guid でエントリ検索 */
	FPPHkNavPatchEntry* FindEntryByNavMesh(UHavokNavNavMesh* NavMesh);
	FPPHkNavPatchEntry* FindEntryByGuid(const FGuid& Guid);

	void TryLoadEntry(FPPHkNavPatchEntry& Entry);
	void TryUnloadEntry(FPPHkNavPatchEntry& Entry, bool bRemoveInstanceRef);
	void TryLoadAll();
	void TryUnloadAll();

private:
	/** Load 時に追加 StreamingSet を構築するか*/
	UPROPERTY(EditAnywhere, Category="PPHkNav|NavMesh|Patch", AdvancedDisplay)
	bool bBuildAdditionalStreamingSetsOnLoad = true;

	UPROPERTY(Transient)
	TArray<FPPHkNavPatchEntry> PatchEntries;
};
