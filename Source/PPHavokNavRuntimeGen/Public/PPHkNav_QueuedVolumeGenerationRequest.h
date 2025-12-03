// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "IPPHkNav_QueuedRequest.h"
#include "PPHkNavHavokNavVolumeGeneratorFactory.h" // For FPPHkNavVolumeGenerationParams
#include "HavokNavNavVolumeGeneratorResult.h"
#include "PPHkNav_QueuedVolumeGenerationRequest.generated.h"

class UPPHkNav_RuntimeGenerationSubsystem;
class UHavokNavNavVolumeGenerator;
class UHavokNavNavVolume;

/**
 * @class UPPHkNav_QueuedVolumeGenerationRequest
 * @brief NavVolume生成リクエストをカプセル化するオブジェクト
 * UHavokNavNavVolumeGeneratorのライフサイクルを管理し、完了コールバックを処理します
 */
UCLASS()
class PPHAVOKNAVRUNTIMEGEN_API UPPHkNav_QueuedVolumeGenerationRequest : public UObject, public IPPHkNav_QueuedRequest
{
	GENERATED_BODY()

public:
	void Initialize(
		FPPHkNavVolumeGenerationParams&& InParams,
		UPPHkNav_RuntimeGenerationSubsystem* InOwnerSubsystem);

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
	void GeneratorCompleted(UHavokNavNavVolumeGenerator* CompletedGenerator, FHavokNavNavVolumeGeneratorResult Result);

private:
	UPROPERTY(Transient)
	TObjectPtr<UPPHkNav_RuntimeGenerationSubsystem> OwnerSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UHavokNavNavVolumeGenerator> Generator;

	FPPHkNavVolumeGenerationParams Params{};

	/** リクエストがキャンセルされているかどうか */
	bool bIsCancelled = false;

	/** リクエストが完了しているかどうか */
	bool bIsFinished = false;
};
