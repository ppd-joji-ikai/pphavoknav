// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "IHavokNavNavMeshGenerationInputEntityGatherer.h"
#include "PPHkNav_BoundsInputEntityGatherer.generated.h"

class AActor;
struct FHavokNavNavMeshGenerationInput;
struct FHavokNavNavMeshUserEdgeStaticInformationSet;

/**
 * @brief 指定範囲内のジオメトリを収集するNavMeshジェネレーター用エンティティ収集クラス
 * 指定された境界内のすべてのプリミティブをスキャンしてNavMesh生成用のエンティティを収集します
 * ランタイムNavMesh生成に最適化されています
 */
UCLASS()
class PPHAVOKNAVRUNTIMEGEN_API UPPHkNav_BoundsInputEntityGatherer : public UObject, public IHavokNavNavMeshGenerationInputEntityGatherer
{
    GENERATED_BODY()

    struct FPrimitive;

public:
    /**
     * @brief 初期化メソッド
     * @param InWorld ワールドコンテキスト
     */
    void Initialize(UWorld* InWorld)
    {
        World = InWorld;
    }

    // IHavokNavNavMeshGenerationInputEntityGatherer interface
    virtual FHavokNavNavMeshGenerationInputEntitySet GatherInputEntities(
        TSubclassOf<UHavokNavNavMeshLayer> Layer,
        FTransform const& GenerationTransform,
        FHavokNavNavMeshGenerationBounds const& GenerationLocalBounds,
        TScriptInterface<IHavokNavNavMeshGenerationController> Controller,
        float InputGatheringBoundsExpansion) const override;

protected:
    /**
     * @brief 指定された範囲内のプリミティブを収集します
     * @param Layer NavMeshレイヤークラス
     * @param GenerationTransform 生成空間の変換
     * @param GenerationLocalBounds 生成範囲のローカル境界
     * @param InputGatheringBoundsExpansion 収集範囲の拡張
     * @param PrimitivesInOut 収集されたプリミティブの出力配列
     */
    virtual void CollectPrimitives(
        TSubclassOf<UHavokNavNavMeshLayer> Layer,
        FTransform const& GenerationTransform,
        const FHavokNavNavMeshGenerationBounds& GenerationLocalBounds,
        float InputGatheringBoundsExpansion,
        TArray<FPrimitive>& PrimitivesInOut) const;

    /**
     * @brief プリミティブを処理してEntitySetに追加します
     * @param Primitive 処理するプリミティブ
     * @param GenerationTransform 生成空間の変換
     * @param Controller ジェネレーションコントローラー
     * @param InputEntitiesOut 出力用のエンティティセット
     */
    virtual void ProcessPrimitive(
        const FPrimitive& Primitive,
        FTransform const& GenerationTransform,
        TScriptInterface<IHavokNavNavMeshGenerationController> Controller,
        FHavokNavNavMeshGenerationInputEntitySet& InputEntitiesOut) const;

    /**
     * @brief アクターからSeedPointとUserEdgeコンポーネントを検出して処理します
     * @param Actor 処理するアクター
     * @param Layer NavMeshレイヤークラス
     * @param GenerationTransform 生成空間の変換
     * @param GenerationLocalBounds 生成範囲のローカル境界
     * @param Controller ジェネレーションコントローラー
     * @param InputEntitiesOut 出力用のエンティティセット
     * @param UserEdgeDataSetBuilder UserEdgeデータビルダー
     */
    virtual void ProcessSeedPointsAndUserEdges(
        AActor const* Actor,
        TSubclassOf<UHavokNavNavMeshLayer> Layer,
        FTransform const& GenerationTransform,
        FHavokNavNavMeshGenerationBounds const& GenerationLocalBounds,
        TScriptInterface<IHavokNavNavMeshGenerationController> Controller,
        FHavokNavNavMeshGenerationInputEntitySet& InputEntitiesOut,
        FHavokNavNavMeshUserEdgeStaticInformationSet::FBuilder& UserEdgeDataSetBuilder) const;

private:
    /** ワールドへの参照 */
    UPROPERTY()
    TWeakObjectPtr<UWorld> World;
};
