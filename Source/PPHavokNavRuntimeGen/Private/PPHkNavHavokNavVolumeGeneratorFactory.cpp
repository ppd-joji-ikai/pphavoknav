// Copyright Pocketpair, Inc. All Rights Reserved.

#include "PPHkNavHavokNavVolumeGeneratorFactory.h"
#include "HavokNavNavVolumeGenerator.h"
#include "HavokNavNavVolumeLayer.h"
#include "HavokNavNavVolumeGenerationSettings.h"
#include "HavokNavDefaultGenerationSettings.h"
#include "IHavokNavNavVolumeGenerationController.h"
#include "Engine/World.h"

UHavokNavNavVolumeGenerator* UPPHkNavHavokNavVolumeGeneratorFactory::CreateGenerator(const FPPHkNavVolumeGenerationParams& Params)
{
	if (!Params.World)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPPHkNavHavokNavVolumeGeneratorFactory::CreateGenerator: World is null"));
		return nullptr;
	}

	if (!Params.Layer)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPPHkNavHavokNavVolumeGeneratorFactory::CreateGenerator: Layer is null"));
		return nullptr;
	}

	if (!Params.BoundsProvider)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPPHkNavHavokNavVolumeGeneratorFactory::CreateGenerator: BoundsProvider is null"));
		return nullptr;
	}

	if (!Params.InputEntityGatherer)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPPHkNavHavokNavVolumeGeneratorFactory::CreateGenerator: InputEntityGatherer is null"));
		return nullptr;
	}

	const UHavokNavNavVolumeGenerationSettings* Settings = GetEffectiveNavVolumeGenerationSettings(Params);
	if (!Settings)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPPHkNavHavokNavVolumeGeneratorFactory::CreateGenerator: No valid NavVolumeGenerationSettings found"));
		return nullptr;
	}

	const UClass* GenerationControllerClass = GetEffectiveNavVolumeGenerationControllerClass(Params);
	if (!GenerationControllerClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s Generation Controller is not set on %s. Generation will be skipped."), *Params.DebugInfo, *Params.Layer->GetName());
		return nullptr;
	}

	// Create the generator
	UHavokNavNavVolumeGenerator* Generator = NewObject<UHavokNavNavVolumeGenerator>(Params.World);
	if (!Generator)
	{
		UE_LOG(LogTemp, Error, TEXT("UPPHkNavHavokNavVolumeGeneratorFactory::CreateGenerator: Failed to create generator"));
		return nullptr;
	}

	// Configure the generator
	Generator->SetFlags(RF_Transient | RF_TextExportTransient | RF_DuplicateTransient);
	Generator->Settings = Settings;
	Generator->Layer = Params.Layer;
	Generator->Transform = Params.GenerationTransform;
	Generator->InputEntityGatherer = Params.InputEntityGatherer;
	Generator->BoundsProvider = Params.BoundsProvider;
	UObject* ControllerObject = NewObject<UObject>(Generator, GenerationControllerClass);
	Generator->Controller = TScriptInterface<IHavokNavNavVolumeGenerationController>(ControllerObject);
	Generator->DebugInfo = Params.DebugInfo;

#if WITH_EDITOR
	// Set up the loader adapter if a factory is provided
	if (Params.LoaderAdapterFactory)
	{
		Generator->LoaderAdapter = Params.LoaderAdapterFactory(Params.Layer, Generator->Settings);
	}
#endif

	// Params.OnGenerationCompletedCallback は UPPHkNav_QueuedVolumeGenerationRequest 側でハンドリングするためここでは設定しない

	return Generator;
}

const UHavokNavNavVolumeGenerationSettings* UPPHkNavHavokNavVolumeGeneratorFactory::GetEffectiveNavVolumeGenerationSettings(const FPPHkNavVolumeGenerationParams& Params)
{
	// If an override is provided, use it
	if (Params.GenerationSettingsOverride)
	{
		return Params.GenerationSettingsOverride;
	}

	// Fall back to project default settings
	return GetDefault<UHavokNavDefaultGenerationSettings>()->GetNavVolumeGenerationSettingsForLayer(Params.Layer);
}

const UClass* UPPHkNavHavokNavVolumeGeneratorFactory::GetEffectiveNavVolumeGenerationControllerClass(const FPPHkNavVolumeGenerationParams& Params)
{
	// If an override is provided, use it
	if (Params.ControllerOverride.GenerationControllerClass)
	{
		return Params.ControllerOverride.GenerationControllerClass;
	}

	// Otherwise, try to get controller class from the layer
	if (Params.Layer)
	{
		if (const UHavokNavNavVolumeLayer* LayerCDO = Params.Layer.GetDefaultObject())
		{
			return LayerCDO->GenerationParameters.GetGenerationController();
		}
	}

	// No valid controller class found
	return nullptr;
}
