// Copyright Pocketpair, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "PPHkNavPyramidNavMeshAuthoringComponent_EditorOnly.generated.h"

namespace Pp
{
	struct FHavokNavPyramidGeometryParams;
}

/**
 * Pyramid (四角錐) NavMesh オーサリング用エディタ専用コンポーネント
 * - 原点は底面中心 (Z=0)
 * - 頂点(Apex) は +Z(Height) 方向へ配置 (ApexOffsetXYCentimeters により XY オフセット可能)
 */
UCLASS(ClassGroup=(PPHkNav), meta=(BlueprintSpawnableComponent, DisplayName="PPHkNav Pyramid NavMesh Authoring (Editor Only)"))
class PPHAVOKNAVRUNTIMEGEN_API UPPHkNavPyramidNavMeshAuthoringComponent_EditorOnly : public UBoxComponent
{
	GENERATED_BODY()
public:
	explicit UPPHkNavPyramidNavMeshAuthoringComponent_EditorOnly(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Nav 生成用パラメータ計算 */
	void CalcPyramidGeometryParams(Pp::FHavokNavPyramidGeometryParams& OutParams) const;

	// ---- UPrimitiveComponent overrides ----
public:
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

#if WITH_EDITOR
protected:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	// 底面幅[cm] (Y方向 全体長)
	UPROPERTY(EditDefaultsOnly, Category="Pyramid", meta=(ClampMin="0.0"))
	float WidthCentimeters{400.f};
	// 底面奥行[cm] (X方向 全体長)
	UPROPERTY(EditDefaultsOnly, Category="Pyramid", meta=(ClampMin="0.0"))
	float DepthCentimeters{400.f};
	// 高さ[cm] (+Z 方向, 負値で逆さピラミッド)
	UPROPERTY(EditDefaultsOnly, Category="Pyramid")
	float HeightCentimeters{162.5f};
	// Apex ローカル XY オフセット[cm]
	UPROPERTY(EditDefaultsOnly, Category="Pyramid")
	FVector2D ApexOffsetXYCentimeters{FVector2D::ZeroVector};
	// NavMesh Up ベクトル (法線補正/押し出し方向推定)
	UPROPERTY(EditDefaultsOnly, Category="Pyramid")
	FVector NavMeshUpVector{FVector::UpVector};

	// Erosion 対策 (外側拡張) を行うか
	UPROPERTY(EditDefaultsOnly, Category="Pyramid")
	bool bAntiErosionExpand{ true };
	// Erosion 対策 拡張量[cm]
	UPROPERTY(EditDefaultsOnly, Category="Pyramid", meta=(EditCondition="bAntiErosionExpand", ClampMin="0.0"))
	float AntiErosionRadius{ 50.0f };

	// ビジュアル表示カラー
	UPROPERTY(EditDefaultsOnly, Category="Visual")
	FColor BaseColor{FColor::Green};
	UPROPERTY(EditDefaultsOnly, Category="Visual")
	FColor EdgeColor{FColor::Cyan};
	UPROPERTY(EditDefaultsOnly, Category="Visual")
	FColor ApexColor{FColor::Red};
	UPROPERTY(EditDefaultsOnly, Category="Visual")
	FColor NormalColor{FColor::Yellow};
};
