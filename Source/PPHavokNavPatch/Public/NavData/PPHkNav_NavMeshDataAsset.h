// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PPHkNavNavMeshUserEdgeSocket.h"
#include "Engine/DataAsset.h"
#include "PPHkNav_NavMeshDataAsset.generated.h"

class UHavokNavNavMesh;

/**
 * 事前ベイク済み UHavokNavNavMesh を保持するデータアセット
 */
UCLASS()
class PPHKNAVPATCH_API UPPHkNav_NavMeshDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * @brief NavMesh が有効かどうか
	 * エッジソケット列は空でもよい
	 * @return == true NavMeshが空でない
	 */
	bool IsValid() const;

	/**
	 * Editor-Only: NavMesh とエッジソケット列の設定
	 * @param NewNavMesh 
	 * @param NewSockets 
	 */
	void SetNewNavMesh(UHavokNavNavMesh* NewNavMesh, TConstArrayView<FPPHkNavBakedNavMeshUserEdgeSocket> NewSockets);

public:
	/** 事前ベイク済み NavMesh アセット */
	UPROPERTY(EditAnywhere)
	TObjectPtr<UHavokNavNavMesh> PreBakedNavMesh{nullptr};

	/** エッジソケット群 */
	UPROPERTY(EditAnywhere)
	TArray<FPPHkNavBakedNavMeshUserEdgeSocket> BakedEdgeSockets{};
};