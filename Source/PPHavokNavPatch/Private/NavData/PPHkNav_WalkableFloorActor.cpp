// Copyright Pocketpair, Inc. All Rights Reserved.


#include "PPHkNav_WalkableFloorActor.h"

#include "HavokNavNavMesh.h"
#include "PPHkNavNavMeshMultiPatchComponent.h"
#include "PPHkNav_NavMeshDataAsset.h"
#include "ClusterGraph/HavokNavClusterGraph.h"
#include "Util/PPHkNavLevelInstanceIdIssuer.h"

APPHkNav_WalkableFloorActor::APPHkNav_WalkableFloorActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MultiPatchComponent = CreateDefaultSubobject<UPPHkNavNavMeshMultiPatchComponent>(TEXT("MultiPatchComponent"));
	SetRootComponent(MultiPatchComponent);
	PrimaryActorTick.bCanEverTick = false;
}

void APPHkNav_WalkableFloorActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// 事前プレビューしたい場合はここでオプション的に Instantiate 可
}

void APPHkNav_WalkableFloorActor::BeginPlay()
{
	Super::BeginPlay();

	if (!bAutoInstantiateOnBeginPlay)
	{
		return;
	}

	if (!bNavMeshInstantiated)
	{
		if (!InstantiateNavMeshInternal())
		{
			UE_LOG(LogTemp, Warning, TEXT("WalkableFloor: Failed to instantiate NavMesh (%s)"), *GetName());
		}
	}
}

void APPHkNav_WalkableFloorActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TeardownNavMeshInternal();
	Super::EndPlay(EndPlayReason);
}

bool APPHkNav_WalkableFloorActor::ReloadNavMesh()
{
	TeardownNavMeshInternal();
	return InstantiateNavMeshInternal();
}

bool APPHkNav_WalkableFloorActor::InstantiateNavMeshInternal()
{
	if (bNavMeshInstantiated)
	{
		return true;
	}

	if (!NavMeshDataAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("WalkableFloor: NavMeshDataAsset is null (%s)"), *GetName());
		return false;
	}

	UHavokNavNavMesh* const Source = NavMeshDataAsset->PreBakedNavMesh;
	if (!Source)
	{
		UE_LOG(LogTemp, Warning, TEXT("WalkableFloor: PreBakedNavMesh is null in DataAsset (%s)"), *GetName());
		return false;
	}

	if (!IsValid(MultiPatchComponent))
	{
		UE_LOG(LogTemp, Error, TEXT("WalkableFloor: MultiPatchComponent missing (%s)"), *GetName());
		return false;
	}

	const uint64 NewLevelInstanceId = FPPHkNavLevelInstanceIdIssuer::Allocate();
	MultiPatchComponent->AddPatchNavMeshWithLevelInstanceId(Source, GetActorTransform(), NewLevelInstanceId);
	bNavMeshInstantiated = true;
	return true;
}

void APPHkNav_WalkableFloorActor::TeardownNavMeshInternal()
{
	if (!bNavMeshInstantiated)
	{
		return;
	}

	if (MultiPatchComponent)
	{
		MultiPatchComponent->ResetPatches();
	}

	bNavMeshInstantiated = false;
}
