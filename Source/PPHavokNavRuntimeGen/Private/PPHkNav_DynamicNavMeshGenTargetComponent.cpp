// Fill out your copyright notice in the Description page of Project Settings.


#include "PPHkNav_DynamicNavMeshGenTargetComponent.h"

#include "HavokNavNavMesh.h"
#include "HavokNavNavMeshInstanceSetComponent.h"
#include "PPHkNav_DynamicNavMeshSubsystem.h"
#include "ClusterGraph/HavokNavClusterGraph.h"
#include "ClusterGraph/HavokNavClusterGraphUtilities.h"

namespace MassPal::Navigation::Private
{
	// NavMeshが削除されたときのハンドラ
	static void HandleNavMeshRemoved(UHavokNavNavMeshInstanceSetComponent* , UHavokNavNavMesh* HavokNavNavMesh)
	{
		HavokNavNavMesh->RemoveConnectionsFromConnectedNavMeshes();
		if (const UHavokNavClusterGraph* ClusterGraph = FHavokNavClusterGraphUtilities::TryGetClusterGraphForNavMesh(HavokNavNavMesh))
		{
			ClusterGraph->RemoveConnectionsFromConnectedClusterGraphs();
		}
	}
}

UPPHkNav_DynamicNavMeshGenTargetComponent::UPPHkNav_DynamicNavMeshGenTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	InstanceSetComponent = CreateDefaultSubobject<UHavokNavNavMeshInstanceSetComponent>(TEXT("InstanceSetComponent"));
	InstanceSetComponent->SetupAttachment(this);
	if (!IsTemplate())
	{
		InstanceSetComponent->OnNavMeshRemoved().AddStatic(&MassPal::Navigation::Private::HandleNavMeshRemoved);
	}
}

void UPPHkNav_DynamicNavMeshGenTargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// ロード済みNavMeshを全てクリア
	if (InstanceSetComponent)
	{
		InstanceSetComponent->Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void UPPHkNav_DynamicNavMeshGenTargetComponent::RequestGeneration()
{
	if (InstanceSetComponent)
	{
		InstanceSetComponent->Reset();
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// NavMesh生成をリクエストします
	if (const UPPHkNav_DynamicNavMeshSubsystem* NavGenSubsystem = GetWorld()->GetSubsystem<UPPHkNav_DynamicNavMeshSubsystem>())
	{
		TWeakObjectPtr<ThisClass> WeakThis = this;
		auto&& Callback = [WeakThis](const auto& Result)
		{
			if (UPPHkNav_DynamicNavMeshGenTargetComponent* This = WeakThis.Get())
			{
				This->OnNavMeshGenerationCompleted(Result);
			}
		};
		
		FPPHkNavActorNavMeshGenerationParams Params
		{
			.RequestingActor = Owner,
			.Layer = Layer,
			.ControllerOverride = Override,
			.OnGeneratedCallback = MoveTemp(Callback),
		};
		
		NavGenSubsystem->GetGeneratorInterface()->RequestActorNavMeshGeneration(MoveTemp(Params));
	}	
}

void UPPHkNav_DynamicNavMeshGenTargetComponent::OnNavMeshGenerationCompleted(const FHavokNavNavMeshGeneratorResult& Result)
{
	if (Result.NavMesh)
	{
		if (Result.ClusterGraph)
		{
			Result.NavMesh->SetClusterGraph(Result.ClusterGraph);
		}

		Result.NavMesh->Rename(nullptr, InstanceSetComponent);
		InstanceSetComponent->AddBaseNavMesh(Result.NavMesh);
	}
	else
	{
		InstanceSetComponent->RemoveNavMeshForLayer(Result.Layer);
	}
}
