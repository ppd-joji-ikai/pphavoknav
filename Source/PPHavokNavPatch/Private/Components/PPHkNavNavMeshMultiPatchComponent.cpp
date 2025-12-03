// Copyright Pocketpair, Inc. All Rights Reserved.
#include "PPHkNavNavMeshMultiPatchComponent.h"

#include "HavokNavNavMesh.h"
#include "HavokNavNavMeshInstance.h"
#include "HavokNavNavMeshConnection.h"
#include "HavokNavWorldSubsystem.h"
#include "HavokNavWorld.h"
#include "PPHkNav_NavMeshInstance.h"
#include "ClusterGraph/HavokNavClusterGraph.h"
#include "ClusterGraph/HavokNavClusterGraphUtilities.h"
#include "Engine/World.h"
#include "LevelInstance/LevelInstanceInterface.h"

namespace
{
	// InstanceSet 実装から必要部分のみ抽出
	static void MakePartitionedWorldNavMeshConnectionBindings(UHavokNavWorld* HavokNavWorld, AActor* Actor, UHavokNavNavMesh const* NavMesh, TArray<UHavokNavWorld::FNavMeshConnectionBinding>& BindingsInOut)
	{
		for (UHavokNavNavMeshConnection* Connection : NavMesh->GetNavMeshConnections())
		{
			check(Connection->MatchesGuid(NavMesh->GetGuid()));
#if WITH_EDITOR
			UHavokNavNavMeshInstance* OtherInstance = HavokNavWorld->TryGetNavMeshInstanceInSameWorldPartitionLevelInstance(Actor, Connection->GetOtherNavMeshGuid(NavMesh->GetGuid()));
#else
			UHavokNavNavMeshInstance* OtherInstance = HavokNavWorld->TryGetNavMeshInstanceInWorldPartitionLevelInstance(NavMesh->GetCookedWorldPartitionLevelInstanceId(), Connection->GetOtherNavMeshGuid(NavMesh->GetGuid()));
#endif
			if (OtherInstance)
			{
				BindingsInOut.Add({ OtherInstance, Connection });
			}
		}
	}

	static TArray<UHavokNavWorld::FNavMeshConnectionBinding> MakeNavMeshConnectionBindings(UHavokNavWorld* HavokNavWorld, AActor* Actor, UHavokNavNavMesh const* NavMesh)
	{
		check(Actor->GetWorld());
		ILevelInstanceInterface* OwningLevelInstance = FHavokNavUtilities::GetActorLevelInstance(Actor);
		bool bMakeBindingsForPartitionedWorld = Actor->GetWorld()->IsPartitionedWorld() && !OwningLevelInstance;

		TArray<UHavokNavWorld::FNavMeshConnectionBinding> Bindings;
		if (bMakeBindingsForPartitionedWorld)
		{
			MakePartitionedWorldNavMeshConnectionBindings(HavokNavWorld, Actor, NavMesh, Bindings);
		}
		else
		{
			for (UHavokNavNavMeshConnection* Connection : NavMesh->GetNavMeshConnections())
			{
				check(Connection->MatchesGuid(NavMesh->GetGuid()));
				if (UHavokNavNavMeshInstance* OtherInstance = HavokNavWorld->TryGetNavMeshInstanceInLevelInstance(OwningLevelInstance, Connection->GetOtherNavMeshGuid(NavMesh->GetGuid())))
				{
					Bindings.Add({ OtherInstance, Connection });
				}
			}
		}
		return Bindings;
	}
}

UPPHkNavNavMeshMultiPatchComponent::UPPHkNavNavMeshMultiPatchComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
	bWantsOnUpdateTransform = true;
}

FPPHkNavPatchEntry* UPPHkNavNavMeshMultiPatchComponent::FindEntryByNavMesh(UHavokNavNavMesh* NavMesh)
{
	return PatchEntries.FindByPredicate([NavMesh](FPPHkNavPatchEntry& E){ return E.NavMesh.Get() == NavMesh; });
}

FPPHkNavPatchEntry* UPPHkNavNavMeshMultiPatchComponent::FindEntryByGuid(const FGuid& Guid)
{
	return PatchEntries.FindByPredicate([&Guid](FPPHkNavPatchEntry& E){ return E.Guid == Guid; });
}

void UPPHkNavNavMeshMultiPatchComponent::AddPatchNavMesh(UHavokNavNavMesh* NavMesh, const FTransform& PatchTransform)
{
	if (!NavMesh)
	{
		return ;
	}
	// 既存なら再ロードのみ
	if (FPPHkNavPatchEntry* Existing = FindEntryByNavMesh(NavMesh))
	{
		Existing->Transform = PatchTransform;
		TryLoadEntry(*Existing);
		return ;
	}

	FPPHkNavPatchEntry& NewEntry = PatchEntries.AddDefaulted_GetRef();
	NewEntry.NavMesh = NavMesh;
	NewEntry.Guid = NavMesh->GetGuid();
	NewEntry.Transform = PatchTransform;
	if (IsActive())
	{
		TryLoadEntry(NewEntry);
	}
	return ;
}

void UPPHkNavNavMeshMultiPatchComponent::AddPatchNavMeshWithLevelInstanceId(UHavokNavNavMesh* NavMesh, const FTransform& PatchTransform, uint64 LevelInstanceId)
{
    if (!NavMesh)
    {
        return;
    }
    if (FPPHkNavPatchEntry* Existing = FindEntryByNavMesh(NavMesh))
    {
        Existing->Transform = PatchTransform;
        Existing->LevelInstanceIdOverride = LevelInstanceId;
        Existing->bUseLevelInstanceIdOverride = true;
        TryLoadEntry(*Existing);
        return;
    }

    FPPHkNavPatchEntry& NewEntry = PatchEntries.AddDefaulted_GetRef();
    NewEntry.NavMesh = NavMesh;
    NewEntry.Guid = NavMesh->GetGuid();
    NewEntry.Transform = PatchTransform;
    NewEntry.LevelInstanceIdOverride = LevelInstanceId;
    NewEntry.bUseLevelInstanceIdOverride = true;
    if (IsActive())
    {
        TryLoadEntry(NewEntry);
    }
}

bool UPPHkNavNavMeshMultiPatchComponent::RemovePatchByGuid(const FGuid& NavMeshGuid)
{
	if (FPPHkNavPatchEntry* Entry = FindEntryByGuid(NavMeshGuid))
	{
		RemovePatchByNavMesh(Entry->NavMesh.Get());
		return true;
	}
	return false;
}

bool UPPHkNavNavMeshMultiPatchComponent::RemovePatchByNavMesh(UHavokNavNavMesh* NavMesh)
{
	if (!NavMesh)
	{
		return false;
	}
	if (FPPHkNavPatchEntry* Entry = FindEntryByNavMesh(NavMesh))
	{
		TryUnloadEntry(*Entry, true);
		const int32 Index = static_cast<int32>(Entry - PatchEntries.GetData());
		if (PatchEntries.IsValidIndex(Index))
		{
			PatchEntries.RemoveAtSwap(Index, 1, EAllowShrinking::Yes);
		}
		return true;
	}
	return false;
}

void UPPHkNavNavMeshMultiPatchComponent::ResetPatches()
{
	for (FPPHkNavPatchEntry& Entry : PatchEntries)
	{
		TryUnloadEntry(Entry, true);
	}
	PatchEntries.Reset();
}

TArray<UHavokNavNavMesh*> UPPHkNavNavMeshMultiPatchComponent::GetLoadedNavMeshes() const
{
	TArray<UHavokNavNavMesh*> Result;
	for (const FPPHkNavPatchEntry& Entry : PatchEntries)
	{
		if (UHavokNavNavMesh* NavMesh = Entry.NavMesh.Get())
		{
			Result.Add(NavMesh);
		}
	}
	return Result;
}

void UPPHkNavNavMeshMultiPatchComponent::SetPatchInstancesVelocity(FVector LinearVelocity, FVector AngularVelocity)
{
	for (FPPHkNavPatchEntry& Entry : PatchEntries)
	{
		if (IsValid(Entry.Instance))
		{
			Entry.Instance->SetVelocity(LinearVelocity, AngularVelocity);
		}
	}
}

void UPPHkNavNavMeshMultiPatchComponent::Activate(bool bReset)
{
	Super::Activate(bReset);
	if (IsActive())
	{
		TryLoadAll();
	}
}

void UPPHkNavNavMeshMultiPatchComponent::Deactivate()
{
	TryUnloadAll();
	Super::Deactivate();
}

void UPPHkNavNavMeshMultiPatchComponent::OnRegister()
{
	Super::OnRegister();

	const FTransform& NewTransform = GetComponentTransform();
	for (FPPHkNavPatchEntry& Entry : PatchEntries)
	{
		Entry.Transform = NewTransform;
		if (IsValid(Entry.Instance))
		{
			Entry.Instance->SetTransform(NewTransform);
		}
	}
}

void UPPHkNavNavMeshMultiPatchComponent::OnUnregister()
{
	TryUnloadAll();
	Super::OnUnregister();
}

void UPPHkNavNavMeshMultiPatchComponent::BeginPlay()
{
	Super::BeginPlay();

	ensure(bRegistered);
	if (!bRegistered)
	{
		this->RegisterComponent();
		ensure(bRegistered);
	}
}

void UPPHkNavNavMeshMultiPatchComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Deactivate によりロード済みインスタンスを確実にアンロード
	Deactivate();
	Super::EndPlay(EndPlayReason);
}

void UPPHkNavNavMeshMultiPatchComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	Super::OnUpdateTransform(UpdateTransformFlags, Teleport);
	const FTransform& NewTransform = GetComponentTransform();
	for (FPPHkNavPatchEntry& Entry : PatchEntries)
	{
		Entry.Transform = NewTransform;
		if (IsValid(Entry.Instance))
		{
			Entry.Instance->SetTransform(NewTransform);
		}
	}
}

void UPPHkNavNavMeshMultiPatchComponent::TryLoadEntry(FPPHkNavPatchEntry& Entry)
{
	UHavokNavNavMesh* NavMesh = Entry.NavMesh.Get();
	if (!NavMesh)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World || !World->bIsWorldInitialized || World->WorldType == EWorldType::Inactive)
	{
		return; // 後で再試行
	}
	if (IsValid(Entry.Instance))
	{
		return; // 既にロード済み
	}

	UHavokNavWorldSubsystem* WorldSubsystem = UHavokNavWorldSubsystem::GetInstance(World);
	if (!WorldSubsystem)
	{
		return; // EditorPreview 等
	}

	UHavokNavWorld* HavokNavWorld = WorldSubsystem->HavokNavWorld;
	if (!HavokNavWorld || NavMesh->IsHkNavMeshEmpty())
	{
		return;
	}

	const UHavokNavNavMeshInstance::EBuildStreamingSetsOnLoadMode BuildMode = bBuildAdditionalStreamingSetsOnLoad ?
		UHavokNavNavMeshInstance::EBuildStreamingSetsOnLoadMode::BuildAdditionalStreamingSets :
		UHavokNavNavMeshInstance::EBuildStreamingSetsOnLoadMode::PrecomputedStreamingSetsOnly;

    const uint64 LevelInstanceId = Entry.bUseLevelInstanceIdOverride ? Entry.LevelInstanceIdOverride : FHavokNavUtilities::GetLevelInstanceId(GetOwner(), NavMesh);
	TArray<UHavokNavWorld::FNavMeshConnectionBinding> ConnectionBindings = MakeNavMeshConnectionBindings(HavokNavWorld, GetOwner(), NavMesh);
	Entry.Instance = HavokNavWorld->LoadNavMesh(NavMesh, LevelInstanceId, Entry.Transform, ConnectionBindings, BuildMode, UPPHkNavPatchNavMeshInstance::StaticClass());

	if (IsValid(Entry.Instance) && NavMesh->GetClusterGraph())
	{
		TArray<UHavokNavWorld::FClusterGraphConnectionBinding> ClusterBindings = FHavokNavClusterGraphUtilities::MakeClusterGraphConnectionBindings(HavokNavWorld, GetOwner(), NavMesh->GetClusterGraph());
		Entry.Instance->SetClusterGraphInstance(HavokNavWorld->LoadClusterGraph(NavMesh->GetClusterGraph(), LevelInstanceId, Entry.Transform, ClusterBindings));
	}
}

void UPPHkNavNavMeshMultiPatchComponent::TryUnloadEntry(FPPHkNavPatchEntry& Entry, bool bRemoveInstanceRef)
{
	// 接続の切断 (元 NavMesh データ自体)
	if (UHavokNavNavMesh* NavMesh = Entry.NavMesh.Get())
	{
		NavMesh->RemoveConnectionsFromConnectedNavMeshes();
		if (const UHavokNavClusterGraph* ClusterGraph = FHavokNavClusterGraphUtilities::TryGetClusterGraphForNavMesh(NavMesh))
		{
			ClusterGraph->RemoveConnectionsFromConnectedClusterGraphs();
		}
	}
	if (IsValid(Entry.Instance))
	{
		Entry.Instance->Unload();
	}
	if (bRemoveInstanceRef)
	{
		Entry.Instance = nullptr;
	}
}

void UPPHkNavNavMeshMultiPatchComponent::TryLoadAll()
{
	for (FPPHkNavPatchEntry& Entry : PatchEntries)
	{
		TryLoadEntry(Entry);
	}
}

void UPPHkNavNavMeshMultiPatchComponent::TryUnloadAll()
{
	for (FPPHkNavPatchEntry& Entry : PatchEntries)
	{
		TryUnloadEntry(Entry, false);
	}
	for (FPPHkNavPatchEntry& Entry : PatchEntries)
	{
		Entry.Instance = nullptr;
	}
}
