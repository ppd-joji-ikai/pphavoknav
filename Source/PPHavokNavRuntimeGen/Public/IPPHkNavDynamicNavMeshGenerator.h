// Copyright (c) 2024 Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HavokNavNavVolumeGenerationComponent.h"
#include "IPPHkNavVolumeBoundsActor.h"
#include "PPHkNavHavokNavMeshGeneratorFactory.h"
#include "UObject/Interface.h"

#include "IPPHkNavDynamicNavMeshGenerator.generated.h"

class UHavokNavNavMesh;
class UHavokNavNavMeshLayer;
class UHavokNavNavVolumeLayer;

/**
 * @brief ActorNavMesh生成リクエストのためのパラメータを格納する構造体
 */
USTRUCT(BlueprintType)
struct PPHKNAVRUNTIMEGEN_API FPPHkNavActorNavMeshGenerationParams
{
	GENERATED_BODY()

	/** NavMesh生成の基点となるアクター。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh Generation")
	TWeakObjectPtr<AActor> RequestingActor{};

	/** 生成するNavMeshのレイヤー。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh Generation")
	TSubclassOf<UHavokNavNavMeshLayer> Layer{};

	/** 生成設定をオーバーライドするためのコントローラー。 */
	UPROPERTY(EditAnywhere, Category = "NavMesh Generation")
	FHavokNavNavMeshGenerationControllerOverride ControllerOverride{};

	/**
	 * NavMesh生成完了時に呼び出されるコールバック。
	 * 生成に成功した場合はGeneratedMeshに有効なポインタが、失敗した場合はnullptrが渡されます。
	 */
	using FGenerationCompletedCallback = TFunction<void(const FHavokNavNavMeshGeneratorResult&)>;
	FGenerationCompletedCallback OnGeneratedCallback{};
};

/**
 * @brief BoxNavMesh生成リクエストのためのパラメータを格納する構造体
 */
USTRUCT(BlueprintType)
struct PPHKNAVRUNTIMEGEN_API FPPHkNavBoxNavMeshGenerationParams
{
	GENERATED_BODY()

	/** NavMesh生成の基点となるボックス。ワールド空間 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh Generation")
	FBox GenerationBox{};

	/** 生成するNavMeshのレイヤー。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh Generation")
	TSubclassOf<UHavokNavNavMeshLayer> Layer{};

	/** 生成コントローラのオーバーライド設定。 */
	UPROPERTY(EditAnywhere, Category = "NavMesh Generation")
	FHavokNavNavMeshGenerationControllerOverride ControllerOverride{};

	/** 生成設定のオーバーライド。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TObjectPtr<const UHavokNavNavMeshGenerationSettings> GenerationSettingsOverride = nullptr;
	
	/** 生成NavMeshの法線ベクトル */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh Generation")
	FVector UpVector = FVector::UpVector;

	/** 生成空間のトランスフォーム */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh Generation")
	FTransform GenerationTransform = FTransform::Identity;

	/**
	 * NavMesh生成完了時に呼び出されるコールバック。
	 * 生成に成功した場合はGeneratedMeshに有効なポインタが、失敗した場合はnullptrが渡されます。
	 */
	using FGenerationCompletedCallback = TFunction<void(const FHavokNavNavMeshGeneratorResult&)>;
	FGenerationCompletedCallback OnGeneratedCallback{};
};

/**
 * @brief BoxNavVolume生成リクエストのためのパラメータを格納する構造体
 */
USTRUCT(BlueprintType)
struct PPHKNAVRUNTIMEGEN_API FPPHkNavBoxNavVolumeGenerationParams
{
	GENERATED_BODY()

	/** NavVolume生成の基点となるボックス。GenerationTransformのローカル空間 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavVolume Generation")
	FBox GenerationBox{};

	/** 生成するNavVolumeのレイヤー。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavVolume Generation")
	TSubclassOf<UHavokNavNavVolumeLayer> Layer{};

	/** 生成設定をオーバーライドするためのコントローラー。 */
	UPROPERTY(EditAnywhere, Category = "NavVolume Generation")
	FHavokNavNavVolumeGenerationControllerOverride ControllerOverride{};
	
	/** 生成NavVolumeの法線ベクトル */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavVolume Generation")
	FVector UpVector = FVector::UpVector;

	/** 生成空間のトランスフォーム */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavVolume Generation")
	FTransform GenerationTransform = FTransform::Identity;

	/** 生成するAVolumeなActor */
	TWeakInterfacePtr<IPPHkNavVolumeBoundsActor> NavVolumeBoundsActor{};

	/**
	 * NavVolume生成完了時に呼び出されるコールバック。
	 * 生成に成功した場合はGeneratedVolumeに有効なポインタが、失敗した場合はnullptrが渡されます。
	 */
	using FGenerationCompletedCallback = TFunction<void(const FHavokNavNavVolumeGeneratorResult&)>;
	FGenerationCompletedCallback OnGeneratedCallback{};
};

UINTERFACE(MinimalAPI)
class UPPHkNavDynamicNavMeshGenerator : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief 動的NavMeshジェネレーター
 */
class PPHKNAVRUNTIMEGEN_API IPPHkNavDynamicNavMeshGenerator
{
	GENERATED_BODY()

public:
	/**
	 * @brief アクターNavMeshの生成を要求する
	 */
	virtual void RequestActorNavMeshGeneration(FPPHkNavActorNavMeshGenerationParams&& RequestParams) = 0;

	/**
	 * @brief ボックスNavMeshの生成を要求する
	 */
	virtual void RequestBoxNavMeshGeneration(FPPHkNavBoxNavMeshGenerationParams&& RequestParams) = 0;

	/**
	 * @brief ボックスNavVolumeの生成を要求する
	 */
	virtual void RequestBoxNavVolumeGeneration(FPPHkNavBoxNavVolumeGenerationParams&& RequestParams) = 0;
};
