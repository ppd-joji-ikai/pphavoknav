// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PPHkNavHavokNavMeshGeneratorFactory.h"
#include "PPHkNav_DynamicNavMeshGenTargetComponent.generated.h"


class IHavokNavGenerationQueueMember;
class UHavokNavNavMeshLayer;
class UHavokNavNavMeshInstanceSetComponent;

/**
 * @class UPPHkNav_DynamicNavMeshGenTargetComponent
 * @brief このコンポーネントがアタッチされたアクターが、動的NavMesh生成の対象であることを示します。
 * アクターの生成時と破壊時に UPPHkNav_DynamicNavMeshSubsystem へ通知を行います。
 * アクターの移動にNavMeshを追従させるために適切な階層にAttachmentする必要があります。
 * @see UPPHkNav_DynamicNavMeshSubsystem
 */
UCLASS(ClassGroup=(MassPal), meta=(BlueprintSpawnableComponent))
class PPHKNAVRUNTIMEGEN_API UPPHkNav_DynamicNavMeshGenTargetComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UPPHkNav_DynamicNavMeshGenTargetComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "PPHkNav|NavMesh")
	void RequestGeneration();
	
private:
	void OnNavMeshGenerationCompleted(const FHavokNavNavMeshGeneratorResult& Result);

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UHavokNavNavMeshInstanceSetComponent> InstanceSetComponent;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UHavokNavNavMeshLayer> Layer;

	UPROPERTY(EditAnywhere)
	FHavokNavNavMeshGenerationControllerOverride Override;
};
