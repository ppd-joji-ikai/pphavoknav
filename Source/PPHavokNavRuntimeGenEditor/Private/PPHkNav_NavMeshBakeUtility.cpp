// Copyright Pocketpair, Inc. All Rights Reserved.

#include "PPHkNav_NavMeshBakeUtility.h"

#include "Editor.h"
#include "HavokNavNavMesh.h"
#include "HavokNavNavMeshGenerationSettings.h"
#include "IPPHkNavDynamicNavMeshGenerator.h"
#include "PPHkNavRuntimeGenEditorSettings.h"
#include "PPHkNav_DynamicNavMeshSubsystem.h"
#include "PPHkNav_NavMeshDataAsset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Components/SceneComponent.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Actor.h"
#include "Logging/StructuredLog.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "PPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent.h"
#include "Misc/DateTime.h"

void UPPHkNav_NavMeshBakeUtility::BakeSelected()
{
#if WITH_EDITOR
    if(!GEditor)
    {
        UE_LOG(LogTemp, Warning, TEXT("GEditor invalid"));
        return;
    }

    // デフォルト設定の適用
    ApplyDefaultsIfNeeded();

    if (Layer == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("Layer is null"));
        return;
    }

    // Content Browser で Blueprint 選択
    int32 BakedBlueprintCount = 0;
    TArray<FAssetData> SelectedAssets;
    GEditor->GetContentBrowserSelections(SelectedAssets);
    for(const FAssetData& AssetData : SelectedAssets)
    {
        const UClass* AssetClass = AssetData.GetClass();
        if(!AssetClass || !AssetClass->IsChildOf(UBlueprint::StaticClass()))
        {
            continue;
        }
        if(UBlueprint* BP = Cast<UBlueprint>(AssetData.GetAsset()))
        {
            HandleBlueprintBake(*BP);
            ++BakedBlueprintCount;
        }
    }

    if(BakedBlueprintCount == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid Actor / Blueprint selected for NavMesh bake."));
        return;
    }

    // 全ジョブ完了まで待機
    WaitUntilAllJobsFinished(BakedBlueprintCount);

    UE_LOG(LogTemp, Log, TEXT("BakeSelected queued generation jobs. Blueprints: %d"), BakedBlueprintCount);
#endif
}

void UPPHkNav_NavMeshBakeUtility::BakeBlueprint(UBlueprint* InBlueprint)
{
#if WITH_EDITOR
    // 入力 / エディタ状態ガード
    if(!GEditor)
    {
        UE_LOG(LogTemp, Warning, TEXT("BakeBlueprint: GEditor invalid."));
        return;
    }
    if(!InBlueprint)
    {
        UE_LOG(LogTemp, Warning, TEXT("BakeBlueprint: InBlueprint is null."));
        return;
    }

    // デフォルト設定適用
    ApplyDefaultsIfNeeded();
    if(!Layer)
    {
        UE_LOG(LogTemp, Warning, TEXT("BakeBlueprint: Layer is null."));
        return;
    }

    HandleBlueprintBake(*InBlueprint);

    // 全ジョブ完了まで待機
    WaitUntilAllJobsFinished(1);

    UE_LOG(LogTemp, Log, TEXT("BakeBlueprint queued generation job. Blueprint: %s"), *InBlueprint->GetName());
#endif // WITH_EDITOR
}

void UPPHkNav_NavMeshBakeUtility::ApplyDefaultsIfNeeded()
{
    const UPPHkNavRuntimeGenEditorSettings* Settings = GetDefault<UPPHkNavRuntimeGenEditorSettings>();
    check(Settings);

    // Layer
    if(!Layer)
    {
        if(Settings->DefaultNavMeshLayer.IsValid())
        {
            Layer = Settings->DefaultNavMeshLayer.Get();
        }
        else if(Settings->DefaultNavMeshLayer.IsNull() == false)
        {
            // 未ロードなら同期ロード
            Layer = Settings->DefaultNavMeshLayer.LoadSynchronous();
        }
    }

    // ControllerOverride
    if (ControllerOverride.GenerationControllerClass == nullptr)
    {
        ControllerOverride = Settings->DefaultControllerOverride;
    }

    // GenerationSettingsOverride
    if (!GenerationSettingsOverride)
    {
        if (Settings->DefaultGenerationSettingsOverride.IsNull() == false)
        {
            GenerationSettingsOverride = Settings->DefaultGenerationSettingsOverride.LoadSynchronous();
        }
    }
    if(TargetFolderPath.IsEmpty()) { TargetFolderPath = Settings->DefaultTargetFolderPath; }
    if(AssetNamePrefix.IsEmpty())  { AssetNamePrefix  = Settings->DefaultAssetNamePrefix; }
}

void UPPHkNav_NavMeshBakeUtility::WaitUntilAllJobsFinished(const int32 TotalJobs)
{
#if WITH_EDITOR
    if (TotalJobs <= 0)
    {
        return;
    }

    FScopedSlowTask SlowTask(static_cast<float>(TotalJobs),
        NSLOCTEXT("PPHkNav", "BakeNavMeshCaption", "Baking NavMesh..."), true);
    SlowTask.MakeDialog(/*bShowCancelButton*/false, /*bAllowInPIE*/false);

    int32 LastReportedCompleted = 0;
    while (PendingJobsCount > 0)
    {
        const int32 Completed = TotalJobs - PendingJobsCount;
        if (Completed > LastReportedCompleted)
        {
            const int32 Delta = Completed - LastReportedCompleted;
            SlowTask.EnterProgressFrame(static_cast<float>(Delta),
                FText::Format(NSLOCTEXT("PPHkNav", "BakeNavMeshProgressFmt", "Completed {0} / {1}"), Completed, TotalJobs));
            LastReportedCompleted = Completed;
        }

        if (SlowTask.ShouldCancel())
        {
            break;
        }

        FPlatformProcess::Sleep(0.01f);
    }
#endif
}

void UPPHkNav_NavMeshBakeUtility::HandleBlueprintBake(UBlueprint& Blueprint)
{
#if WITH_EDITOR
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if(!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("Editor world not found for blueprint bake."));
        return;
    }
    UClass* GeneratedClass = Blueprint.GeneratedClass;
    if(!GeneratedClass || !GeneratedClass->IsChildOf(AActor::StaticClass()))
    {
        UE_LOG(LogTemp, Warning, TEXT("Blueprint is not actor based: %s"), *Blueprint.GetName());
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.ObjectFlags = RF_Transient;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* TempActor = World->SpawnActor<AActor>(GeneratedClass, FTransform::Identity, SpawnParams);
    if(!TempActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to spawn temp actor for blueprint: %s"), *Blueprint.GetName());
        return;
    }
    TempActor->SetActorLabel(FString::Format(TEXT("TempBake_{0}"), { *Blueprint.GetName() }), false);
    
    const FBox BoundsWorld = ComputeActorBoundsWorld(*TempActor);
    if(!BoundsWorld.IsValid)
    {
        UE_LOG(LogTemp, Warning, TEXT("Blueprint bounds invalid: %s"), *Blueprint.GetName()); TempActor->Destroy();
        return;
    }

    // ソケット収集
    TInlineComponentArray<UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly*> MultiSocketComponents;
    TempActor->GetComponents(MultiSocketComponents);

    TArray<FPPHkNavBakedNavMeshUserEdgeSocket> BakedSockets;
    BakedSockets.Reserve(MultiSocketComponents.Num() * 4); // 適当な余裕確保

    const FTransform ActorWorld = TempActor->GetActorTransform();

    auto ConvertAndAdd = [&BakedSockets, &ActorWorld](const FPPHkNavBakedNavMeshUserEdgeSocket& Src, const FTransform& CompWorld)
    {
        FPPHkNavBakedNavMeshUserEdgeSocket Dst = Src; // 基本コピー
        // Start/End の位置をワールド化後 Actor 相対へ
        const FVector StartWorldPos = CompWorld.TransformPosition(Src.CenterStart);
        const FVector EndWorldPos   = CompWorld.TransformPosition(Src.CenterEnd);
        Dst.CenterStart = ActorWorld.InverseTransformPosition(StartWorldPos);
        Dst.CenterEnd   = ActorWorld.InverseTransformPosition(EndWorldPos);

        // Forward も Actor 相対ベクトルへ
        const FVector StartWorldFwd = CompWorld.TransformVectorNoScale(Src.ForwardStart).GetSafeNormal();
        const FVector EndWorldFwd   = CompWorld.TransformVectorNoScale(Src.ForwardEnd).GetSafeNormal();
        Dst.ForwardStart = ActorWorld.InverseTransformVectorNoScale(StartWorldFwd).GetSafeNormal();
        Dst.ForwardEnd   = ActorWorld.InverseTransformVectorNoScale(EndWorldFwd).GetSafeNormal();

        // Up ベクトルはワールド指定のまま正規化して格納
        const FVector StartWorldUp  = (Src.UpVectorStart).GetSafeNormal();
        const FVector EndWorldUp    = (Src.UpVectorEnd).GetSafeNormal();
        Dst.UpVectorStart= StartWorldUp;
        Dst.UpVectorEnd  = EndWorldUp;

        BakedSockets.Add(MoveTemp(Dst));
    };

    if(MultiSocketComponents.Num() > 0)
    {
        for(UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly* Comp : MultiSocketComponents)
        {
            if(!Comp) { continue; }
            const TArray<FPPHkNavBakedNavMeshUserEdgeSocket>& Arr = Comp->GetSocketDefinitions();
            const FTransform CompWorld = Comp->GetComponentTransform();
            for(const FPPHkNavBakedNavMeshUserEdgeSocket& S : Arr)
            {
                ConvertAndAdd(S, CompWorld);
            }
        }
    }

    IssueActorGenerationRequest(*TempActor, Blueprint.GetName(), BoundsWorld, MoveTemp(BakedSockets));
    TempActor->Destroy();
#endif
}

FBox UPPHkNav_NavMeshBakeUtility::ComputeActorBoundsWorld(const AActor& TargetActor) const
{
    FBox AccumBox(ForceInit);
    TArray<USceneComponent*> Components;
    TargetActor.GetComponents(Components);
    for(const USceneComponent* Comp : Components)
    {
        if(!Comp || !Comp->IsRegistered())
        {
            continue;
        }

        // NavMesh に影響しないコンポーネントを除外
        if (!Comp->CanEverAffectNavigation())
        {
            continue;
        }
        
        const FBoxSphereBounds B = Comp->Bounds;
        if(B.BoxExtent.IsNearlyZero())
        {
            continue;
        }
        AccumBox += B.GetBox();
    }

    // マージン: Actorの場合不要なはずだが念のため. Erosionに影響するかもしれない
    if(AccumBox.IsValid)
    {
        const FVector Margin = FVector(50.0f, 50.0f, 50.0f);
        AccumBox = FBox(AccumBox.Min - Margin, AccumBox.Max + Margin);
    }
    return AccumBox;
}

bool UPPHkNav_NavMeshBakeUtility::PrepareNamePairs(const FString& BaseName,
                                                      FString& OutNavMeshPackagePath, FString& OutNavMeshObjName,
                                                      FString& OutDataPackagePath, FString& OutDataObjName) const
{
    if(TargetFolderPath.IsEmpty())
    {
        return false;
    }

    OutNavMeshPackagePath = FString::Printf(TEXT("%s/%s%s_NavMesh"), *TargetFolderPath, *AssetNamePrefix, *BaseName);
    OutDataPackagePath   = FString::Printf(TEXT("%s/%s%s_Data"),    *TargetFolderPath, *AssetNamePrefix, *BaseName);
    OutNavMeshObjName = FPackageName::GetLongPackageAssetName(OutNavMeshPackagePath);
    OutDataObjName    = FPackageName::GetLongPackageAssetName(OutDataPackagePath);

    if(!FPackageName::IsValidLongPackageName(OutNavMeshPackagePath) ||
       !FPackageName::IsValidLongPackageName(OutDataPackagePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid package path: %s / %s"), *OutNavMeshPackagePath, *OutDataPackagePath);
        return false;
    }
    return true;
}

void UPPHkNav_NavMeshBakeUtility::IssueActorGenerationRequest(AActor& SpawnedActor, const FString& BaseName, const FBox& BoundsWorld, TArray<FPPHkNavBakedNavMeshUserEdgeSocket>&& BakedSockets)
{
#if WITH_EDITOR
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if(!World)
    {
        UE_LOG(LogTemp, Error, TEXT("Editor world invalid."));
        return;
    }

    const UPPHkNav_DynamicNavMeshSubsystem* Subsystem = World->GetSubsystem<UPPHkNav_DynamicNavMeshSubsystem>();
    if(!Subsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("UPPHkNav_DynamicNavMeshSubsystem not found."));
        return;
    }

    IPPHkNavDynamicNavMeshGenerator* Generator = Subsystem->GetGeneratorInterface();
    if(!Generator)
    {
        UE_LOG(LogTemp, Error, TEXT("NavMesh generator interface invalid."));
        return;
    }

    FString NavMeshPackagePath;
    FString DataPackagePath;
    FString NavMeshObjName;
    FString DataObjName;
    if(!PrepareNamePairs(BaseName, NavMeshPackagePath, NavMeshObjName, DataPackagePath, DataObjName))
    {
        return;
    }

    FPPHkNavActorNavMeshGenerationParams Params{};
    Params.RequestingActor = &SpawnedActor;
    Params.Layer = Layer;
    Params.ControllerOverride = ControllerOverride;
    Params.OnGeneratedCallback =
        [WeakThis = TWeakObjectPtr(this),
         NavMeshPackagePath,
         NavMeshObjName,
         DataPackagePath,
         DataObjName,
         BakedSockets = MoveTemp(BakedSockets)](const FHavokNavNavMeshGeneratorResult& Result)
        {
            if(UPPHkNav_NavMeshBakeUtility* This = WeakThis.Get())
            {
                This->OnGenerationFinished(Result, NavMeshPackagePath, NavMeshObjName, DataPackagePath, DataObjName,BakedSockets);
            }
        };

    ++PendingJobsCount;
    Generator->RequestActorNavMeshGeneration(MoveTemp(Params));

    UE_LOGFMT(LogTemp, Log, "Actor-based NavMesh generation requested: {Name} Bounds: {BoundsWorld}", *BaseName, *BoundsWorld.ToString());
#endif
}

void UPPHkNav_NavMeshBakeUtility::IssueBoxGenerationRequest(const FString& BaseName, const FBox& WorldBounds, const FTransform& GenerationTransform)
{
#if WITH_EDITOR
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if(!World)
    {
        UE_LOG(LogTemp, Error, TEXT("Editor world invalid."));
        return;
    }

    const UPPHkNav_DynamicNavMeshSubsystem* Subsystem = World->GetSubsystem<UPPHkNav_DynamicNavMeshSubsystem>();
    if(!Subsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("UPPHkNav_DynamicNavMeshSubsystem not found."));
        return;
    }

    IPPHkNavDynamicNavMeshGenerator* Generator = Subsystem->GetGeneratorInterface();
    if(!Generator)
    {
        UE_LOG(LogTemp, Error, TEXT("NavMesh generator interface invalid."));
        return;
    }

    FString NavMeshPackagePath;
    FString DataPackagePath;
    FString NavMeshObjName;
    FString DataObjName;
    if(!PrepareNamePairs(BaseName, NavMeshPackagePath, NavMeshObjName, DataPackagePath, DataObjName))
    {
        return;
    }

    FPPHkNavBoxNavMeshGenerationParams Params{};
    Params.GenerationBox = WorldBounds;
    Params.Layer = Layer;
    Params.ControllerOverride = ControllerOverride;
    Params.UpVector = GenerationTransform.GetUnitAxis(EAxis::Z);
    Params.GenerationTransform = GenerationTransform;
    Params.GenerationSettingsOverride = GenerationSettingsOverride;
    Params.OnGeneratedCallback =
        [WeakThis = TWeakObjectPtr(this),
         NavMeshPackagePath,
         NavMeshObjName,
         DataPackagePath,
         DataObjName](const FHavokNavNavMeshGeneratorResult& Result)
        {
            if(UPPHkNav_NavMeshBakeUtility* This = WeakThis.Get())
            {
                TArray<FPPHkNavBakedNavMeshUserEdgeSocket> BakedSocket;
                This->OnGenerationFinished(Result, NavMeshPackagePath, NavMeshObjName, DataPackagePath, DataObjName, BakedSocket);
            }
        };

    ++PendingJobsCount;
    Generator->RequestBoxNavMeshGeneration(MoveTemp(Params));
    UE_LOG(LogTemp, Log, TEXT("NavMesh generation requested: %s"), *BaseName);
#endif
}

void UPPHkNav_NavMeshBakeUtility::OnGenerationFinished(const FHavokNavNavMeshGeneratorResult& Result,
                                                          const FString NavMeshPackagePath,
                                                          const FString NavMeshObjName,
                                                          const FString DataPackagePath,
                                                          const FString DataObjName,
                                                          const TArray<FPPHkNavBakedNavMeshUserEdgeSocket>& BakedSockets)
{
    --PendingJobsCount;

    if(!Result.NavMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("Generation result invalid."));
        return;
    }
    if(Result.NavMesh->IsHkNavMeshEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Generation NavMesh is empty."));
        return;
    }

    // 再利用フロー: 既存 DataAsset を探す
    UPackage* ExistingDataPkg = FindPackage(nullptr, *DataPackagePath);
    UPPHkNav_NavMeshDataAsset* ExistingDataAsset = nullptr;
    if(ExistingDataPkg)
    {
        ExistingDataAsset = FindObject<UPPHkNav_NavMeshDataAsset>(ExistingDataPkg, *DataObjName);
    }

    if(const bool bCanReuse = ExistingDataAsset != nullptr)
    {
        // NavMesh パッケージ (存在しなければ作成)
        UPackage* NavMeshPackage = CreatePackage(*NavMeshPackagePath);
        if(!NavMeshPackage)
        {
            UE_LOG(LogTemp, Error, TEXT("Package creation failed (reuse)."));
            return;
        }

        // 旧 NavMesh のアーカイブ (名前確保)
        if(ExistingDataAsset->PreBakedNavMesh)
        {
            UObject* OldNavMesh = ExistingDataAsset->PreBakedNavMesh;
            const FString TimeSuffix = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
            const FString ArchiveName = FString::Printf(TEXT("%s_Archived_%s"), *OldNavMesh->GetName(), *TimeSuffix);
            OldNavMesh->Rename(*ArchiveName, OldNavMesh->GetOuter(), REN_DontCreateRedirectors | REN_DoNotDirty);
            OldNavMesh->SetFlags(RF_Transient); // パッケージ保存対象から除外
        }

        // 既に同名のオブジェクトが残っていないか保険 (万一残っていたらリネーム)
        if(UObject* Conflict = FindObject<UObject>(NavMeshPackage, *NavMeshObjName))
        {
            const FString AltName = FString::Printf(TEXT("%s_Obsolete_%s"), *NavMeshObjName, *FDateTime::Now().ToString(TEXT("%H%M%S")));
            Conflict->Rename(*AltName, Conflict->GetOuter(), REN_DontCreateRedirectors | REN_DoNotDirty);
        }

        UHavokNavNavMesh* DuplicatedNavMesh = DuplicateObject<UHavokNavNavMesh>(Result.NavMesh, NavMeshPackage, *NavMeshObjName);
        if(!DuplicatedNavMesh)
        {
            UE_LOG(LogTemp, Error, TEXT("NavMesh duplicate failed (reuse)."));
            return;
        }
        DuplicatedNavMesh->SetFlags(RF_Public | RF_Standalone);
        NavMeshPackage->MarkPackageDirty();

        ExistingDataAsset->SetNewNavMesh(DuplicatedNavMesh, BakedSockets);
        ExistingDataAsset->MarkPackageDirty();

        FAssetRegistryModule::AssetCreated(DuplicatedNavMesh); // DataAsset は既存なので登録不要
        UE_LOG(LogTemp, Log, TEXT("Reused DataAsset: %s  Updated NavMesh: %s"), *ExistingDataAsset->GetPathName(), *DuplicatedNavMesh->GetPathName());

        if(PendingJobsCount <= 0)
        {
            UE_LOG(LogTemp, Log, TEXT("All nav mesh bake jobs finished (reuse)."));
        }
        return;
    }

    // 新規作成フロー
    UPackage* NavMeshPackage = CreatePackage(*NavMeshPackagePath);
    UPackage* DataAssetPackage = CreatePackage(*DataPackagePath);
    if(!NavMeshPackage || !DataAssetPackage)
    {
        UE_LOG(LogTemp, Error, TEXT("Package creation failed."));
        return;
    }

    UHavokNavNavMesh* DuplicatedNavMesh = DuplicateObject<UHavokNavNavMesh>(Result.NavMesh, NavMeshPackage, *NavMeshObjName);
    if(!DuplicatedNavMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("NavMesh duplicate failed."));
        return;
    }
    DuplicatedNavMesh->SetFlags(RF_Public | RF_Standalone);
    NavMeshPackage->MarkPackageDirty();

    UPPHkNav_NavMeshDataAsset* DataAsset = NewObject<UPPHkNav_NavMeshDataAsset>(DataAssetPackage, *DataObjName, RF_Public | RF_Standalone);
    if(!ensure(DataAsset)) { return; }
    DataAsset->SetNewNavMesh(DuplicatedNavMesh, BakedSockets);
    DataAssetPackage->MarkPackageDirty();

    FAssetRegistryModule::AssetCreated(DuplicatedNavMesh);
    FAssetRegistryModule::AssetCreated(DataAsset);

    UE_LOG(LogTemp, Log, TEXT("Created NavMesh/DataAsset: %s / %s"), *DuplicatedNavMesh->GetPathName(), *DataAsset->GetPathName());

    if(PendingJobsCount <= 0)
    {
        UE_LOG(LogTemp, Log, TEXT("All nav mesh bake jobs finished."));
    }
}
