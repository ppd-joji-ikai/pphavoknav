// Copyright Pocketpair, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "HavokNavTypes.h"
#include "Components/SceneComponent.h"
#include "PPHkNavInstancedNavMeshComponent.generated.h"

class UPPHkNav_NavMeshDataAsset;
class UPPHkNavInstancedNavMeshUserEdgeGenerator;
class UHavokNavNavMesh;
class UHavokNavNavMeshInstance;
class UHavokNavWorldSubsystem;
class UHavokNavWorld;
class UHavokNavNavMeshUserEdge;

namespace PPHkNav::Private { struct FInstancedNavMeshComponentInternal; }

/** インスタンス一意識別子。Index とは独立し RemoveAtSwap 後も安定 */
USTRUCT(BlueprintType)
struct PPHAVOKNAVPATCH_API FPPHkNavInstancedNavMeshInstanceId
{
	GENERATED_BODY()
public:
	FPPHkNavInstancedNavMeshInstanceId() = default;
	explicit FPPHkNavInstancedNavMeshInstanceId(const int32 In) : Value(In) {}
	friend bool operator==(const FPPHkNavInstancedNavMeshInstanceId& L, const FPPHkNavInstancedNavMeshInstanceId& R){ return L.Value == R.Value; }
	friend bool operator!=(const FPPHkNavInstancedNavMeshInstanceId& L, const FPPHkNavInstancedNavMeshInstanceId& R){ return !(L==R); }
	friend uint32 GetTypeHash(const FPPHkNavInstancedNavMeshInstanceId& Id){ return ::GetTypeHash(Id.Value); }
	bool IsValid() const { return Value != INDEX_NONE; }

public:
	UPROPERTY()
	int32 Value{INDEX_NONE};
};

/** 単一インスタンス記録 (内部管理用) */
USTRUCT()
struct PPHAVOKNAVPATCH_API FPPHkNavInstancedNavMeshInstanceRecord
{
	GENERATED_BODY()
	/** 永続 Id */
	UPROPERTY() int32 Id{INDEX_NONE};
	/** Component 相対 Transform */
	UPROPERTY() FTransform RelativeTransform{};
	/** 明示 LevelInstanceId 指定 (0 なら自動) */
	UPROPERTY() uint64 LevelInstanceIdOverride{0ULL};
	/** Runtime ロード結果 */
	UPROPERTY(Transient) TObjectPtr<UHavokNavNavMeshInstance> RuntimeInstance{nullptr};
	/** まだロード未試行 (World 未初期化など) */
	UPROPERTY() bool bPendingLoad{true};

	/** ユーザーエッジセット識別子 (ロード後に設定される) */
	FHavokNavNavMeshDynamicUserEdgeSetIdentifier EdgeSetId;
};

/**
 * NavMesh の事前ベイク結果 (UPPHkNav_NavMeshDataAsset 経由) を大量インスタンス化し Transform のみを差異として保持するコンポーネント。
 */
UCLASS(ClassGroup=(PPHkNav), meta=(BlueprintSpawnableComponent), BlueprintType)
class PPHAVOKNAVPATCH_API UPPHkNavInstancedNavMeshComponent : public USceneComponent
{
	GENERATED_BODY()
public:
	UPPHkNavInstancedNavMeshComponent();

	using FInstanceId = FPPHkNavInstancedNavMeshInstanceId;
	using FInstanceRecord = FPPHkNavInstancedNavMeshInstanceRecord;

	/** インスタンス追加 (Transform は Component 相対。bWorldSpace=true でワールド基準) */
	FInstanceId AddInstance(const FTransform& InstanceTransform, bool bWorldSpace=false);

	/** バッチ追加 */
	TArray<FInstanceId> BatchAddInstances(const TConstArrayView<FTransform> InstanceTransforms, bool bWorldSpace=false);

	/** 単一インスタンス Transform 更新 */
	void UpdateInstanceTransform(FInstanceId InstanceId, const FTransform& NewTransform, const bool bWorldSpace=false);

	/** バッチ更新 */
	void BatchUpdateInstanceTransforms(const TConstArrayView<FInstanceId> InstanceIds, const TConstArrayView<FTransform> NewTransforms, const bool bWorldSpace=false);

	/** 削除 */
	bool RemoveInstance(FInstanceId InstanceId);

	/** 複数削除 */
	bool BatchRemoveInstances(const TConstArrayView<FInstanceId> InstanceIds);

	/** 全破棄 */
	UFUNCTION(BlueprintCallable, Category="InstancedNavMesh")
	void ClearInstances();

	/** 数 */
	UFUNCTION(BlueprintCallable, Category="InstancedNavMesh")
	int32 GetInstanceCount() const { return InstanceRecords.Num(); }

	/** 有効 Id 判定 */
	bool IsValidInstanceId(const FInstanceId& InstanceId) const { return InstanceId.IsValid() && IdToIndexMap.Contains(InstanceId.Value); }

public:
	// Blueprint API
	UFUNCTION(BlueprintCallable, Category="InstancedNavMesh")
	FPPHkNavInstancedNavMeshInstanceId AddInstanceBP(const FTransform& InstanceTransform, const bool bWorldSpace)
	{
		return AddInstance(InstanceTransform, bWorldSpace);
	}

	UFUNCTION(BlueprintCallable, Category="InstancedNavMesh")
	void RemoveInstanceBP(const FPPHkNavInstancedNavMeshInstanceId& InstanceId)
	{
		RemoveInstance(InstanceId);
	}
	
protected:
	// UObject / Component ライフサイクル
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Activate(bool bReset) override;
	virtual void Deactivate() override;
	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) override;

#if WITH_EDITOR
public:
	// Editor 可視化用 Getter （ランタイムビルドでは露出しない）
	UPPHkNav_NavMeshDataAsset* GetNavMeshDataAsset() const { return NavMeshDataAsset; }
#endif // WITH_EDITOR

private:
	friend struct PPHkNav::Private::FInstancedNavMeshComponentInternal;
	void HandleRuntimeInstanceLoaded(UHavokNavNavMeshInstance* LoadedInstance);

private: // データ
	/** ベース DataAsset */
	UPROPERTY(EditAnywhere, Category="InstancedNavMesh")
	TObjectPtr<UPPHkNav_NavMeshDataAsset> NavMeshDataAsset{nullptr};

	/** 追加 StreamingSet ビルド */
	UPROPERTY(EditAnywhere, Category="InstancedNavMesh", AdvancedDisplay)
	bool bBuildAdditionalStreamingSetsOnLoad{true};

	/** ユーザーエッジ生成器 */
	UPROPERTY(EditAnywhere, Instanced, Category="Edges")
	TObjectPtr<UPPHkNavInstancedNavMeshUserEdgeGenerator> EdgeGenerator;
	
	/** インスタンス配列 */
	UPROPERTY(Transient)
	TArray<FPPHkNavInstancedNavMeshInstanceRecord> InstanceRecords;

	/** Id -> Index マップ (RemoveAtSwap 対応) */
	UPROPERTY(Transient)
	TMap<int32,int32> IdToIndexMap;

	/** 次に割り当てる Id */
	UPROPERTY(Transient)
	int32 NextInstanceId{1}; // 0 と -1は無効値
};
