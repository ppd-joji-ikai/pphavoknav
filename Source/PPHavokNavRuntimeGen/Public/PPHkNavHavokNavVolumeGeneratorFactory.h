// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HavokNavNavVolumeGeneratorResult.h"
#include "HavokNavNavVolumeGenerationComponent.h" // For FHavokNavNavVolumeGenerationControllerOverride
#include "Templates/UniquePtr.h"
#include "Templates/Function.h"
#include "WorldPartition/WorldPartitionActorLoaderInterface.h"
#include "PPHkNavHavokNavVolumeGeneratorFactory.generated.h"

class UHavokNavNavVolume;
class UHavokNavNavVolumeLayer;
class UHavokNavNavVolumeGenerator;
class UHavokNavNavVolumeGenerationSettings;
class IHavokNavNavVolumeGenerationBoundsProvider;
class IHavokNavNavVolumeGenerationInputEntityGatherer;
class UWorld;

/**
 * @struct FPPHkNavVolumeGenerationParams
 * @brief Parameters for navvolume generation.
 * This struct consolidates all inputs required for a generation task.
 */
USTRUCT(BlueprintType)
struct PPHAVOKNAVRUNTIMEGEN_API FPPHkNavVolumeGenerationParams
{
	GENERATED_BODY()

	/** The world in which to generate the navvolume. */
	UPROPERTY()
	TObjectPtr<UWorld> World = nullptr;

	/** The layer for which to generate the navvolume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TSubclassOf<UHavokNavNavVolumeLayer> Layer = nullptr;

	/** Optional override for the generation controller. */
	UPROPERTY(EditAnywhere, Category = "Generation")
	FHavokNavNavVolumeGenerationControllerOverride ControllerOverride{};

	/** Optional override for generation settings. If null, project defaults are used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TObjectPtr<const UHavokNavNavVolumeGenerationSettings> GenerationSettingsOverride = nullptr;

	/** The provider for the generation bounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TScriptInterface<IHavokNavNavVolumeGenerationBoundsProvider> BoundsProvider = nullptr;

	/** The transform for the generated navvolume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FHavokNavAxialTransform GenerationTransform = FHavokNavAxialTransform::Identity;

	/** A factory function to create the input entity gatherer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TScriptInterface<IHavokNavNavVolumeGenerationInputEntityGatherer> InputEntityGatherer = nullptr;

#if WITH_EDITOR
	/** A factory function to create the world partition loader adapter. */
	TFunction<TUniquePtr<IWorldPartitionActorLoaderInterface::ILoaderAdapter>(TSubclassOf<UHavokNavNavVolumeLayer>, const UHavokNavNavVolumeGenerationSettings*)> LoaderAdapterFactory{};
#endif

	/** Debug string for logging. */
	FString DebugInfo{};

	/** Callback executed when this specific generation request is completed. */
	TFunction<void(const FHavokNavNavVolumeGeneratorResult&)> OnGenerationCompletedCallback;
};

/**
 * @class UPPHkNavHavokNavVolumeGeneratorFactory
 * @brief A factory class for creating UHavokNavNavVolumeGenerator instances.
 * This class is stateless and provides static methods to create generators
 * based on the provided parameters.
 */
UCLASS()
class PPHAVOKNAVRUNTIMEGEN_API UPPHkNavHavokNavVolumeGeneratorFactory : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief Creates a new UHavokNavNavVolumeGenerator instance.
	 * @param Params The parameters for the generation task.
	 * @return A new generator instance, or nullptr if creation failed.
	 */
	static UHavokNavNavVolumeGenerator* CreateGenerator(const FPPHkNavVolumeGenerationParams& Params);
	
	static const UHavokNavNavVolumeGenerationSettings* GetEffectiveNavVolumeGenerationSettings(const FPPHkNavVolumeGenerationParams& Params);
	static const UClass* GetEffectiveNavVolumeGenerationControllerClass(const FPPHkNavVolumeGenerationParams& Params);
};
