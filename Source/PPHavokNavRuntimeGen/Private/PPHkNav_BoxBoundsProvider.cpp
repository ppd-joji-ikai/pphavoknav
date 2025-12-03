// Copyright Pocketpair, Inc. All Rights Reserved.

#include "PPHkNav_BoxBoundsProvider.h"
#include "HavokNavGenerationUtilities.h"
#include "HavokNavNavMeshLayer.h"

FHavokNavNavMeshGenerationBounds UPPHkNav_BoxBoundsProvider::GetBounds(TSubclassOf<UHavokNavNavMeshLayer> Layer,
                                                                         FTransform const& GenerationTransform) const
{
	// レイヤーの確認
	if (TargetLayer && Layer != TargetLayer)
	{
		return {};
	}
	
	// ボックスが無効な場合は空を返す
	if (BoundingBox.IsValid == false)
	{
		return {};
	}

	// スケールが1であることを確認（NavMesh生成の要件）
	check(GenerationTransform.GetScale3D().Equals(FVector::OneVector));
	
	// ワールド空間からジェネレーション空間への変換行列
	const FTransform WorldToGenerationSpaceTransform = GenerationTransform.Inverse();
	
	// ワールド空間のボックスをジェネレーション空間に変換
	const FBox GenerationSpaceBounds = BoundingBox.TransformBy(WorldToGenerationSpaceTransform);

	// Positive bounds（NavMesh生成の対象となる領域）を生成
	FHavokNavLineLoop LineLoop;
	
	// ボックスの境界から2Dポリゴン（LineLoop）を作成
	// これは指定されたUpVectorに基づいて、2D平面上での閉じたポリゴンになる
	HavokNavGenerationUtilities::GetLineLoopPoints(BoundingBox, WorldToGenerationSpaceTransform, UpVector, LineLoop.Points);

	// NavMesh生成範囲を構築して返す
	return FHavokNavNavMeshGenerationBounds
	{
		// 全体がPositiveではない（特定領域のみNavMeshが生成される）
		.bEverywhereIsPositive = false,
		// 境界ボックス
		.BoundingBox = GenerationSpaceBounds,
		// 正の生成範囲（NavMeshが生成される領域）
		.PositiveBound = LineLoop,
		// 負の生成範囲（NavMeshが生成されない穴）- 今回は使用しない
		.NegativeBounds = {},
	};
}
