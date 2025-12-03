// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "PPHkNavPatch_CustomData.generated.h"

/**
 * MassPalNav のカスタムデータの基底クラス
 * これを継承して各種データを定義すること
 * @note FInstanceStructの型絞り込みに使用する
 * @note もしくは TInstancedStruct<FPPHkNavPatch_CustomDataBase> で保持してよい
 */
USTRUCT()
struct PPHAVOKNAVPATCH_API FPPHkNavPatch_CustomDataBase
{
	GENERATED_BODY()
};


/**
 * @brief NavMeshフェースの通行可能状態を表す
 * 動的なペイント用のデータ
 */
UENUM(BlueprintType)
enum class EPPHkNavPatch_FaceTraversalState : uint8
{
	None,
	Traversable, // この面は通行可能
	Blocking,    // この面はブロック中ののため通行不可
};

/**
 * 
 */
USTRUCT()
struct PPHAVOKNAVPATCH_API FPPHkNavPatch_GroupIdFaceData : public FPPHkNavPatch_CustomDataBase
{
	GENERATED_BODY()

	// この面の所属するグループID (勢力圏)
	uint8 GroupId{};

	// この面の通行可能状態
	UPROPERTY(EditAnywhere, Category = "Dynamic Painting Data")
	EPPHkNavPatch_FaceTraversalState FaceTraversalState = EPPHkNavPatch_FaceTraversalState::None;

	bool operator==(const FPPHkNavPatch_GroupIdFaceData& Other) const
	{
		return GroupId == Other.GroupId && FaceTraversalState == Other.FaceTraversalState;
	}
};
