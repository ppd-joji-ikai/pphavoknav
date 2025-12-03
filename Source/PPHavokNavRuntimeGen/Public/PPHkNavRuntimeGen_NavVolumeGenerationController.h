// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IHavokNavNavVolumeGenerationController.h"
#include "PPHkNavRuntimeGen_NavVolumeGenerationController.generated.h"

class UPrimitiveComponent;
class UStaticMeshComponent;
class AStaticMeshActor;

/**
 * MassPal向けの拡張されたHavokナビゲーションボリューム生成コントローラー
 * 動的にスポーンしたStaticMeshやアクターを適切にBlockingSurfaceとして処理します
 */
UCLASS(meta = (DisplayName = "PPHkNav NavVolume Generation Controller"))
class PPHKNAVRUNTIMEGEN_API UPPHkNavRuntimeGen_NavVolumeGenerationController : public UObject, public IHavokNavNavVolumeGenerationController
{
	GENERATED_BODY()

public:
	UPPHkNavRuntimeGen_NavVolumeGenerationController();

	//~ Begin IHavokNavNavVolumeGenerationController Interface
	virtual UScriptStruct* GetCellDataType() const override { return nullptr; }
	virtual FProcessPhysicalObjectResult ProcessPrimitiveComponent(const UPrimitiveComponent* Component, FHavokNavAnyRef DataOut, int& ResolutionMultiplierOut) const override;
	virtual FProcessPhysicalObjectResult ProcessModifierVolume(const AHavokNavModifierVolume* Volume, FHavokNavAnyRef DataOut, int& ResolutionMultiplierOut) const override;
	virtual void GetDefaultCellData(FHavokNavAnyRef DataOut) const override {}
	//~ End IHavokNavNavVolumeGenerationController Interface

protected:
	/**
	 * 動的オブジェクトをナビゲーション障害物として処理するかどうか
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassPal Navigation", meta = (DisplayName = "Process Dynamic Objects"))
	bool bProcessDynamicObjects = true;

	/**
	 * 最小サイズ閾値 - この値より小さいオブジェクトは無視される [cm]
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassPal Navigation", meta = (DisplayName = "Minimum Size Threshold", Units = "cm"))
	float MinimumSizeThresholdCm = 50.0f;

	/**
	 * Movableなオブジェクトも処理対象とするかどうか
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassPal Navigation", meta = (DisplayName = "Process Movable Objects"))
	bool bProcessMovableObjects = true;

private:
	/**
	 * コンポーネントが動的オブジェクトかどうかを判定
	 * @param Component 判定対象のプリミティブコンポーネント
	 * @return 動的オブジェクトの場合true
	 */
	bool IsDynamicObject(const UPrimitiveComponent* Component) const;

	/**
	 * コンポーネントがナビゲーション障害物として処理すべきサイズかどうかを判定
	 * @param Component 判定対象のプリミティブコンポーネント
	 * @return 処理すべきサイズの場合true
	 */
	bool IsValidSizeForNavigation(const UPrimitiveComponent* Component) const;

	/**
	 * オブジェクトがブロッキングサーフェスとして処理すべきかどうかを判定
	 * @param Component 判定対象のプリミティブコンポーネント
	 * @return BlockingSurfaceとして処理すべき場合true
	 */
	bool ShouldProcessAsBlockingSurface(const UPrimitiveComponent* Component) const;
};
