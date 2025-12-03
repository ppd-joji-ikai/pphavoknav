// Copyright Pocketpair, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PPHkNavHavokNavMeshGeneratorFactory.h"
//#include "PPHkNavNavMeshUserEdgeSocket.h"
#include "PPHkNav_NavMeshBakeUtility.generated.h"

struct FPPHkNavBakedNavMeshUserEdgeSocket;
class UBlueprint;
class UHavokNavNavMesh;
class UHavokNavNavMeshGenerationSettings;
class UHavokNavNavMeshLayer;
class UPPHkNav_DynamicNavMeshSubsystem;
class UPPHkNavNavMeshUserEdgeSocketAuthoringComponent; // フォワード

/**
 * Blueprint / Actor 選択対象から NavMesh を事前ベイクし DataAsset 化するエディタユーティリティ
 */
UCLASS(BlueprintType)
class PPHAVOKNAVRUNTIMEGENEDITOR_API UPPHkNav_NavMeshBakeUtility : public UObject
{
    GENERATED_BODY()

public:
    /**
     * 選択中 (Actor or Blueprint) から NavMesh をベイク
     */
    UFUNCTION(CallInEditor, BlueprintCallable, Category="PPHkNav|NavMeshBake")
    void BakeSelected();

    void BakeBlueprint(UBlueprint* InBlueprint);
    
private:
    void ApplyDefaultsIfNeeded();
    void WaitUntilAllJobsFinished(const int32 TotalJobs);
    
    void HandleBlueprintBake(UBlueprint& Blueprint);
    FBox ComputeActorBoundsWorld(const AActor& TargetActor) const;
    bool PrepareNamePairs(const FString& BaseName, FString& OutNavMeshPackagePath, FString& OutNavMeshObjName,
                          FString& OutDataPackagePath, FString& OutDataObjName) const;
    void IssueActorGenerationRequest(AActor& SpawnedActor, const FString& BaseName, const FBox& BoundsWorld,
                                     TArray<FPPHkNavBakedNavMeshUserEdgeSocket>&& BakedSockets);
    void IssueBoxGenerationRequest(const FString& BaseName, const FBox& WorldBounds, const FTransform& GenerationTransform);
    void OnGenerationFinished(const FHavokNavNavMeshGeneratorResult& Result,
                              const FString NavMeshPackagePath,
                              const FString NavMeshObjName,
                              const FString DataPackagePath,
                              const FString DataObjName,
                              const TArray<FPPHkNavBakedNavMeshUserEdgeSocket>& BakedSockets);

protected:
    /** 生成対象 Layer */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PPHkNav|NavMeshBake")
    TSubclassOf<UHavokNavNavMeshLayer> Layer{};

    /** 生成コントローラのオーバーライド */
    UPROPERTY(EditAnywhere, Category="PPHkNav|NavMeshBake")
    FHavokNavNavMeshGenerationControllerOverride ControllerOverride{};

    /** 生成設定のオーバーライド */
    UPROPERTY(EditAnywhere, Category = "Generation")
    TObjectPtr<const UHavokNavNavMeshGenerationSettings> GenerationSettingsOverride{};
    
    /** 保存先フォルダ (/Game/...) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PPHkNav|NavMeshBake")
    FString TargetFolderPath{};

    /** 生成アセット名接頭辞 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PPHkNav|NavMeshBake")
    FString AssetNamePrefix{};

    UPROPERTY(Transient)
    int32 PendingJobsCount{0};
};