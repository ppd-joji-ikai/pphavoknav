// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HavokNavNavMeshInstance.h"
#include "IPPHkNavMeshInstanceRoleInterface.h"
#include "PPHkNav_NavMeshInstance.generated.h"

/**
 * カスタムNavMeshInstanceのベースクラス
 * 特に実装もデータも持たない
 */
UCLASS()
class PPHKNAVPATCH_API UPPHkNavMeshInstanceBase : public UHavokNavNavMeshInstance
{
	GENERATED_BODY()
};

/**
 * Patch NavMeshInstanceクラス
 */
UCLASS()
class PPHKNAVPATCH_API UPPHkNavPatchNavMeshInstance : public UPPHkNavMeshInstanceBase, public IPPHkNavMeshInstanceRoleInterface
{
	GENERATED_BODY()

public:
	// IPPHkNavMeshInstanceRoleInterface begin
	virtual EPPHkNavMeshInstanceRole GetNavMeshInstanceKind() const override { return EPPHkNavMeshInstanceRole::Patch; }
	virtual bool IsLevelNavMeshInstance() const override { return false; }
	virtual bool IsPatchNavMeshInstance() const override { return true; }
	// IPPHkNavMeshInstanceRoleInterface end
};
