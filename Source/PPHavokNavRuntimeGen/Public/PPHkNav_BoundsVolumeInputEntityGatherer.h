// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "IHavokNavNavVolumeGenerationInputEntityGatherer.h"
#include "PPHkNav_BoundsVolumeInputEntityGatherer.generated.h"

class AActor;
struct FHavokNavNavVolumeGenerationInput;
struct FHavokNavNavVolumeUserEdgeStaticInformationSet;

/**
 * @brief 指定範囲内のジオメトリを収集するNavVolumeジェネレーター用エンティティ収集クラス
 * 指定された境界内のすべてのプリミティブをスキャンしてNavVolume生成用のエンティティを収集します
 * ランタイムNavVolume生成に最適化されています
 */
UCLASS()
class PPHAVOKNAVRUNTIMEGEN_API UPPHkNav_BoundsVolumeInputEntityGatherer : public UObject, public IHavokNavNavVolumeGenerationInputEntityGatherer
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

    // IHavokNavNavVolumeGenerationInputEntityGatherer interface
    virtual FHavokNavNavVolumeGenerationInputEntitySet GatherInputEntities(
        TSubclassOf<UHavokNavNavVolumeLayer> Layer,
        FHavokNavAxialTransform const& GenerationTransform,
        FHavokNavNavVolumeGenerationBounds const& GenerationLocalBounds,
        TScriptInterface<IHavokNavNavVolumeGenerationController> Controller,
        FBox const& InputGatheringBoundsExpansion) const override;

protected:
    /**
     * @brief 指定された範囲内のプリミティブを収集します
     * @param Layer NavVolumeレイヤークラス
     * @param GenerationTransform 生成空間の変換
     * @param GenerationLocalBounds 生成範囲のローカル境界
     * @param InputGatheringBoundsExpansion 収集範囲の拡張
     * @param PrimitivesInOut 収集されたプリミティブの出力配列
     */
    virtual void CollectPrimitives(
        TSubclassOf<UHavokNavNavVolumeLayer> Layer,
        FHavokNavAxialTransform const& GenerationTransform,
        const FHavokNavNavVolumeGenerationBounds& GenerationLocalBounds,
        FBox const& InputGatheringBoundsExpansion,
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
        FHavokNavAxialTransform const& GenerationTransform,
        TScriptInterface<IHavokNavNavVolumeGenerationController> Controller,
        FHavokNavNavVolumeGenerationInputEntitySet& InputEntitiesOut) const;

    /**
     * @brief アクターからSeedPointとUserEdgeコンポーネントを検出して処理します
     * @param Actor 処理するアクター
     * @param Layer NavVolumeレイヤークラス
     * @param GenerationTransform 生成空間の変換
     * @param GenerationLocalBounds 生成範囲のローカル境界
     * @param Controller ジェネレーションコントローラー
     * @param InputEntitiesOut 出力用のエンティティセット
     * @param UserEdgeDataSetBuilder UserEdgeデータビルダー
     */
    virtual void ProcessSeedPointsAndUserEdges(
        AActor const* Actor,
        TSubclassOf<UHavokNavNavVolumeLayer> Layer,
        FHavokNavAxialTransform const& GenerationTransform,
        FHavokNavNavVolumeGenerationBounds const& GenerationLocalBounds,
        TScriptInterface<IHavokNavNavVolumeGenerationController> Controller,
        FHavokNavNavVolumeGenerationInputEntitySet& InputEntitiesOut,
        FHavokNavNavVolumeUserEdgeStaticInformationSet::FBuilder& UserEdgeDataSetBuilder) const;

private:
    /** ワールドへの参照 */
    UPROPERTY()
    TWeakObjectPtr<UWorld> World;
};
