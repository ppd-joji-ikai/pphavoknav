// Copyright Pocketpair, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "PPHkNavTriangleNavMeshAuthoringComponent_EditorOnly.generated.h"

namespace Pp
{
	struct FHavokNavTriangleGeometryParams;
}

/**
 * Triangle NavMesh オーサリング用エディタ専用コンポーネント
 */
UCLASS(ClassGroup=(PPHkNav), meta=(BlueprintSpawnableComponent, DisplayName="PPHkNav Triangle NavMesh Authoring (Editor Only)"))
class PPHAVOKNAVRUNTIMEGEN_API UPPHkNavTriangleNavMeshAuthoringComponent_EditorOnly : public UBoxComponent
{
	GENERATED_BODY()
public:
	explicit UPPHkNavTriangleNavMeshAuthoringComponent_EditorOnly(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Nav 生成用パラメータ計算 */
	void CalcTriangleGeometryParams(Pp::FHavokNavTriangleGeometryParams& OutParams) const;

	// ---- UPrimitiveComponent overrides ----
public:
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

#if WITH_EDITOR
protected:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	// 三角形ローカル頂点
	UPROPERTY(EditDefaultsOnly, Category="Triangle", meta=(MakeEditWidget=true))
	FVector VertexA{ FVector::ZeroVector };
	UPROPERTY(EditDefaultsOnly, Category="Triangle", meta=(MakeEditWidget=true))
	FVector VertexB{ FVector(200.f, 0.f, 0.f) };
	UPROPERTY(EditDefaultsOnly, Category="Triangle", meta=(MakeEditWidget=true))
	FVector VertexC{ FVector(0.f, 200.f, 0.f) };

	// NavMesh Up ベクトル
	UPROPERTY(EditDefaultsOnly, Category="Triangle")
	FVector NavMeshUpVector{ FVector::UpVector };

	// Erosion対策の拡張するか
	UPROPERTY(EditDefaultsOnly, Category="Triangle")
	bool bAntiErosionExpand{ true };

	// Erosion対策の拡張量[cm]
	UPROPERTY(EditDefaultsOnly, Category="Triangle", meta=(EditCondition="bAntiErosionExpand", ClampMin="0.0"))
	float AntiErosionRadius{ 50.0f };
	
	// プリズム生成
	UPROPERTY(EditDefaultsOnly, Category="Triangle")
	bool bGeneratePrism{ false };

	// プリズム高さ[cm]
	UPROPERTY(EditDefaultsOnly, Category="Triangle", meta=(EditCondition="bGeneratePrism", ClampMin="0.0"))
	float PrismHeight{ 100.0f };

	// エディタ表示カラー (必要なら BP から調整)
	UPROPERTY(EditDefaultsOnly, Category="Visual")
	FColor TriangleColor{ FColor::Green };
	UPROPERTY(EditDefaultsOnly, Category="Visual")
	FColor PrismColor{ FColor::Cyan };
	UPROPERTY(EditDefaultsOnly, Category="Visual")
	FColor NormalColor{ FColor::Yellow };
	UPROPERTY(EditDefaultsOnly, Category="Visual")
	FColor VertexColor{ FColor::Red };
};
