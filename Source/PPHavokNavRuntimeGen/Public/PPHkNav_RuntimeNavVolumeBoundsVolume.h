// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPPHkNavVolumeBoundsActor.h"
#include "GameFramework/Volume.h"
#include "PPHkNavHavokNavVolumeGeneratorFactory.h"
#include "PPHkNav_RuntimeNavVolumeBoundsVolume.generated.h"

class UHavokNavNavVolumeLayer;
class UHavokNavNavVolumeInstanceSetComponent;
struct FHavokNavNavVolumeGeneratorResult;

/**
 * @class APPHkNav_RuntimeNavVolumeBoundsVolume
 * @brief ボリュームの範囲内に動的にNavVolumeを生成するアクター
 * エディタでボリュームの形状を編集し、ランタイムで指定された範囲内のNavVolumeを生成します。
 */
UCLASS(Blueprintable, hidecategories = (Advanced, Physics, Collision, Volume, Brush, Mover, Will, BlockAll, CanBeBaseForCharacter))
class PPHKNAVRUNTIMEGEN_API APPHkNav_RuntimeNavVolumeBoundsVolume : public AVolume, public IPPHkNavVolumeBoundsActor
{
    GENERATED_BODY()

public:
    /**
     * @brief コンストラクタ
     */
    APPHkNav_RuntimeNavVolumeBoundsVolume();

#if WITH_EDITOR
    //~ Begin AActor Interface
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    //~ End AActor Interface
#endif

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /**
     * @brief NavVolumeの生成を要求します
     */
    UFUNCTION(BlueprintCallable, Category = "PPHkNav|NavVolume")
    void RequestGeneration();

protected:
    /** NavVolumeのインスタンスを保持・表示するためのコンポーネントです */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PPHkNav|NavVolume")
    TObjectPtr<UHavokNavNavVolumeInstanceSetComponent> InstanceSetComponent{};

private:
    /**
     * @brief NavVolume生成が完了したときに呼び出されるコールバックです
     * @param Result 生成結果
     */
    void OnNavVolumeGenerationCompleted(const FHavokNavNavVolumeGeneratorResult& Result) const;

public:
    // IPPHkNavVolumeBoundsActor begin
    virtual FBox GetNavVolumeBounds() const override;
    virtual int GetPriority() const override {return 0;}
    virtual FGuid GetDominationGuid() const override;
    virtual bool IsInNavVolumeLayer(TSubclassOf<UHavokNavNavVolumeLayer> InLayer) const override;
    // IPPHkNavVolumeBoundsActor end

private:
    /** 生成するNavVolumeのレイヤーです */
    UPROPERTY(EditAnywhere, Category = "PPHkNav|NavVolume")
    TSubclassOf<UHavokNavNavVolumeLayer> Layer{};

    /** 生成設定をオーバーライドするためのコントローラーです */
    UPROPERTY(EditAnywhere, Category = "PPHkNav|NavVolume")
    FHavokNavNavVolumeGenerationControllerOverride ControllerOverride{};

    UPROPERTY(VisibleAnywhere)
    FGuid Guid = FGuid::NewGuid();
};
