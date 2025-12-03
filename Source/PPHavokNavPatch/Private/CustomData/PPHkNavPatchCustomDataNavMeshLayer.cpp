// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomData/PPHkNavPatchCustomDataNavMeshLayer.h"

#include "PPHkNavPatchCustomNavMeshSilhouetteGenerator.h"
#include "PPHkNavPatch_CustomData.h"


namespace PPHkNav::Navigation
{
	static constexpr FColor HalfGreen{32, 192, 64, 170};
	static constexpr FColor HalfRed{192, 32, 64, 170};
	static constexpr FColor HalfOrange{192, 192, 64, 170};
} // namespace MassPal::Navigation

UPPHkNavPatchCustomDataNavMeshLayer::UPPHkNavPatchCustomDataNavMeshLayer()
{
	GenerationParameters.ErosionRadius = 50;

	DynamicCuttingController = UPPHkNavPatchCustomNavMeshDynamicCuttingController::StaticClass();
	
	DefaultFaceDisplayColor = PPHkNav::Navigation::HalfGreen;
}

FColor UPPHkNavPatchCustomDataNavMeshLayer::GetFaceDisplayColor(FHavokNavAnyConstRef FaceData) const
{
	const FFaceDataType& CustomData = FaceData.As<FFaceDataType>();
	switch (CustomData.FaceTraversalState)
	{
	case EPPHkNavPatch_FaceTraversalState::None:
		return DefaultFaceDisplayColor;
	case EPPHkNavPatch_FaceTraversalState::Traversable:
		return PPHkNav::Navigation::HalfOrange;
	case EPPHkNavPatch_FaceTraversalState::Blocking:
		return PPHkNav::Navigation::HalfRed;
	default:
		return DefaultFaceDisplayColor;
	}
}

FColor UPPHkNavPatchCustomDataNavMeshLayer::GetFaceDisplayColorStatic()
{
	return PPHkNav::Navigation::HalfGreen;
}

FColor UPPHkNavPatchCustomDataNavMeshLayer::GetEdgeDisplayColorStatic()
{
	return FColor::Red;
}
