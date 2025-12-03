// Copyright (c) 2025 Pocketpair, Inc. All Rights Reserved.

#include "PPHkNav_RuntimeNavMeshBoundsVolume.h"

#include "HavokNavNavMesh.h"
#include "PPHkNav_DynamicNavMeshSubsystem.h"
#include "IPPHkNavDynamicNavMeshGenerator.h"
#include "Engine/World.h"
#include "HavokNavNavMeshInstanceSetComponent.h"
#include "ClusterGraph/HavokNavClusterGraph.h"
#include "ClusterGraph/HavokNavClusterGraphUtilities.h"
#include "Components/BrushComponent.h"
#include "PPHkNavNavMeshMultiPatchComponent.h"

APPHkNav_RuntimeNavMeshBoundsVolume::APPHkNav_RuntimeNavMeshBoundsVolume()
    :Super()
{
    PrimaryActorTick.bCanEverTick = false;
    
    // ボリュームはコリジョンなし
    GetBrushComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetBrushComponent()->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
    GetBrushComponent()->SetGenerateOverlapEvents(false);
    
    // ボリュームをエディタ上で表示
    BrushColor = FColor(0, 255, 128, 255);
    bColored = true;
    Super::SetActorHiddenInGame(false);

    // NavMeshインスタンスを保持・表示するためのコンポーネントを作成
    InstanceSetComponent = CreateDefaultSubobject<UHavokNavNavMeshInstanceSetComponent>(TEXT("InstanceSetComponent"));
    InstanceSetComponent->SetupAttachment(GetBrushComponent());
    if (!IsTemplate())
    {
        InstanceSetComponent->OnNavMeshRemoved().AddStatic(&ThisClass::OnNavMeshRemoved);
    }

    // Patch 追加用コンポーネント
    MultiPatchComponent = CreateDefaultSubobject<UPPHkNavNavMeshMultiPatchComponent>(TEXT("MultiPatchComponent"));
    MultiPatchComponent->SetupAttachment(GetBrushComponent());
}

void APPHkNav_RuntimeNavMeshBoundsVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 生成されたNavMeshをクリア
    if (InstanceSetComponent)
    {
        InstanceSetComponent->Reset();
    }
    // MultiPatchでロードしたパッチ群を全てアンロード
    if (MultiPatchComponent)
    {
        MultiPatchComponent->ResetPatches();
    }
    Super::EndPlay(EndPlayReason);
}

void APPHkNav_RuntimeNavMeshBoundsVolume::RequestGeneration()
{
    const UWorld* CurrentWorld = GetWorld();
    if (!CurrentWorld)
    {
        return;
    }

    const UPPHkNav_DynamicNavMeshSubsystem* NavSubsystem = CurrentWorld->GetSubsystem<UPPHkNav_DynamicNavMeshSubsystem>();
    if (!NavSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPPHkNav_DynamicNavMeshSubsystem not found."));
        return;
    }

    UBrushComponent* BrushComp = GetBrushComponent();
    if (!BrushComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("BrushComponent is not valid."));
        return;
    }

    // 生成基点は自アクターとして生成範囲はBrushで定義した範囲とする
    const FTransform ActorTransform = GetActorTransform();
    const FBox WorldBounds = BrushComp->Bounds.GetBox();
    FPPHkNavBoxNavMeshGenerationParams Params{};
    Params.GenerationBox = WorldBounds;
    Params.Layer = Layer;
    Params.ControllerOverride = ControllerOverride;
    Params.UpVector = GetActorUpVector();
    Params.GenerationTransform = ActorTransform;
    Params.GenerationSettingsOverride = nullptr; // 明示設定 (デフォルト設定使用)
    Params.OnGeneratedCallback = [WeakThis = TWeakObjectPtr(this), ActorTransform](const FHavokNavNavMeshGeneratorResult& Result)
    {
        if (APPHkNav_RuntimeNavMeshBoundsVolume* This = WeakThis.Get())
        {
            constexpr bool bIsBaseNavMesh = true;
            This->OnNavMeshGenerationCompleted(Result, bIsBaseNavMesh, ActorTransform);
        }
    };
    NavSubsystem->GetGeneratorInterface()->RequestBoxNavMeshGeneration(MoveTemp(Params));
}

void APPHkNav_RuntimeNavMeshBoundsVolume::RequestPatchNavMeshGeneration(const FTransform& PatchTransform, const FBox& WorldPatchBox)
{
    const UWorld* CurrentWorld = GetWorld();
    if (!CurrentWorld)
    {
        return;
    }
    const UPPHkNav_DynamicNavMeshSubsystem* NavSubsystem = CurrentWorld->GetSubsystem<UPPHkNav_DynamicNavMeshSubsystem>();
    if (!NavSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPPHkNav_DynamicNavMeshSubsystem not found (Patch)."));
        return;
    }

    FPPHkNavBoxNavMeshGenerationParams Params{};
    Params.GenerationBox = WorldPatchBox;
    Params.Layer = Layer;
    Params.ControllerOverride = ControllerOverride;
    Params.UpVector = PatchTransform.GetUnitAxis(EAxis::Z);
    Params.GenerationTransform = PatchTransform;
    Params.GenerationSettingsOverride = nullptr; // 明示設定 (デフォルト設定使用)
    Params.OnGeneratedCallback = [WeakThis = TWeakObjectPtr(this), PatchTransform](const FHavokNavNavMeshGeneratorResult& Result)
    {
        if (APPHkNav_RuntimeNavMeshBoundsVolume* This = WeakThis.Get())
        {
            constexpr bool bIsBaseNavMesh = false;
            This->OnNavMeshGenerationCompleted(Result, bIsBaseNavMesh, PatchTransform);
        }
    };
    NavSubsystem->GetGeneratorInterface()->RequestBoxNavMeshGeneration(MoveTemp(Params));
}

bool APPHkNav_RuntimeNavMeshBoundsVolume::RemovePatchNavMeshByGuid(const FGuid& PatchGuid)
{
    if (!MultiPatchComponent)
    {
        return false;
    }
    return MultiPatchComponent->RemovePatchByGuid(PatchGuid);
}

void APPHkNav_RuntimeNavMeshBoundsVolume::RemoveAllPatchNavMesh()
{
    if (MultiPatchComponent)
    {
        MultiPatchComponent->ResetPatches();
    }
}

void APPHkNav_RuntimeNavMeshBoundsVolume::OnNavMeshGenerationCompleted(const FHavokNavNavMeshGeneratorResult& Result, const bool bIsBaseNavMesh, const FTransform& GenerationTransform)
{
    if (!Result.NavMesh)
    {
        return;
    }
    if (Result.ClusterGraph)
    {
        Result.NavMesh->SetClusterGraph(Result.ClusterGraph);
    }

    if (bIsBaseNavMesh)
    {
        // ベースNavMesh (従来挙動) : Layer 単位で置換 (必要なら)
        Result.NavMesh->Rename(nullptr, InstanceSetComponent);
        if (InstanceSetComponent)
        {
            InstanceSetComponent->AddBaseNavMesh(Result.NavMesh);
        }
        UE_LOG(LogTemp, Verbose, TEXT("Base NavMesh generated for %s."), *GetName());
    }
    else
    {
        // パッチNavMesh (増分)
        Result.NavMesh->Rename(nullptr, MultiPatchComponent);
        if (MultiPatchComponent)
        {
            MultiPatchComponent->AddPatchNavMesh(Result.NavMesh, GenerationTransform);
        }
        UE_LOG(LogTemp, Verbose, TEXT("Patch NavMesh generated for %s."), *GetName());
    }
}

void APPHkNav_RuntimeNavMeshBoundsVolume::OnNavMeshRemoved(UHavokNavNavMeshInstanceSetComponent* Component, UHavokNavNavMesh* NavMeshBeingRemoved)
{
    NavMeshBeingRemoved->RemoveConnectionsFromConnectedNavMeshes();

    if (const UHavokNavClusterGraph* ClusterGraph = FHavokNavClusterGraphUtilities::TryGetClusterGraphForNavMesh(NavMeshBeingRemoved))
    {
        ClusterGraph->RemoveConnectionsFromConnectedClusterGraphs();
    }
}
