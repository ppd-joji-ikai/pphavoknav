// Copyright Pocketpair, Inc. All Rights Reserved.

#include "PPHkNavInstancedNavMeshComponent.h"

#include "HavokNavNavMesh.h"
#include "HavokNavNavMeshInstance.h"
#include "HavokNavTypes.h"
#include "HavokNavUtilities.h"
#include "HavokNavWorld.h"
#include "HavokNavWorldSubsystem.h"
#include "PPHkNavInstancedNavMeshUserEdgeGenerator.h"
#include "PPHkNav_NavMeshDataAsset.h"
#include "PPHkNav_NavMeshInstance.h"
#include "ClusterGraph/HavokNavClusterGraph.h"
#include "ClusterGraph/HavokNavClusterGraphUtilities.h"
#include "Engine/World.h"
#include "Util/PPHkNavLevelInstanceIdIssuer.h"

namespace PPHkNav::Private
{
	static FORCEINLINE FTransform MakeWorldTransform(const FTransform& ComponentWorld, const FTransform& Relative)
	{
		return Relative * ComponentWorld;
	}

	struct FInstancedNavMeshComponentInternal
	{
		static UHavokNavNavMesh* GetBaseNavMesh(const UPPHkNavInstancedNavMeshComponent* C)
		{
			return (C && C->NavMeshDataAsset) ? C->NavMeshDataAsset->PreBakedNavMesh : nullptr;
		}

		static UHavokNavWorldSubsystem* GetNavWorldSubsystem(const UPPHkNavInstancedNavMeshComponent* C)
		{
			if (!C) return nullptr;
			if (UWorld* World = C->GetWorld())
			{
				return UHavokNavWorldSubsystem::GetInstance(World);
			}
			return nullptr;
		}

		static UHavokNavWorld* GetHavokNavWorld(const UPPHkNavInstancedNavMeshComponent* C)
		{
			if (UHavokNavWorldSubsystem* Subsystem = GetNavWorldSubsystem(C))
			{
				return Subsystem->HavokNavWorld;
			}
			return nullptr;
		}

		static int32 AllocateNewId(UPPHkNavInstancedNavMeshComponent* C)
		{
			return C ? C->NextInstanceId++ : INDEX_NONE;
		}

		static int32 IdToIndex(const UPPHkNavInstancedNavMeshComponent* C, const int32 Id)
		{
			if (C)
			{
				if (const int32* Found = C->IdToIndexMap.Find(Id))
				{
					return *Found;
				}
			}
			return INDEX_NONE;
		}

		static void TryLoadInstance(UPPHkNavInstancedNavMeshComponent* C, UPPHkNavInstancedNavMeshComponent::FInstanceRecord& Record)
		{
			if (!IsValid(C)) { return; }
			if (!Record.bPendingLoad && IsValid(Record.RuntimeInstance))
			{
				return;
			}
			UHavokNavNavMesh* BaseMesh = GetBaseNavMesh(C);
			if (!BaseMesh) { return; }
			if (BaseMesh->IsHkNavMeshEmpty())
			{
				Record.bPendingLoad = false; // 空メッシュはロード不要
				return;
			}

			if (const UWorld* World = C->GetWorld();
				!World || !World->bIsWorldInitialized || World->WorldType == EWorldType::Inactive) { return; }
			
			const UHavokNavWorldSubsystem* Subsystem = GetNavWorldSubsystem(C);
			if (!Subsystem || !Subsystem->HavokNavWorld) { return; }

			UHavokNavWorld* NavWorld = Subsystem->HavokNavWorld.Get();

			// NavMeshのロード
			const FTransform WorldTransform = MakeWorldTransform(C->GetComponentTransform(), Record.RelativeTransform);
			const uint64 LevelInstanceId = (Record.LevelInstanceIdOverride != 0ULL) ? Record.LevelInstanceIdOverride : FHavokNavUtilities::GetLevelInstanceId(C->GetOwner(), BaseMesh);
			const UHavokNavNavMeshInstance::EBuildStreamingSetsOnLoadMode BuildMode = C->bBuildAdditionalStreamingSetsOnLoad
					? UHavokNavNavMeshInstance::EBuildStreamingSetsOnLoadMode::BuildAdditionalStreamingSets
					: UHavokNavNavMeshInstance::EBuildStreamingSetsOnLoadMode::PrecomputedStreamingSetsOnly;
			Record.RuntimeInstance = NavWorld->LoadNavMesh(BaseMesh, LevelInstanceId, WorldTransform, /*ConnectionBindings*/{}, BuildMode, UPPHkNavPatchNavMeshInstance::StaticClass());
			Record.bPendingLoad = false;

			if (IsValid(Record.RuntimeInstance))
			{
				// クラスターグラフのロード
				if (UHavokNavClusterGraph* ClusterGraph = BaseMesh->GetClusterGraph())
				{
					TArray<UHavokNavWorld::FClusterGraphConnectionBinding> ClusterBindings = FHavokNavClusterGraphUtilities::MakeClusterGraphConnectionBindings(NavWorld, C->GetOwner(), ClusterGraph);
					UHavokNavClusterGraphInstance* ClusterGraphInstance = NavWorld->LoadClusterGraph(ClusterGraph, LevelInstanceId, WorldTransform, ClusterBindings);
					Record.RuntimeInstance->SetClusterGraphInstance(ClusterGraphInstance);
				}

				// ユーザーエッジの実行時生成
				{
					Record.RuntimeInstance->OnLoaded().AddUObject(C, &UPPHkNavInstancedNavMeshComponent::HandleRuntimeInstanceLoaded);
					// 既に同期ロード完了状態なら即ビルド
					if (Record.RuntimeInstance->IsLoaded())
					{
						C->HandleRuntimeInstanceLoaded(Record.RuntimeInstance.Get());
					}
				}
			}
		}

		static void TryUnloadInstance(const UPPHkNavInstancedNavMeshComponent* C, UPPHkNavInstancedNavMeshComponent::FInstanceRecord& Record)
		{
			if (!C) { return; }

			// ユーザーエッジセット破棄
			if (Record.EdgeSetId)
			{
				UHavokNavWorld* NavWorld = GetHavokNavWorld(C);
				C->EdgeGenerator->RemoveEdges(NavWorld, Record.EdgeSetId);
				Record.EdgeSetId = {};
			}

			// NavMeshInstanceのアンロード
			if (IsValid(Record.RuntimeInstance))
			{
				Record.RuntimeInstance->OnLoaded().RemoveAll(C);
				Record.RuntimeInstance->Unload();
				Record.RuntimeInstance = nullptr;
			}
			Record.bPendingLoad = true;
		}

		static void TryLoadAll(UPPHkNavInstancedNavMeshComponent* C)
		{
			if (!C) return;
			for (auto& R : C->InstanceRecords)
			{
				TryLoadInstance(C, R);
			}
		}

		static void TryUnloadAll(UPPHkNavInstancedNavMeshComponent* C)
		{
			if (!C) return;
			for (auto& R : C->InstanceRecords)
			{
				TryUnloadInstance(C, R);
			}
		}

		static void RefreshAllRuntimeInstanceTransforms(UPPHkNavInstancedNavMeshComponent* C)
		{
			if (!C) return;
			const FTransform ComponentWorld = C->GetComponentTransform();
			for (auto& R : C->InstanceRecords)
			{
				if (IsValid(R.RuntimeInstance))
				{
					R.RuntimeInstance->SetTransform(MakeWorldTransform(ComponentWorld, R.RelativeTransform));
				}
			}
		}
	};
}// namespace PPHkNav::Private

// ===== Component =====
UPPHkNavInstancedNavMeshComponent::UPPHkNavInstancedNavMeshComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
	bWantsOnUpdateTransform = true;

	EdgeGenerator = CreateDefaultSubobject<UPPHkNavInstancedNavMeshUserEdgeGenerator>(TEXT("NavMeshUserEdgeEdgeGenerator"));
}

// ===== Public API =====
UPPHkNavInstancedNavMeshComponent::FInstanceId UPPHkNavInstancedNavMeshComponent::AddInstance(const FTransform& InstanceTransform, bool bWorldSpace /*=false*/)
{
	const TConstArrayView<FTransform> View(&InstanceTransform, 1);
	const TArray<FInstanceId> Ids = BatchAddInstances(View, bWorldSpace);
	return Ids.Num() > 0 ? Ids[0] : FInstanceId();
}

TArray<UPPHkNavInstancedNavMeshComponent::FInstanceId> UPPHkNavInstancedNavMeshComponent::BatchAddInstances(const TConstArrayView<FTransform> InstanceTransforms, const bool bWorldSpace /*=false*/)
{
	TArray<FInstanceId> OutIds;
	if (!NavMeshDataAsset || !NavMeshDataAsset->PreBakedNavMesh || InstanceTransforms.Num() == 0)
	{
		return OutIds; // 空
	}

	OutIds.Reserve(InstanceTransforms.Num());
	const FTransform ComponentXform = GetComponentTransform();
	for (const FTransform& SrcTransform : InstanceTransforms)
	{
		const FTransform Relative = bWorldSpace ? (SrcTransform.GetRelativeTransform(ComponentXform)) : SrcTransform;

		FInstanceRecord& Record = InstanceRecords.AddDefaulted_GetRef();
		Record.Id = PPHkNav::Private::FInstancedNavMeshComponentInternal::AllocateNewId(this);
		Record.RelativeTransform = Relative;
		Record.LevelInstanceIdOverride = FPPHkNavLevelInstanceIdIssuer::Allocate();
		Record.bPendingLoad = true;

		IdToIndexMap.Add(Record.Id, InstanceRecords.Num() - 1);

		PPHkNav::Private::FInstancedNavMeshComponentInternal::TryLoadInstance(this, Record);

		OutIds.Add(FInstanceId(Record.Id));
	}
	return OutIds;
}

void UPPHkNavInstancedNavMeshComponent::UpdateInstanceTransform(FInstanceId InstanceId,
                                                                   const FTransform& NewTransform,
                                                                   const bool bWorldSpace /*=false*/)
{
	const TConstArrayView<FInstanceId> IdView(&InstanceId, 1);
	const TConstArrayView<FTransform> XformView(&NewTransform, 1);
	BatchUpdateInstanceTransforms(IdView, XformView, bWorldSpace);
}

void UPPHkNavInstancedNavMeshComponent::BatchUpdateInstanceTransforms(
	const TConstArrayView<FInstanceId> InstanceIds, const TConstArrayView<FTransform> NewTransforms,
	const bool bWorldSpace/*=false*/)
{
	if (InstanceIds.Num() != NewTransforms.Num())
	{
		check(InstanceIds.Num() == NewTransforms.Num());
		return; // サイズ不一致
	}
	const FTransform ComponentXform = GetComponentTransform();
	for (int32 i = 0; i < InstanceIds.Num(); ++i)
	{
		const int32 Index = PPHkNav::Private::FInstancedNavMeshComponentInternal::IdToIndex(
			this, InstanceIds[i].Value);
		if (!InstanceRecords.IsValidIndex(Index))
		{
			continue; // 無効 Id
		}
		FInstanceRecord& R = InstanceRecords[Index];
		R.RelativeTransform = bWorldSpace ? (NewTransforms[i].GetRelativeTransform(ComponentXform)) : NewTransforms[i];
		if (IsValid(R.RuntimeInstance))
		{
			R.RuntimeInstance->SetTransform(
				PPHkNav::Private::MakeWorldTransform(ComponentXform, R.RelativeTransform));
		}
	}
}

bool UPPHkNavInstancedNavMeshComponent::RemoveInstance(FInstanceId InstanceId)
{
	const TConstArrayView<FInstanceId> IdView(&InstanceId, 1);
	return BatchRemoveInstances(IdView);
}

bool UPPHkNavInstancedNavMeshComponent::BatchRemoveInstances(const TConstArrayView<FInstanceId> InstanceIds)
{
	bool bAllOk = true;
	for (const FInstanceId& InstanceId : InstanceIds)
	{
		const int32 Index = PPHkNav::Private::FInstancedNavMeshComponentInternal::IdToIndex(this, InstanceId.Value);
		if (!InstanceRecords.IsValidIndex(Index))
		{
			bAllOk = false; // 一部失敗
			continue;
		}
		FInstanceRecord& R = InstanceRecords[Index];
		PPHkNav::Private::FInstancedNavMeshComponentInternal::TryUnloadInstance(this, R);
		IdToIndexMap.Remove(R.Id);

		if (const int32 LastIndex = InstanceRecords.Num() - 1; Index != LastIndex)
		{
			const FInstanceRecord& Moved = InstanceRecords[LastIndex];
			const int32 MovedId = Moved.Id;
			InstanceRecords.Swap(Index, LastIndex);
			InstanceRecords.Pop();
			IdToIndexMap[MovedId] = Index;
		}
		else
		{
			InstanceRecords.Pop();
		}
	}
	return bAllOk;
}

void UPPHkNavInstancedNavMeshComponent::ClearInstances()
{
	PPHkNav::Private::FInstancedNavMeshComponentInternal::TryUnloadAll(this);
	InstanceRecords.Reset();
	IdToIndexMap.Reset();
}

// ===== Lifecycle =====
void UPPHkNavInstancedNavMeshComponent::OnRegister()
{
	Super::OnRegister();
	PPHkNav::Private::FInstancedNavMeshComponentInternal::RefreshAllRuntimeInstanceTransforms(this);
	PPHkNav::Private::FInstancedNavMeshComponentInternal::TryLoadAll(this);
}

void UPPHkNavInstancedNavMeshComponent::OnUnregister()
{
	PPHkNav::Private::FInstancedNavMeshComponentInternal::TryUnloadAll(this);
	Super::OnUnregister();
}

void UPPHkNavInstancedNavMeshComponent::BeginPlay()
{
	Super::BeginPlay();
	PPHkNav::Private::FInstancedNavMeshComponentInternal::TryLoadAll(this);
}

void UPPHkNavInstancedNavMeshComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Deactivate();
	Super::EndPlay(EndPlayReason);
}

void UPPHkNavInstancedNavMeshComponent::Activate(bool bReset)
{
	Super::Activate(bReset);
	if (IsActive())
	{
		PPHkNav::Private::FInstancedNavMeshComponentInternal::TryLoadAll(this);
	}
}

void UPPHkNavInstancedNavMeshComponent::Deactivate()
{
	PPHkNav::Private::FInstancedNavMeshComponentInternal::TryUnloadAll(this);
	Super::Deactivate();
}

void UPPHkNavInstancedNavMeshComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags,
                                                             ETeleportType Teleport)
{
	Super::OnUpdateTransform(UpdateTransformFlags, Teleport);
	PPHkNav::Private::FInstancedNavMeshComponentInternal::RefreshAllRuntimeInstanceTransforms(this);
}

void UPPHkNavInstancedNavMeshComponent::HandleRuntimeInstanceLoaded(UHavokNavNavMeshInstance* LoadedInstance)
{
	if (!LoadedInstance) { return; }
	if (!IsValid(NavMeshDataAsset)){ return; }
	
	FPPHkNavInstancedNavMeshInstanceRecord* FoundRecord = InstanceRecords.FindByPredicate(
		[LoadedInstance](const auto& R) { return R.RuntimeInstance == LoadedInstance; });

	if (!FoundRecord)
	{
		// 管理外のインスタンス
		return;
	}

	if (UPPHkNavInstancedNavMeshUserEdgeGenerator* Gen = GetValid(EdgeGenerator))
	{
		check(!FoundRecord->EdgeSetId); // Edge未生成であるべし
		UHavokNavWorld* HavokNavWorld = PPHkNav::Private::FInstancedNavMeshComponentInternal::GetHavokNavWorld(this);
		FoundRecord->EdgeSetId = Gen->GenerateEdgesForInstanceSocket(HavokNavWorld, LoadedInstance, NavMeshDataAsset->BakedEdgeSockets);
	}
}
