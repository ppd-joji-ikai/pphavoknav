// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "IHavokNavNavMeshGenerationInputEntityGatherer.h"
#include "PPHkNav_ActorInputEntityGatherer.generated.h"

class AActor;
struct FHavokNavNavMeshGenerationInput;

/**
 * @brief 特定のアクターからNavMesh生成のための入力ジオメトリを収集するクラス
 */
UCLASS()
class PPHAVOKNAVRUNTIMEGEN_API UPPHkNav_ActorInputEntityGatherer : public UObject, public IHavokNavNavMeshGenerationInputEntityGatherer
{
    GENERATED_BODY()

public:
    void Initialize(const TObjectPtr<AActor>& InActor)
    {
        SetActor(InActor.Get());
    }
    
    /**
     * @brief ジオメトリ収集の対象となるアクターを設定します。
     * @param InActor 対象のアクター
     */
    void SetActor(const TObjectPtr<AActor>& InActor)
    {
        TargetActor = InActor;
    }

    // IHavokNavNavMeshGenerationInputEntityGatherer interface
    virtual FHavokNavNavMeshGenerationInputEntitySet GatherInputEntities(
        TSubclassOf<UHavokNavNavMeshLayer> Layer,
        FTransform const& GenerationTransform,
        FHavokNavNavMeshGenerationBounds const& GenerationLocalBounds,
        TScriptInterface<IHavokNavNavMeshGenerationController> Controller,
        float InputGatheringBoundsExpansion) const override;

private:
    UPROPERTY()
    TWeakObjectPtr<AActor> TargetActor;
};