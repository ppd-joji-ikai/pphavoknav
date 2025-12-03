// Copyright (c) 2024 Pocketpair, Inc. All Rights Reserved.

#include "PPHkNav_QueuedGenerationRequest.h"
#include "PPHkNav_RuntimeGenerationSubsystem.h"
#include "PPHkNavHavokNavMeshGeneratorFactory.h"
#include "HavokNavNavMeshGenerator.h"
#include "HavokNavNavMesh.h"
#include "ClusterGraph/HavokNavClusterGraph.h"
#include "IHavokNavNavMeshConnectionsGenerator.h"

void UPPHkNav_QueuedGenerationRequest::Initialize(
	FPPHkNavMeshGenerationParams&& InParams,
	UPPHkNav_RuntimeGenerationSubsystem* InOwnerSubsystem,
	TScriptInterface<IHavokNavNavMeshConnectionsGenerator> InConnectionsGenerator)
{
	Params = MoveTemp(InParams);
	OwnerSubsystem = InOwnerSubsystem;
	NavMeshConnectionsGenerator = InConnectionsGenerator;
	bIsCancelled = false;
	bIsFinished = false;
}

void UPPHkNav_QueuedGenerationRequest::StartGeneration()
{
	if (bIsCancelled)
	{
		return;
	}

	Generator = UPPHkNavHavokNavMeshGeneratorFactory::CreateGenerator(Params);
	if (Generator)
	{
		Generator->OnGenerationCompleted().AddUObject(this, &UPPHkNav_QueuedGenerationRequest::GeneratorCompleted);
		Generator->InitiateGeneration();
	}
	else
	{
		// ジェネレーター作成が失敗した場合、即座に完了を報告してキューを続行できるようにする
		bIsFinished = true;
		ReportFinished(OwnerSubsystem);
	}
}

void UPPHkNav_QueuedGenerationRequest::Cancel()
{
	bIsCancelled = true;
	
	if (Generator)
	{
		Generator->UnloadWorldPartitionLoaderAdapterResources();
		Generator = nullptr;
	}
}

bool UPPHkNav_QueuedGenerationRequest::IsFinished() const
{
	return bIsFinished;
}

bool UPPHkNav_QueuedGenerationRequest::IsCancelled() const
{
	return bIsCancelled;
}

int32 UPPHkNav_QueuedGenerationRequest::GetPriority() const
{
	return 0;
}

FString UPPHkNav_QueuedGenerationRequest::GetRequestTypeName() const
{
	return TEXT("NavMesh");
}

void UPPHkNav_QueuedGenerationRequest::ReportFinished(UPPHkNav_RuntimeGenerationSubsystem* Subsystem)
{
	if (Subsystem)
	{
		Subsystem->ReportRequestFinished(this);
	}
}

void UPPHkNav_QueuedGenerationRequest::GeneratorCompleted(UHavokNavNavMeshGenerator* CompletedGenerator, FHavokNavNavMeshGeneratorResult Result)
{
	// 自分のジェネレーターの完了のみを処理することを確認
	if (CompletedGenerator != Generator)
	{
		return;
	}

	// NavMeshの後処理（接続生成）
	if (Result.NavMesh && NavMeshConnectionsGenerator.GetInterface())
	{
		GenerateConnections(Result.NavMesh, Result.ClusterGraph);
	}

	// コールバックを呼び出す
	if (Params.OnGenerationCompletedCallback)
	{
		Params.OnGenerationCompletedCallback(Result);
	}

	bIsFinished = true;
	ReportFinished(OwnerSubsystem);
}

void UPPHkNav_QueuedGenerationRequest::GenerateConnections(UHavokNavNavMesh* NavMesh, UHavokNavClusterGraph* ClusterGraph) const
{
	if (!NavMeshConnectionsGenerator)
	{
		return;
	}

	FTransform NavMeshTransform = Params.GenerationTransform;
	NavMeshTransform.SetScale3D(FVector::OneVector);

	const UHavokNavNavMeshGenerationSettings* NavMeshGenerationSettings = UPPHkNavHavokNavMeshGeneratorFactory::GetEffectiveNavMeshGenerationSettings(Params);
	const TArray<FHavokNavNavMeshConnectionGenerationResult> ConnectionGenerationResults = NavMeshConnectionsGenerator->
		GenerateNavMeshConnections(NavMesh, ClusterGraph, NavMeshTransform, NavMeshGenerationSettings);

	for (const FHavokNavNavMeshConnectionGenerationResult& Result : ConnectionGenerationResults)
	{
		Result.NavMeshConnection->Rename(nullptr, NavMesh);
		NavMesh->AddNavMeshConnection(Result.NavMeshConnection.Get());

		auto* DuplicateConnection = DuplicateObject(Result.NavMeshConnection.Get(), Result.OtherNavMesh.Get());
		Result.OtherNavMesh->AddNavMeshConnection(DuplicateConnection);

		if (Result.ClusterGraphConnection.Get())
		{
			check(Result.OtherClusterGraph.IsValid());

			Result.ClusterGraphConnection->Rename(nullptr, ClusterGraph);
			ClusterGraph->AddClusterGraphConnection(Result.ClusterGraphConnection.Get());

			auto* DuplicateGraphConnection = DuplicateObject(Result.ClusterGraphConnection.Get(), Result.OtherClusterGraph.Get());
			Result.OtherClusterGraph->AddClusterGraphConnection(DuplicateGraphConnection);
		}
	}
}
