// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HavokNavTypes.h"
#include "HavokNavNavMeshUserEdgeDescription.h"
#include "PPHkNavInstancedNavMeshUserEdgeGenerator.generated.h"

struct FPPHkNavBakedNavMeshUserEdgeSocket;
class UPPHkNav_NavMeshDataAsset;
class UHavokNavNavMeshInstance;
class UHavokNavWorld;
class UPPHkNavInstancedNavMeshComponent;
class UHavokNavNavMeshUserEdge;

/**
 * Instanced NavMesh の間に動的ユーザーエッジを生成/管理するサービス
 * ユーザーエッジをセット単位で生成・破棄する
 * ひとつのNavMeshInstanceからは複数のユーザーエッジが生成されることがある
 * 設定値に従い大きなエッジは複数の小さなセグメントに分割されることがある
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class PPHAVOKNAVPATCH_API UPPHkNavInstancedNavMeshUserEdgeGenerator : public UObject
{
	GENERATED_BODY()

public:
	using FEdgeSetId = FHavokNavNavMeshDynamicUserEdgeSetIdentifier;
	UPPHkNavInstancedNavMeshUserEdgeGenerator();

	/**
	 * 指定ユーザーエッジソケットにしたがってエッジセットを生成する
	 * @remark エッジセットは leakしないように必ず RemoveEdges() で破棄してください
	 * @return EdgeSetId (失敗時は 無効値)
	 */
	[[nodiscard]]
	FEdgeSetId GenerateEdgesForInstanceSocket(UHavokNavWorld* NavWorld, UHavokNavNavMeshInstance* LoadedInstance, const TConstArrayView<FPPHkNavBakedNavMeshUserEdgeSocket> Sockets);

	/**
	 * 指定エッジセットを破棄
	 */
	void RemoveEdges(UHavokNavWorld* NavWorld, const FEdgeSetId EdgeSetId);

	// PPHkNav::NavMeshInstanceInitializerから呼ばれる
	[[nodiscard]]
	static FEdgeSetId GenerateEdgesForInstanceSocketLogic(UHavokNavWorld* NavWorld, UHavokNavNavMeshInstance* LoadedInstance, const TConstArrayView<FPPHkNavBakedNavMeshUserEdgeSocket> Sockets);

protected:
	virtual void FinishDestroy() override;

protected:
	TArray<FHavokNavNavMeshDynamicUserEdgeSetIdentifier> GeneratedEdgeSetIds;
};
