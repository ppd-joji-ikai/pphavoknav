// Copyright Pocketpair, Inc. All Rights Reserved.

#include "PPHkNav_QueuedVolumeGenerationRequest.h"
#include "PPHkNav_RuntimeGenerationSubsystem.h"
#include "PPHkNavHavokNavVolumeGeneratorFactory.h"
#include "HavokNavNavVolumeGenerator.h"

void UPPHkNav_QueuedVolumeGenerationRequest::Initialize(
	FPPHkNavVolumeGenerationParams&& InParams,
	UPPHkNav_RuntimeGenerationSubsystem* InOwnerSubsystem)
{
	Params = MoveTemp(InParams);
	OwnerSubsystem = InOwnerSubsystem;
	bIsCancelled = false;
	bIsFinished = false;
}

void UPPHkNav_QueuedVolumeGenerationRequest::StartGeneration()
{
	if (bIsCancelled)
	{
		return;
	}

	Generator = UPPHkNavHavokNavVolumeGeneratorFactory::CreateGenerator(Params);
	if (Generator)
	{
		Generator->OnGenerationCompleted().AddUObject(this, &UPPHkNav_QueuedVolumeGenerationRequest::GeneratorCompleted);
		Generator->InitiateGeneration();
	}
	else
	{
		// ジェネレーター作成が失敗した場合、即座に完了を報告してキューを続行できるようにする
		bIsFinished = true;
		ReportFinished(OwnerSubsystem);
	}
}

void UPPHkNav_QueuedVolumeGenerationRequest::Cancel()
{
	bIsCancelled = true;
	
	if (Generator)
	{
		Generator->UnloadWorldPartitionLoaderAdapterResources();
		Generator = nullptr;
	}
}

bool UPPHkNav_QueuedVolumeGenerationRequest::IsFinished() const
{
	return bIsFinished;
}

bool UPPHkNav_QueuedVolumeGenerationRequest::IsCancelled() const
{
	return bIsCancelled;
}

int32 UPPHkNav_QueuedVolumeGenerationRequest::GetPriority() const
{
	return 0;
}

FString UPPHkNav_QueuedVolumeGenerationRequest::GetRequestTypeName() const
{
	return TEXT("NavVolume");
}

void UPPHkNav_QueuedVolumeGenerationRequest::ReportFinished(UPPHkNav_RuntimeGenerationSubsystem* Subsystem)
{
	if (Subsystem)
	{
		Subsystem->ReportRequestFinished(this);
	}
}

void UPPHkNav_QueuedVolumeGenerationRequest::GeneratorCompleted(UHavokNavNavVolumeGenerator* CompletedGenerator, FHavokNavNavVolumeGeneratorResult Result)
{
	// 自分のジェネレーターの完了のみを処理することを確認
	if (CompletedGenerator != Generator)
	{
		return;
	}

	// コールバックを呼び出す
	if (Params.OnGenerationCompletedCallback)
	{
		Params.OnGenerationCompletedCallback(Result);
	}

	bIsFinished = true;
	ReportFinished(OwnerSubsystem);
}
