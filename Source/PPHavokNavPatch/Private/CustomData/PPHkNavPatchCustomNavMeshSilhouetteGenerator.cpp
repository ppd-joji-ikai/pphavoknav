// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomData/PPHkNavPatchCustomNavMeshSilhouetteGenerator.h"

//////////////////////////////////////////////////////////////////
///UMassPalNavigation_CustomNavMeshSilhouetteGenerator
//////////////////////////////////////////////////////////////////
void UPPHkNavPatchCustomNavMeshSilhouetteGenerator::SetSilhouettePaintingData(const FPPHkNavPatch_GroupIdFaceData& InFaceData)
{
	FaceData = InFaceData; // コピーして保持する
}

//////////////////////////////////////////////////////////////////
///UMassPalNavigation_CustomNavMeshDynamicCuttingController
//////////////////////////////////////////////////////////////////
EHavokNavNavMeshSilhouetteEffect UPPHkNavPatchCustomNavMeshDynamicCuttingController::ProcessFace(
	FHavokNavAnyConstRef OriginalFaceData, TArrayView<UHavokNavNavMeshSilhouetteGenerator const*> SilhouetteGenerators,
	FHavokNavAnyRef FaceDataOut) const
{
	if (SilhouetteGenerators.IsEmpty())
	{
		return EHavokNavNavMeshSilhouetteEffect::Unchanged;
	}
	
	bool bFaceDataChanged = false;
	for (UHavokNavNavMeshSilhouetteGenerator const* Generator : SilhouetteGenerators)
	{
		if (auto* CustomGenerator = Cast<UPPHkNavPatchCustomNavMeshSilhouetteGenerator>(Generator))
		{
			const FCustomFaceDataType& Original = OriginalFaceData.As<FCustomFaceDataType>();
			const FCustomFaceDataType& Source = CustomGenerator->GetSilhouettePaintingData();
			FCustomFaceDataType& DataOut = FaceDataOut.As<FCustomFaceDataType>();
			if (Original != Source)
			{
				DataOut = Source;
				bFaceDataChanged = true;
			}
		}
		else
		{
			// 一つでも知らないSilhouetteGeneratorがあればCutする
			return EHavokNavNavMeshSilhouetteEffect::Cut;
		}
	}

	return bFaceDataChanged ? EHavokNavNavMeshSilhouetteEffect::ChangeData : EHavokNavNavMeshSilhouetteEffect::Unchanged;
}
