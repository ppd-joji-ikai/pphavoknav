// Copyright (c) 2024 Pocketpair, Inc. All Rights Reserved.

#include "PPHkNavHavokNavMeshGeneratorFactory.h"
#include "HavokNavNavMesh.h"
#include "ClusterGraph/HavokNavClusterGraph.h"
#include "HavokNavNavMeshGenerator.h"
#include "HavokNavNavMeshGenerationSettings.h"
#include "HavokNavDefaultGenerationSettings.h"
#include "IHavokNavNavMeshGenerationBoundsProvider.h"
#include "IHavokNavNavMeshGenerationInputEntityGatherer.h"
#include "HavokNavWorldSubsystem.h"
#include "WorldPartition/HavokNavScopedLoaderAdapterHelper.h"

#if WITH_EDITOR
#include "ScopedTransaction.h"
#endif // WITH_EDITOR

UHavokNavNavMeshGenerator* UPPHkNavHavokNavMeshGeneratorFactory::CreateGenerator(const FPPHkNavMeshGenerationParams& Params)
{
	if (!Params.Layer)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s Layer is not set. Generation will be skipped."), *Params.DebugInfo);
		return nullptr;
	}

	const UClass* GenerationControllerClass = GetEffectiveNavMeshGenerationControllerClass(Params);
	if (!GenerationControllerClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s Generation Controller is not set on %s. Generation will be skipped."), *Params.DebugInfo, *Params.Layer->GetName());
		return nullptr;
	}

	if (!Params.BoundsProvider)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s Bounds Provider has not been supplied. Generation will be skipped."), *Params.DebugInfo);
		return nullptr;
	}

	const FVector NormalizedUpVector = Params.UpVector.GetSafeNormal();
	if (NormalizedUpVector.IsZero())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s up vector cannot be normalized. Generation will be skipped."), *Params.DebugInfo);
		return nullptr;
	}

	if (!Params.InputEntityGatherer)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s Input Entity Gatherer has not been created. Generation will be skipped."), *Params.DebugInfo);
		return nullptr;
	}

	const UHavokNavNavMeshGenerationSettings* Settings = GetEffectiveNavMeshGenerationSettings(Params);
	if (!Settings)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s has no Nav Mesh Generation Settings. Check the Project Settings > Plugins > Havok Navigation > Default Generation Settings."), *Params.DebugInfo);
		return nullptr;
	}
	// Since we don't support snapshots, we ensure that the settings don't have a snapshot path.
	ensure(Settings->SnapshotPath.IsEmpty());
	ensure(Settings->TraversalAnalysisSettings.SnapshotPath.IsEmpty());

	// 生成
	UHavokNavNavMeshGenerator* Generator = NewObject<UHavokNavNavMeshGenerator>();
	Generator->SetFlags(RF_Transient | RF_TextExportTransient | RF_DuplicateTransient);
	Generator->Layer = Params.Layer;
	Generator->InputEntityGatherer = Params.InputEntityGatherer;
	Generator->BoundsProvider = Params.BoundsProvider;
	Generator->Controller = NewObject<UObject>(Generator, GenerationControllerClass);
	Generator->Transform = Params.GenerationTransform;
	Generator->UpVector = NormalizedUpVector;
	Generator->DebugInfo = Params.DebugInfo;
	Generator->Settings = Settings;

#if WITH_EDITOR
	if (Params.LoaderAdapterFactory)
	{
		Generator->LoaderAdapter = Params.LoaderAdapterFactory(Params.Layer, Generator->Settings);
	}
#endif // WITH_EDITOR

	return Generator;
}

const UHavokNavNavMeshGenerationSettings* UPPHkNavHavokNavMeshGeneratorFactory::GetEffectiveNavMeshGenerationSettings(const FPPHkNavMeshGenerationParams& Params)
{
	if (Params.GenerationSettingsOverride)
	{
		return Params.GenerationSettingsOverride;
	}

	return GetDefault<UHavokNavDefaultGenerationSettings>()->GetNavMeshGenerationSettingsForLayer(Params.Layer);
}

const UClass* UPPHkNavHavokNavMeshGeneratorFactory::GetEffectiveNavMeshGenerationControllerClass(const FPPHkNavMeshGenerationParams& Params)
{
	if (Params.ControllerOverride.GenerationControllerClass)
	{
		return Params.ControllerOverride.GenerationControllerClass;
	}

	if (Params.Layer)
	{
		return Params.Layer.GetDefaultObject()->GenerationParameters.GetGenerationController();
	}

	return nullptr;
}
