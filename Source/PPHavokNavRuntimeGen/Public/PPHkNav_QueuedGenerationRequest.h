// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "IPPHkNav_QueuedRequest.h"
#include "PPHkNavHavokNavMeshGeneratorFactory.h" // For FPPHkNavMeshGenerationParams
#include "HavokNavNavMeshGeneratorResult.h"
#include "PPHkNav_QueuedGenerationRequest.generated.h"

class UPPHkNav_RuntimeGenerationSubsystem;
class UHavokNavNavMeshGenerator;
class IHavokNavNavMeshConnectionsGenerator;
class UHavokNavNavMesh;
class UHavokNavClusterGraph;
class UHavokNavNavMeshGenerationSettings;

/**
 * @class UPPHkNav_QueuedGenerationRequest
 * @brief NavMesh生成リクエストをカプセル化するオブジェクト
 * UHavokNavNavMeshGeneratorのライフサイクルを管理し、完了コールバックを処理します
 */
UCLASS()
class PPHAVOKNAVRUNTIMEGEN_API UPPHkNav_QueuedGenerationRequest : public UObject, public IPPHkNav_QueuedRequest
{
	GENERATED_BODY()

public:
	void Initialize(
		FPPHkNavMeshGenerationParams&& InParams,
		UPPHkNav_RuntimeGenerationSubsystem* InOwnerSubsystem,
		TScriptInterface<IHavokNavNavMeshConnectionsGenerator> InConnectionsGenerator);

	//~ Begin IPPHkNav_QueuedRequest Interface
	virtual void StartGeneration() override;
	virtual void Cancel() override;
	virtual bool IsFinished() const override;
	virtual bool IsCancelled() const override;
	virtual int32 GetPriority() const override;
	virtual FString GetRequestTypeName() const override;
	//~ End IPPHkNav_QueuedRequest Interface

protected:
	//~ Begin IPPHkNav_QueuedRequest Interface
	virtual void ReportFinished(UPPHkNav_RuntimeGenerationSubsystem* Subsystem) override;
	//~ End IPPHkNav_QueuedRequest Interface

private:
	void GeneratorCompleted(UHavokNavNavMeshGenerator* CompletedGenerator, FHavokNavNavMeshGeneratorResult Result);
	void GenerateConnections(UHavokNavNavMesh* NavMesh, UHavokNavClusterGraph* ClusterGraph) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPPHkNav_RuntimeGenerationSubsystem> OwnerSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UHavokNavNavMeshGenerator> Generator;
	
	UPROPERTY(Transient)
	TScriptInterface<IHavokNavNavMeshConnectionsGenerator> NavMeshConnectionsGenerator;

	UPROPERTY(Transient)
	FPPHkNavMeshGenerationParams Params{};

	/** リクエストがキャンセルされているかどうか */
	UPROPERTY(Transient)
	bool bIsCancelled = false;

	/** リクエストが完了しているかどうか */
	UPROPERTY(Transient)
	bool bIsFinished = false;
};
