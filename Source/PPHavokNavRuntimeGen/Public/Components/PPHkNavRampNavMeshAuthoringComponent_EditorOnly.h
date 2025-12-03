// Copyright Pocketpair, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "PPHkNavRampNavMeshAuthoringComponent_EditorOnly.generated.h"

namespace Pp
{
	struct FHavokNavRampGeometryParams;
}

/**
 * Ramp NavMesh オーサリング用エディタ専用 Box コンポーネント
 * 原点はBox中心
 * 前進方向 +Forward 側の +Length/2 が高端 (Z=Height)
 * 長さ/幅は BoxExtent から算出 (Length = 2*Extent.X, Width = 2*Extent.Y)
 */
UCLASS(ClassGroup=(PPHkNav), meta=(BlueprintSpawnableComponent, DisplayName="PPHkNav Ramp NavMesh Authoring (Editor Only)"))
class PPHAVOKNAVRUNTIMEGEN_API UPPHkNavRampNavMeshAuthoringComponent_EditorOnly : public UBoxComponent
{
	GENERATED_BODY()

public:
	UPPHkNavRampNavMeshAuthoringComponent_EditorOnly();

public:
	/** Ramp 用パラメータ計算 */
	void CalcRampGeometryParams(Pp::FHavokNavRampGeometryParams& OutParams) const;

public:
	// UPrimitiveComponent overrides
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

#if WITH_EDITOR
protected:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	// Erosion 対策 (外側拡張) を行うか
	UPROPERTY(EditDefaultsOnly, Category="Ramp")
	bool bAntiErosionExpand{ true };
	// Erosion 対策 拡張量[cm]
	UPROPERTY(EditDefaultsOnly, Category="Ramp", meta=(EditCondition="bAntiErosionExpand", ClampMin="0.0"))
	float AntiErosionRadius{ 50.0f };

	// 可視化カラー (Ramp輪郭)
	UPROPERTY(EditDefaultsOnly, Category="Ramp")
	FColor RampColor{ FColor::Orange };
	// 法線線分カラー
	UPROPERTY(EditDefaultsOnly, Category="Ramp")
	FColor NormalColor{ FColor::Yellow };
};
