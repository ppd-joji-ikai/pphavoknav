// Fill out your copyright notice in the Description page of Project Settings.


#include "PPHkNavPatchNavMeshFaceFilter_GroupId.h"

#include "CustomData/PPHkNavPatch_CustomData.h"

bool UPPHkNavPatch_NavMeshFaceFilter_GroupId::IsFaceEnabled(FHavokNavAnyConstRef FaceData) const
{
	const auto& GroupIdFaceData = FaceData.As<FPPHkNavPatch_GroupIdFaceData>();
	if (GroupIdFaceData.FaceTraversalState == EPPHkNavPatch_FaceTraversalState::Blocking)
	{
		// グループ問わずブロック中は通れない
		return false;
	}
	
	if (GroupIdFaceData.GroupId != 0 && GroupId != 0
		&& GroupIdFaceData.GroupId != GroupId)
	{
		// グループ指定ならグループ所属のみ
		return false;
	}

	// EMassPalNavigation_FaceTraversalState::Noneは通れてよい
	// グループ指定なしも通れてよい
	return true;
}
