// Copyright Pocketpair, Inc. All Rights Reserved.

#include "PPHkNav_RuntimeGenerationSubsystem.h"
#include "PPHkNav_QueuedGenerationRequest.h"
#include "PPHkNav_QueuedVolumeGenerationRequest.h"
#include "IPPHkNav_QueuedRequest.h"
#include "HavokNavDefaultNavMeshConnectionsGenerator.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogPPHkNavRuntimeGen, Log, All);

UPPHkNav_RuntimeGenerationSubsystem::UPPHkNav_RuntimeGenerationSubsystem()
{
	NavMeshConnectionsGenerator = CreateDefaultSubobject<UHavokNavDefaultNavMeshConnectionsGenerator>(TEXT("NavMeshConnectionsGenerator"), /*bTransient=*/true);
}

void UPPHkNav_RuntimeGenerationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPPHkNav_RuntimeGenerationSubsystem::Deinitialize()
{
	CancelAllGeneration();
	Super::Deinitialize();
}

void UPPHkNav_RuntimeGenerationSubsystem::Tick(float DeltaTime)
{
	ProcessNextRequest();
}

bool UPPHkNav_RuntimeGenerationSubsystem::IsTickable() const
{
	return !IsTemplate() && GetWorld() != nullptr && GetWorld()->IsGameWorld();
}

TStatId UPPHkNav_RuntimeGenerationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPPHkNav_RuntimeGenerationSubsystem, STATGROUP_Tickables);
}

void UPPHkNav_RuntimeGenerationSubsystem::RequestGeneration(FPPHkNavMeshGenerationParams&& InParams)
{
	UPPHkNav_QueuedGenerationRequest* NewRequest = NewObject<UPPHkNav_QueuedGenerationRequest>(this);
	NewRequest->Initialize(MoveTemp(InParams), this, NavMeshConnectionsGenerator);
	
	TScriptInterface<IPPHkNav_QueuedRequest> RequestInterface(NewRequest);
	InsertRequestByPriority(RequestInterface);
	ProcessNextRequest();
}

void UPPHkNav_RuntimeGenerationSubsystem::RequestVolumeGeneration(FPPHkNavVolumeGenerationParams&& InParams)
{
	UPPHkNav_QueuedVolumeGenerationRequest* NewRequest = NewObject<UPPHkNav_QueuedVolumeGenerationRequest>(this);
	NewRequest->Initialize(MoveTemp(InParams), this);
	
	TScriptInterface<IPPHkNav_QueuedRequest> RequestInterface(NewRequest);
	InsertRequestByPriority(RequestInterface);
	ProcessNextRequest();
}

void UPPHkNav_RuntimeGenerationSubsystem::CancelAllGeneration()
{
	// 現在のリクエストをキャンセル
	if (IPPHkNav_QueuedRequest* RequestInterface = CurrentRequest.GetInterface())
	{
		RequestInterface->Cancel();
		CurrentRequest = {};
	}

	// キュー内のすべてのリクエストをキャンセル
	for (const TScriptInterface<IPPHkNav_QueuedRequest>& Request : RequestQueue)
	{
		if (IPPHkNav_QueuedRequest* RequestInterface = Request.GetInterface())
		{
			RequestInterface->Cancel();
		}
	}
	RequestQueue.Empty();
}

bool UPPHkNav_RuntimeGenerationSubsystem::IsGenerating() const
{
	return IsBusy() || !RequestQueue.IsEmpty();
}

void UPPHkNav_RuntimeGenerationSubsystem::ReportRequestFinished(IPPHkNav_QueuedRequest* Request)
{
	// 統一されたコールバック処理
	if (CurrentRequest.GetInterface() == Request)
	{
		CurrentRequest = {};
		UE_LOG(LogPPHkNavRuntimeGen, Verbose, TEXT("Request finished: %s"), *Request->GetRequestTypeName());
		ProcessNextRequest();
	}
}

void UPPHkNav_RuntimeGenerationSubsystem::ReportRequestFinished(UPPHkNav_QueuedGenerationRequest* Request)
{
	// 後方互換性のための個別メソッド
	if (IPPHkNav_QueuedRequest* QueuedRequest = Cast<IPPHkNav_QueuedRequest>(Request))
	{
		ReportRequestFinished(QueuedRequest);
	}
}

void UPPHkNav_RuntimeGenerationSubsystem::ReportVolumeRequestFinished(UPPHkNav_QueuedVolumeGenerationRequest* Request)
{
	// 後方互換性のための個別メソッド
	if (IPPHkNav_QueuedRequest* QueuedRequest = Cast<IPPHkNav_QueuedRequest>(Request))
	{
		ReportRequestFinished(QueuedRequest);
	}
}

void UPPHkNav_RuntimeGenerationSubsystem::ProcessNextRequest()
{
	// ガード節: 現在処理中の場合は何もしない
	if (IsBusy())
	{
		return;
	}

	// キューから次のリクエストを取得
	if (RequestQueue.IsEmpty())
	{
		return;
	}

	CurrentRequest = RequestQueue[0];
	RequestQueue.RemoveAt(0);

	// TScriptInterfaceから生ポインタに受けてnullptr比較と処理を同時に行う
	if (auto* QueuedRequest = CurrentRequest.GetInterface())
	{
		// キャンセル済みの場合は次のリクエストに進む
		if (QueuedRequest->IsCancelled())
		{
			CurrentRequest = {};
			ProcessNextRequest();
			return;
		}

		UE_LOG(LogPPHkNavRuntimeGen, Verbose, TEXT("Starting request: %s"), *QueuedRequest->GetRequestTypeName());
		QueuedRequest->StartGeneration();
	}
}

bool UPPHkNav_RuntimeGenerationSubsystem::IsBusy() const
{
	return CurrentRequest.GetInterface() != nullptr;
}

void UPPHkNav_RuntimeGenerationSubsystem::InsertRequestByPriority(TScriptInterface<IPPHkNav_QueuedRequest> Request)
{
	if (const IPPHkNav_QueuedRequest* RequestInterface = Request.GetInterface())
	{
		const int32 RequestPriority = RequestInterface->GetPriority();

		// 優先度順に挿入（数値が小さいほど高優先度）
		// ほとんどのリクエストが同一優先度なので末尾から探索して効率化
		int32 InsertIndex = RequestQueue.Num();
		for (int32 Index = RequestQueue.Num() - 1; Index >= 0; --Index)
		{
			if (const IPPHkNav_QueuedRequest* ExistingRequestInterface = RequestQueue[Index].GetInterface())
			{
				if (RequestPriority >= ExistingRequestInterface->GetPriority())
				{
					InsertIndex = Index + 1;
					break;
				}
			}
			InsertIndex = Index;
		}

		RequestQueue.Insert(Request, InsertIndex);
		UE_LOG(LogPPHkNavRuntimeGen, Verbose, TEXT("Queued request: %s (Priority: %d, Position: %d)"), 
			*RequestInterface->GetRequestTypeName(), RequestPriority, InsertIndex);
	}
	else
	{
		UE_LOG(LogPPHkNavRuntimeGen, Error, TEXT("Invalid request interface"));
	}
}
