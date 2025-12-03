// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IPPHkNavMeshInstanceRoleInterface.generated.h"

/** NavMeshインスタンス役割
 * LevelNavMeshInstance - レベルに紐づくNavMeshInstance. 静的に生成されてロード時にインスタンス化されるもの. 1Volumeにつき1つ
 * PatchNavMeshInstance - 動的に生成されるNavMeshInstance. 主に建築物に利用される. Volumeに紐づかずに管理される
 * */
UENUM(BlueprintType)
enum class EPPHkNavMeshInstanceRole : uint8
{
	None   UMETA(DisplayName="None"),
	Level  UMETA(DisplayName="Level"),
	Patch  UMETA(DisplayName="Patch")
};

UINTERFACE()
class UPPHkNavMeshInstanceRoleInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * NavMeshInstance 役割取得インターフェース
 */
class PPHKNAVPATCH_API IPPHkNavMeshInstanceRoleInterface
{
	GENERATED_BODY()

public:
	virtual EPPHkNavMeshInstanceRole GetNavMeshInstanceKind() const = 0;
	virtual bool IsLevelNavMeshInstance() const = 0;
	virtual bool IsPatchNavMeshInstance() const = 0;
};
