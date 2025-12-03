// Copyright (c) 2025 Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "PPHkNavHavokNavMeshGeneratorFactory.h"
#include "PPHkNav_RuntimeNavMeshBoundsVolume.generated.h"

class UHavokNavNavMeshLayer;
class UHavokNavNavMeshInstanceSetComponent;
class UPPHkNavNavMeshMultiPatchComponent;
struct FHavokNavNavMeshGeneratorResult;

/**
 * @class APPHkNav_RuntimeNavMeshBoundsVolume
 * @brief ボリュームの範囲内に動的にNavMeshを生成するアクター
 * エディタでボリュームの形状を編集し、ランタイムで指定された範囲内のNavMeshを生成します。
 * ボリュームの形状が変更された場合、自動的にNavMeshを再生成しない
 * ユーザーが明示的にRequestGeneration()を呼び出す必要がある
 */
UCLASS(Blueprintable, hidecategories = (Advanced, Physics, Collision, Volume, Brush, Mover, Will, BlockAll, CanBeBaseForCharacter))
class PPHKNAVRUNTIMEGEN_API APPHkNav_RuntimeNavMeshBoundsVolume : public AVolume
{
    GENERATED_BODY()

public:
    APPHkNav_RuntimeNavMeshBoundsVolume();

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    /**
     * @brief ベースNavMeshの生成を要求します
     * 複数回要求した場合、最後のベースNavMeshのみが有効になります
     */
    UFUNCTION(BlueprintCallable, Category = "PPHkNav|NavMesh")
    void RequestGeneration();

    /** 指定範囲でパッチNavMesh生成を要求. 床など個別パッチ用
     * @param PatchTransform
     * @param WorldPatchBox パッチ生成範囲 (ワールド空間)
     */
    UFUNCTION(BlueprintCallable, Category = "PPHkNav|NavMesh")
    void RequestPatchNavMeshGeneration(const FTransform& PatchTransform, const FBox& WorldPatchBox);

    /** 生成済みパッチNavMeshを GUID で削除 */
    UFUNCTION(BlueprintCallable, Category = "PPHkNav|NavMesh")
    bool RemovePatchNavMeshByGuid(const FGuid& PatchGuid);

    UFUNCTION(BlueprintCallable, Category = "PPHkNav|NavMesh")
    void RemoveAllPatchNavMesh();

private:
    /** NavMesh生成完了コールバック (bIsBaseNavMesh=true でベースNavMesh として扱う) */
    void OnNavMeshGenerationCompleted(const FHavokNavNavMeshGeneratorResult& Result, bool bIsBaseNavMesh, const FTransform& GenerationTransform);
    static void OnNavMeshRemoved(UHavokNavNavMeshInstanceSetComponent* Component, UHavokNavNavMesh* NavMeshBeingRemoved);

protected:
    /** NavMeshのインスタンスを保持するためのコンポーネント */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PPHkNav|NavMesh")
    TObjectPtr<UHavokNavNavMeshInstanceSetComponent> InstanceSetComponent{};

    /** 複数Patch NavMesh をインクリメンタル管理するためのコンポーネント */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PPHkNav|NavMesh")
    TObjectPtr<UPPHkNavNavMeshMultiPatchComponent> MultiPatchComponent{};

    /** NavMeshの所属するレイヤー */
    UPROPERTY(EditAnywhere, Category = "PPHkNav|NavMesh")
    TSubclassOf<UHavokNavNavMeshLayer> Layer{};

    /** 生成設定をオーバーライドするためのコントローラー */
    UPROPERTY(EditAnywhere, Category = "PPHkNav|NavMesh")
    FHavokNavNavMeshGenerationControllerOverride ControllerOverride{};
};
