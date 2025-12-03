// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPPHkNavDynamicNavMeshGenerator.h"
#include "UObject/Object.h"
#include "PPHkNav_DynamicNavMeshManager.generated.h"

class UPPHkNav_RuntimeGenerationSubsystem;
/**
 * @brief 動的NavMesh管理オブジェクト
 */
UCLASS()
class PPHAVOKNAVRUNTIMEGEN_API UPPHkNav_DynamicNavMeshManager : public UObject, public IPPHkNavDynamicNavMeshGenerator
{
	GENERATED_BODY()

public:
	/**
	 * @brief Initializes the manager with the world context.
	 * @param InWorld The world this manager belongs to.
	 * @param InRuntimeGenerationSubsystem The subsystem to use for generation requests.
	 */
	void Initialize(UWorld* InWorld, UPPHkNav_RuntimeGenerationSubsystem* InRuntimeGenerationSubsystem);
	
	/** Cleans up the manager. */
	void Deinitialize();

public:
	//IPPHkNavDynamicNavMeshGenerator
	virtual void RequestActorNavMeshGeneration(FPPHkNavActorNavMeshGenerationParams&& RequestParams) override;
	virtual void RequestBoxNavMeshGeneration(FPPHkNavBoxNavMeshGenerationParams&& RequestParams) override;
	/**
	 * @brief ボックスNavVolumeの生成を要求する
	 */
	virtual void RequestBoxNavVolumeGeneration(FPPHkNavBoxNavVolumeGenerationParams&& RequestParams) override;

	// UObject interface
	virtual UWorld* GetWorld() const override;


private:
	UPROPERTY()
	TObjectPtr<UWorld> World;

	UPROPERTY()
	TObjectPtr<UPPHkNav_RuntimeGenerationSubsystem> RuntimeGenerationSubsystem;
};
