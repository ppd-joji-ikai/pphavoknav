// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IPPHkNav_QueuedRequest.generated.h"

class UPPHkNav_RuntimeGenerationSubsystem;

UINTERFACE(MinimalAPI)
class UPPHkNav_QueuedRequest : public UInterface
{
	GENERATED_BODY()
};

/**
 * @interface IPPHkNav_QueuedRequest
 * @brief ナビゲーション生成リクエストの共通インターフェース
 * NavMeshとNavVolumeの両方のリクエストを統一的に処理するためのインターフェース
 */
class PPHAVOKNAVRUNTIMEGEN_API IPPHkNav_QueuedRequest
{
	GENERATED_BODY()

public:
	/**
	 * @brief リクエストの生成を開始します
	 */
	virtual void StartGeneration() = 0;

	/**
	 * @brief リクエストをキャンセルします
	 */
	virtual void Cancel() = 0;

	/**
	 * @brief リクエストが完了しているかどうかを確認します
	 * @return 完了している場合true
	 */
	virtual bool IsFinished() const = 0;

	/**
	 * @brief リクエストがキャンセルされているかどうかを確認します
	 * @return キャンセルされている場合true
	 */
	virtual bool IsCancelled() const = 0;

	/**
	 * @brief リクエストの優先度を取得します
	 * @return 優先度（数値が小さいほど高優先度）
	 */
	virtual int32 GetPriority() const = 0;

	/**
	 * @brief リクエストタイプの名前を取得します（デバッグ用）
	 * @return リクエストタイプ名
	 */
	virtual FString GetRequestTypeName() const = 0;

protected:
	/**
	 * @brief 生成完了を報告します
	 * @param Subsystem 報告先のサブシステム
	 */
	virtual void ReportFinished(UPPHkNav_RuntimeGenerationSubsystem* Subsystem) = 0;
};
