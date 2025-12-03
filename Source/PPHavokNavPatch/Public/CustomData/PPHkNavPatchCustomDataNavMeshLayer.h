// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HavokNavNavMeshLayer.h"

#include "PPHkNavPatchCustomDataNavMeshLayer.generated.h"

struct FPPHkNavPatch_GroupIdFaceData;

/**
 * カスタムデータ付きNavMeshレイヤー
 */
UCLASS()
class PPHAVOKNAVPATCH_API UPPHkNavPatchCustomDataNavMeshLayer : public UHavokNavNavMeshLayer
{
	GENERATED_BODY()
public:
	using FFaceDataType = FPPHkNavPatch_GroupIdFaceData;
	
	UPPHkNavPatchCustomDataNavMeshLayer();
	
public:
	virtual FColor GetFaceDisplayColor(FHavokNavAnyConstRef FaceData) const override;

	// 主にEditor拡張用
	static FColor GetFaceDisplayColorStatic();
	static FColor GetEdgeDisplayColorStatic();
};
