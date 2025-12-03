// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "IHavokNavNavVolumeGenerationBoundsProvider.h"
#include "PPHkNav_BoxVolumeBoundsProvider.generated.h"

class IPPHkNavVolumeBoundsActor;
class UHavokNavNavVolumeLayer;

/**
 * @brief 指定されたボックス領域をNavVolume生成範囲として提供するクラス
 * 静的に定義されたボックスに基づいてNavVolumeを生成するのに適しています
 */
UCLASS()
class PPHAVOKNAVRUNTIMEGEN_API UPPHkNav_BoxVolumeBoundsProvider : public UObject, public IHavokNavNavVolumeGenerationBoundsProvider
{
    GENERATED_BODY()

public:
    /**
     * @brief 初期化する
     * @param InBox 生成範囲となるボックス（ワールド空間）
     * @param InLayer 生成対象レイヤー
     * @param InNavVolumeBoundsActor
     */
    void Initialize(const FBox& InBox, const TSubclassOf<UHavokNavNavVolumeLayer>& InLayer, const TWeakInterfacePtr<IPPHkNavVolumeBoundsActor>& InNavVolumeBoundsActor)
    {
        BoundingBox = InBox;
        TargetLayer = InLayer;
        NavVolumeBoundsActor = InNavVolumeBoundsActor;
    }
    
    // IHavokNavNavVolumeGenerationBoundsProvider interface
    virtual FHavokNavNavVolumeGenerationBounds GetBounds(TSubclassOf<UHavokNavNavVolumeLayer> Layer, FHavokNavAxialTransform const& GenerationTransform) const override;

private:
    // NavVolumeを生成する境界ボックス（ワールド空間）
    UPROPERTY(Transient)
    FBox BoundingBox;

    // NavVolumeを生成する対象レイヤー
    UPROPERTY(Transient)
    TSubclassOf<UHavokNavNavVolumeLayer> TargetLayer;

    // NavVolumeを生成する境界ボックスActor
    TWeakInterfacePtr<IPPHkNavVolumeBoundsActor> NavVolumeBoundsActor;
};
