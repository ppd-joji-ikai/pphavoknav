// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HavokNavNavMeshDynamicCuttingController.h"
#include "HavokNavNavMeshSilhouetteGenerator.h"
#include "PPHkNavPatch_CustomData.h"
#include "PPHkNavPatchCustomNavMeshSilhouetteGenerator.generated.h"

/**
 * @brief カスタムデータに対応したNavMeshのシルエットジェネレーター
 * FPPHkNavPatchSilhouetteGenerationSettingsFragment::SilhouetteGeneratorClass にセットする
 * @see FPPHkNavPatchSilhouetteGenerationSettingsFragment::SilhouetteGeneratorClass
 * @see FPPHkNavPatchGroupIdFaceData
 */
UCLASS()
class PPHAVOKNAVPATCH_API UPPHkNavPatchCustomNavMeshSilhouetteGenerator : public UHavokNavNavMeshSilhouetteGenerator
{
	GENERATED_BODY()
public:
	using FCustomFaceDataType = FPPHkNavPatch_GroupIdFaceData;

public:
	// @brief シルエットのペインティングデータを設定する
	// @note SetEnabled(true)で更新しないと反映されない
	// このデータはHavokスレッドからアクセスされうるので排他制御に気を付けること
	void SetSilhouettePaintingData(const FCustomFaceDataType& InFaceData);
	
	const FCustomFaceDataType& GetSilhouettePaintingData() const { return FaceData; }

	friend class UPPHkNavPatchCustomNavMeshDynamicCuttingController;
	
protected:
	UPROPERTY()
	FPPHkNavPatch_GroupIdFaceData FaceData{};
};

/**
 * @brief NavMeshの動的ペインティング対応コントローラ
 * UHavokNavNavMeshLayer::DynamicCuttingControllerにセットする
 * @see UHavokNavNavMeshLayer::DynamicCuttingController
 * @see FPPHkNavPatch_GroupIdFaceData
 */
UCLASS()
class PPHAVOKNAVPATCH_API UPPHkNavPatchCustomNavMeshDynamicCuttingController : public UHavokNavNavMeshDynamicCuttingController
{
	GENERATED_BODY()
public:
	using FCustomFaceDataType = FPPHkNavPatch_GroupIdFaceData;

public:
	virtual EHavokNavNavMeshSilhouetteEffect ProcessFace(
		FHavokNavAnyConstRef OriginalFaceData,
		TArrayView<UHavokNavNavMeshSilhouetteGenerator const*> SilhouetteGenerators,
		FHavokNavAnyRef FaceDataOut) const override;
};