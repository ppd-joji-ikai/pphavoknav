// Copyright (c) 2024 Pocketpair, Inc. All Rights Reserved.

#include "PPHkNav_DynamicNavMeshManager.h"
#include "PPHkNav_RuntimeGenerationSubsystem.h"
#include "PPHkNav_ActorBoundsProvider.h"
#include "PPHkNav_ActorInputEntityGatherer.h"
#include "PPHkNav_BoxBoundsProvider.h"
#include "PPHkNav_BoundsInputEntityGatherer.h"
#include "PPHkNav_BoxVolumeBoundsProvider.h"
#include "PPHkNav_BoundsVolumeInputEntityGatherer.h"
#include "PPHkNavHavokNavMeshGeneratorFactory.h"
#include "PPHkNavHavokNavVolumeGeneratorFactory.h"
#include "Engine/World.h"

void UPPHkNav_DynamicNavMeshManager::Initialize(UWorld* InWorld, UPPHkNav_RuntimeGenerationSubsystem* InRuntimeGenerationSubsystem)
{
	World = InWorld;
	RuntimeGenerationSubsystem = InRuntimeGenerationSubsystem;
}

void UPPHkNav_DynamicNavMeshManager::Deinitialize()
{
	World = nullptr;
	RuntimeGenerationSubsystem = nullptr;
}

UWorld* UPPHkNav_DynamicNavMeshManager::GetWorld() const
{
	if (World)
	{
		return World;
	}
	return Super::GetWorld();
}

void UPPHkNav_DynamicNavMeshManager::RequestActorNavMeshGeneration(FPPHkNavActorNavMeshGenerationParams&& RequestParams)
{
	AActor* Actor = RequestParams.RequestingActor.Get();
	if (!Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("RequestingActor is null in RequestActorNavMeshGeneration"));
		return;
	}

	if (RequestParams.Layer == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Layer is null in RequestActorNavMeshGeneration"));
		return;
	}

	UWorld* CurrentWorld = GetWorld();
	if (!CurrentWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("CurrentWorld is null in UPPHkNav_DynamicNavMeshManager."));
		return;
	}
	
	if (!RuntimeGenerationSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("RuntimeGenerationSubsystem is null in UPPHkNav_DynamicNavMeshManager."));
		return;
	}

	auto* BoundsProvider = NewObject<UPPHkNav_ActorBoundsProvider>(this);
	BoundsProvider->Initialize(Actor, RequestParams.Layer, Actor->GetActorUpVector());

	auto* InputEntityGatherer = NewObject<UPPHkNav_ActorInputEntityGatherer>(this);
	InputEntityGatherer->Initialize(Actor);

	FPPHkNavMeshGenerationParams CoreParams;
	CoreParams.World = CurrentWorld;
	CoreParams.Layer = RequestParams.Layer;
	CoreParams.ControllerOverride = RequestParams.ControllerOverride;
	CoreParams.GenerationSettingsOverride = nullptr; // Use default settings
	CoreParams.BoundsProvider = BoundsProvider;
	CoreParams.GenerationTransform = Actor->GetActorTransform();
	CoreParams.UpVector = Actor->GetActorUpVector();
	CoreParams.InputEntityGatherer = InputEntityGatherer;
	CoreParams.DebugInfo = FString::Printf(TEXT("UPPHkNav_DynamicNavMeshManager for %s"), *Actor->GetName());
	CoreParams.OnGenerationCompletedCallback = MoveTemp(RequestParams.OnGeneratedCallback);

	RuntimeGenerationSubsystem->RequestGeneration(MoveTemp(CoreParams));
}

void UPPHkNav_DynamicNavMeshManager::RequestBoxNavMeshGeneration(FPPHkNavBoxNavMeshGenerationParams&& RequestParams)
{
	if (!RequestParams.GenerationBox.IsValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("GenerationBox is not valid in RequestBoxNavMeshGeneration"));
		return;
	}

	if (RequestParams.Layer == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Layer is null in RequestBoxNavMeshGeneration"));
		return;
	}

	UWorld* CurrentWorld = GetWorld();
	if (!CurrentWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("CurrentWorld is null in UPPHkNav_DynamicNavMeshManager."));
		return;
	}
	
	if (!RuntimeGenerationSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("RuntimeGenerationSubsystem is null in UPPHkNav_DynamicNavMeshManager."));
		return;
	}

	auto* BoundsProvider = NewObject<UPPHkNav_BoxBoundsProvider>(this);
	BoundsProvider->Initialize(RequestParams.GenerationBox, RequestParams.Layer, RequestParams.UpVector);

	auto* InputEntityGatherer = NewObject<UPPHkNav_BoundsInputEntityGatherer>(this);
	InputEntityGatherer->Initialize(CurrentWorld);

	FPPHkNavMeshGenerationParams CoreParams;
	CoreParams.World = CurrentWorld;
	CoreParams.Layer = RequestParams.Layer;
	CoreParams.ControllerOverride = RequestParams.ControllerOverride;
	CoreParams.GenerationSettingsOverride = RequestParams.GenerationSettingsOverride;
	CoreParams.BoundsProvider = BoundsProvider;
	CoreParams.GenerationTransform = RequestParams.GenerationTransform;
	CoreParams.UpVector = RequestParams.UpVector;
	CoreParams.InputEntityGatherer = InputEntityGatherer;
	CoreParams.DebugInfo = FString::Printf(TEXT("UPPHkNav_DynamicNavMeshManager for Box %s"), *RequestParams.GenerationBox.ToString());
	CoreParams.OnGenerationCompletedCallback = MoveTemp(RequestParams.OnGeneratedCallback);

	RuntimeGenerationSubsystem->RequestGeneration(MoveTemp(CoreParams));
}

void UPPHkNav_DynamicNavMeshManager::RequestBoxNavVolumeGeneration(FPPHkNavBoxNavVolumeGenerationParams&& RequestParams)
{
	if (!RequestParams.GenerationBox.IsValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("GenerationBox is not valid in RequestBoxNavVolumeGeneration"));
		return;
	}

	if (RequestParams.Layer == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Layer is null in RequestBoxNavVolumeGeneration"));
		return;
	}

	UWorld* CurrentWorld = GetWorld();
	if (!CurrentWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("CurrentWorld is null in UPPHkNav_DynamicNavMeshManager."));
		return;
	}
	
	if (!RuntimeGenerationSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("RuntimeGenerationSubsystem is null in UPPHkNav_DynamicNavMeshManager."));
		return;
	}

	auto* BoundsProvider = NewObject<UPPHkNav_BoxVolumeBoundsProvider>(this);
	BoundsProvider->Initialize(RequestParams.GenerationBox, RequestParams.Layer, RequestParams.NavVolumeBoundsActor);

	auto* InputEntityGatherer = NewObject<UPPHkNav_BoundsVolumeInputEntityGatherer>(this);
	InputEntityGatherer->Initialize(CurrentWorld);

	const FHavokNavAxialRotator AxialRotation = FHavokNavAxialRotator::FromQuat(RequestParams.GenerationTransform.GetRotation());
	FPPHkNavVolumeGenerationParams CoreParams;
	CoreParams.World = CurrentWorld;
	CoreParams.Layer = RequestParams.Layer;
	CoreParams.ControllerOverride = RequestParams.ControllerOverride;
	CoreParams.GenerationSettingsOverride = nullptr; // Use default settings
	CoreParams.BoundsProvider = BoundsProvider;
	CoreParams.GenerationTransform = FHavokNavAxialTransform(AxialRotation, RequestParams.GenerationTransform.GetLocation());
	CoreParams.InputEntityGatherer = InputEntityGatherer;
	CoreParams.DebugInfo = FString::Printf(TEXT("UPPHkNav_DynamicNavMeshManager for NavVolume Box %s"), *RequestParams.GenerationBox.ToString());
	CoreParams.OnGenerationCompletedCallback = MoveTemp(RequestParams.OnGeneratedCallback);

	// NavVolume generation request to RuntimeGenerationSubsystem
	RuntimeGenerationSubsystem->RequestVolumeGeneration(MoveTemp(CoreParams));
}
