// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HavokNavNavMeshGeneratorResult.h"
#include "HavokNavNavMeshGenerationComponent.h" // For FHavokNavNavMeshGenerationControllerOverride
#include "Templates/UniquePtr.h"
#include "Templates/Function.h"
#include "WorldPartition/WorldPartitionActorLoaderInterface.h"
#include "PPHkNavHavokNavMeshGeneratorFactory.generated.h"

class UHavokNavNavMesh;
class UHavokNavClusterGraph;
class UHavokNavNavMeshLayer;
class UHavokNavNavMeshGenerator;
class UHavokNavNavMeshGenerationSettings;
class IHavokNavNavMeshGenerationBoundsProvider;
class IHavokNavNavMeshGenerationInputEntityGatherer;
class UWorld;

/**
 * @struct FPPHkNavMeshGenerationParams
 * @brief Parameters for navmesh generation.
 * This struct consolidates all inputs required for a generation task.
 */
USTRUCT(BlueprintType)
struct PPHAVOKNAVRUNTIMEGEN_API FPPHkNavMeshGenerationParams
{
	GENERATED_BODY()

	/** The world in which to generate the navmesh. */
	UPROPERTY()
	TObjectPtr<UWorld> World = nullptr;

	/** The layer for which to generate the navmesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TSubclassOf<UHavokNavNavMeshLayer> Layer = nullptr;

	/** Optional override for the generation controller. */
	UPROPERTY(EditAnywhere, Category = "Generation")
	FHavokNavNavMeshGenerationControllerOverride ControllerOverride{};

	/** Optional override for generation settings. If null, project defaults are used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TObjectPtr<const UHavokNavNavMeshGenerationSettings> GenerationSettingsOverride = nullptr;

	/** The provider for the generation bounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TScriptInterface<IHavokNavNavMeshGenerationBoundsProvider> BoundsProvider = nullptr;

	/** The transform for the generated navmesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FTransform GenerationTransform = FTransform::Identity;

	/** The up vector for generation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FVector UpVector = FVector::ZAxisVector;

	/** A factory function to create the input entity gatherer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TScriptInterface<IHavokNavNavMeshGenerationInputEntityGatherer> InputEntityGatherer = nullptr;

#if WITH_EDITOR
	/** A factory function to create the world partition loader adapter. */
	TFunction<TUniquePtr<IWorldPartitionActorLoaderInterface::ILoaderAdapter>(TSubclassOf<UHavokNavNavMeshLayer>, const UHavokNavNavMeshGenerationSettings*)> LoaderAdapterFactory{};
#endif

	/** Debug string for logging. */
	FString DebugInfo{};

	/** Callback executed when this specific generation request is completed. */
	TFunction<void(const FHavokNavNavMeshGeneratorResult&)> OnGenerationCompletedCallback;
};

/**
 * @class UPPHkNavHavokNavMeshGeneratorFactory
 * @brief A factory class for creating UHavokNavNavMeshGenerator instances.
 * This class is stateless and provides static methods to create generators
 * based on the provided parameters.
 */
UCLASS()
class PPHAVOKNAVRUNTIMEGEN_API UPPHkNavHavokNavMeshGeneratorFactory : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief Creates a new UHavokNavNavMeshGenerator instance.
	 * @param Params The parameters for the generation task.
	 * @return A new generator instance, or nullptr if creation failed.
	 */
	static UHavokNavNavMeshGenerator* CreateGenerator(const FPPHkNavMeshGenerationParams& Params);
	
	static const UHavokNavNavMeshGenerationSettings* GetEffectiveNavMeshGenerationSettings(const FPPHkNavMeshGenerationParams& Params);
	static const UClass* GetEffectiveNavMeshGenerationControllerClass(const FPPHkNavMeshGenerationParams& Params);
};

