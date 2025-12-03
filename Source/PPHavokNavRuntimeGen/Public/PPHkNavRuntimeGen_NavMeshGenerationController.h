// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IHavokNavNavMeshGenerationController.h"
#include "PPHkNavRuntimeGen_NavMeshGenerationController.generated.h"


/**
 * PPHkNav NavMesh生成コントローラーインターフェースのベースクラス
 */
UCLASS(Abstract)
class PPHKNAVRUNTIMEGEN_API UPPHkNavRuntimeGen_NavMeshGenerationControllerBase : public UObject, public IHavokNavNavMeshGenerationController
{
	GENERATED_BODY()

public:
	/// begin IHavokNavNavMeshGenerationController interface
	virtual UScriptStruct* GetPhysicalObjectDataType() const override;
	virtual UScriptStruct* GetFaceDataType() const override;
	virtual FProcessPhysicalObjectResult ProcessPrimitiveComponent(const UPrimitiveComponent* Component, FHavokNavAnyRef DataOut) const override PURE_VIRTUAL(, return {};);
	virtual FProcessPhysicalObjectResult ProcessModifierVolume(const AHavokNavModifierVolume* Volume, FHavokNavAnyRef DataOut) const override;
	virtual EHavokNavNavMeshGenerationFaceUsage ProcessFace(FHavokNavAnyConstRef SurfaceData, TArrayView<FHavokNavAnyConstRef> VolumeDatas, FHavokNavAnyRef FaceDataOut) const override;
	/// end IHavokNavNavMeshGenerationController interface
};

/**
 * シンプル実装
 * 主にレベルジオメトリをNavMeshに変換する用途を想定
 */
UCLASS(DisplayName="PPHkNav NavMesh Generation Controller Simple")
class PPHKNAVRUNTIMEGEN_API UPPHkNavRuntimeGen_NavMeshGenerationController : public UPPHkNavRuntimeGen_NavMeshGenerationControllerBase
{
	GENERATED_BODY()

public:
	/// begin IHavokNavNavMeshGenerationController interface
	virtual FProcessPhysicalObjectResult ProcessPrimitiveComponent(const UPrimitiveComponent* Component, FHavokNavAnyRef DataOut) const override;
	/// end IHavokNavNavMeshGenerationController interface
};


/**
 * CanEverAffectNavigation() が true のPrimitiveComponentを歩行可能サーフェスとして扱う実装
 * それ以外のパラメータはみない. NoCollision推奨
 */
UCLASS(DisplayName="PPHkNav NavMesh Generation Controller CanEverAffectNav")
class PPHKNAVRUNTIMEGEN_API UPPHkNavRuntimeGen_NavMeshGenerationControllerCanEverAffectNav : public UPPHkNavRuntimeGen_NavMeshGenerationControllerBase
{
	GENERATED_BODY()

public:
	/// begin IHavokNavNavMeshGenerationController interface
	virtual FProcessPhysicalObjectResult ProcessPrimitiveComponent(const UPrimitiveComponent* Component, FHavokNavAnyRef DataOut) const override;
	/// end IHavokNavNavMeshGenerationController interface
};