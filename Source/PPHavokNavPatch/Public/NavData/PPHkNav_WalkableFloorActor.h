// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PPHkNav_WalkableFloorActor.generated.h"

class UPPHkNav_NavMeshDataAsset;
class UPPHkNavNavMeshMultiPatchComponent;
class UHavokNavNavMesh;
class UHavokNavClusterGraph;

/**
 * 動的スポーン可能な歩行床
 * DataAsset の UHavokNavNavMesh を複製し、インスタンスセットに登録する
 * BP_WalkableFloor の C++ ベース
 */
UCLASS(Blueprintable)
class PPHKNAVPATCH_API APPHkNav_WalkableFloorActor : public AActor
{
	GENERATED_BODY()

public:
	APPHkNav_WalkableFloorActor(const FObjectInitializer& ObjectInitializer);

	/** Blueprint から明示的に再ロード／再構築 */
	UFUNCTION(BlueprintCallable, Category = "WalkableFloor")
	bool ReloadNavMesh();

protected:
	//~ AActor
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** NavMesh Patchをインスタンス化 */
	bool InstantiateNavMeshInternal();

	/** 既存登録の破棄 */
	void TeardownNavMeshInternal();

protected:
	/** 利用する事前ベイク NavMesh DataAsset */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WalkableFloor")
	TObjectPtr<UPPHkNav_NavMeshDataAsset> NavMeshDataAsset{nullptr};

	/** BeginPlay で自動ロード＆登録 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WalkableFloor")
	bool bAutoInstantiateOnBeginPlay{true};

	/** 既に NavMesh を複製・登録済みか */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "WalkableFloor")
	bool bNavMeshInstantiated{false};

	/** 実際のインスタンスセットコンポーネント */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPPHkNavNavMeshMultiPatchComponent> MultiPatchComponent;
};
