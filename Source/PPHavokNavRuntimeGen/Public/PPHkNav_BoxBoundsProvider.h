// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "IHavokNavNavMeshGenerationBoundsProvider.h"
#include "PPHkNav_BoxBoundsProvider.generated.h"

class UHavokNavNavMeshLayer;

/**
 * @brief 指定されたボックス領域をNavMesh生成範囲として提供するクラス
 * 静的に定義されたボックスに基づいてNavMeshを生成するのに適しています
 */
UCLASS()
class PPHAVOKNAVRUNTIMEGEN_API UPPHkNav_BoxBoundsProvider : public UObject, public IHavokNavNavMeshGenerationBoundsProvider
{
    GENERATED_BODY()

public:
    /**
     * @brief 初期化する
     * @param InBox 生成範囲となるボックス（ワールド空間）
     * @param InLayer 生成対象レイヤー
     * @param InUpVector 生成NavMeshの法線ベクトル
     */
    void Initialize(const FBox& InBox, const TSubclassOf<UHavokNavNavMeshLayer>& InLayer, const FVector& InUpVector)
    {
        BoundingBox = InBox;
        TargetLayer = InLayer;
        UpVector = InUpVector.GetSafeNormal();
        if (UpVector.IsZero())
        {
            UE_LOG(LogTemp, Warning,
                   TEXT("UPPHkNav_BoxBoundsProvider: UpVector cannot be zero. Using default UpVector."));
            UpVector = FVector::UpVector;
        }
    }
    
    // IHavokNavNavMeshGenerationBoundsProvider interface
    virtual FHavokNavNavMeshGenerationBounds GetBounds(TSubclassOf<UHavokNavNavMeshLayer> Layer, FTransform const& GenerationTransform) const override;

private:
    // NavMeshを生成する境界ボックス（ワールド空間）
    UPROPERTY(Transient)
    FBox BoundingBox;

    // NavMeshを生成する対象レイヤー
    UPROPERTY(Transient)
    TSubclassOf<UHavokNavNavMeshLayer> TargetLayer;

    // NavMesh面の法線ベクトル
    UPROPERTY()
    FVector UpVector = FVector::UpVector;
};
