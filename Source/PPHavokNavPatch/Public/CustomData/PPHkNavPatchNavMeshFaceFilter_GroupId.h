// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HavokNavNavMeshFaceFilter.h"

#include "PPHkNavPatchNavMeshFaceFilter_GroupId.generated.h"

/**
 * グループごとのNavMeshFaceFilter
 * Havokスレッドから並列に実行されうることに注意.
 */
UCLASS(Blueprintable, editinlinenew, meta = (DisplayName = "PPHkNav - Nav Mesh Face Filter Group ID"))
class PPHAVOKNAVPATCH_API UPPHkNavPatch_NavMeshFaceFilter_GroupId : public UHavokNavNavMeshFaceFilter
{
	GENERATED_BODY()

public:
	virtual bool IsFaceEnabled(FHavokNavAnyConstRef FaceData) const override;

	const uint8& GetGroupId_Concurrent() const
	{
		return GroupId;
	}

	void SetGroupId_Concurrent(const uint8& InGroupId)
	{
		GroupId = InGroupId;
	}
protected:
	uint8 GroupId{};
};
