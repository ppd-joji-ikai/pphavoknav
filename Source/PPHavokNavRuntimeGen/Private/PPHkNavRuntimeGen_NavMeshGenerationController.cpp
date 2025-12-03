// Copyright Pocketpair, Inc. All Rights Reserved.


#include "PPHkNavRuntimeGen_NavMeshGenerationController.h"

#include "HavokNavBlockingVolume.h"
#include "CustomData/PPHkNavPatch_CustomData.h"

/**
 * UPPHkNavRuntimeGen_NavMeshGenerationControllerBase
 */
UScriptStruct* UPPHkNavRuntimeGen_NavMeshGenerationControllerBase::GetPhysicalObjectDataType() const
{
	return nullptr;
}

UScriptStruct* UPPHkNavRuntimeGen_NavMeshGenerationControllerBase::GetFaceDataType() const
{
	return FPPHkNavPatch_GroupIdFaceData::StaticStruct();
}

IHavokNavNavMeshGenerationController::FProcessPhysicalObjectResult UPPHkNavRuntimeGen_NavMeshGenerationControllerBase::
ProcessModifierVolume(const AHavokNavModifierVolume* Volume, FHavokNavAnyRef DataOut) const
{
	if (Volume->IsA<AHavokNavBlockingVolume>())
	{
		return { EHavokNavNavMeshGenerationPhysicalObjectUsage::BlockingVolume, EHavokNavGeometrySource::SimpleCollision };
	}

	return { EHavokNavNavMeshGenerationPhysicalObjectUsage::None, EHavokNavGeometrySource::SimpleCollision };
}

EHavokNavNavMeshGenerationFaceUsage UPPHkNavRuntimeGen_NavMeshGenerationControllerBase::ProcessFace(
	FHavokNavAnyConstRef SurfaceData, TArrayView<FHavokNavAnyConstRef> VolumeDatas, FHavokNavAnyRef FaceDataOut) const
{
	return EHavokNavNavMeshGenerationFaceUsage::Normal;
}

/**
 * UPPHkNavRuntimeGen_NavMeshGenerationController
 */
IHavokNavNavMeshGenerationController::FProcessPhysicalObjectResult UPPHkNavRuntimeGen_NavMeshGenerationController::
ProcessPrimitiveComponent(const UPrimitiveComponent* Component, FHavokNavAnyRef DataOut) const
{
	// 非Sim & 非PawnのPrimitiveComponentは歩行可能なサーフェスとして扱う
	// 基本的にはワールドで静止しているコリジョンを対象とする
	// 移動プラットフォームや動的スポン床を対象にするため、MobilityはStaticに限定しない
	//
	// 0. CanEverAffectNavigation() が false の場合は除外する
	// 1. Pawnやキャラクターの上を歩いてほしくないので除外する
	// 2. 物理挙動で動くようなPrimitiveは除外する
	// 3. Pawnが乗れるPrimitiveを採用する
	const bool bCanEverAffectNavigation = Component->CanEverAffectNavigation();
	const bool bIsNotPawn = !Component->GetOwner()->IsA(APawn::StaticClass());
	const bool bIsNotSimulated = !Component->IsSimulatingPhysics();
	const bool bBlocksPawns = Component->IsCollisionEnabled() && Component->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block;
	const EHavokNavNavMeshGenerationPhysicalObjectUsage Usage = (bCanEverAffectNavigation && bBlocksPawns && bIsNotSimulated && bIsNotPawn)
	? EHavokNavNavMeshGenerationPhysicalObjectUsage::WalkableSurface
	: EHavokNavNavMeshGenerationPhysicalObjectUsage::None;
	
	return FProcessPhysicalObjectResult{
		.Usage = Usage,
		.GeometrySource = EHavokNavGeometrySource::SimpleCollision
	};
}

/**
 * UPPHkNavRuntimeGen_NavMeshGenerationControllerCanEverAffectNav
 */
IHavokNavNavMeshGenerationController::FProcessPhysicalObjectResult
UPPHkNavRuntimeGen_NavMeshGenerationControllerCanEverAffectNav::ProcessPrimitiveComponent(
	const UPrimitiveComponent* Component, FHavokNavAnyRef DataOut) const
{
	const bool bCanEverAffectNavigation = Component->CanEverAffectNavigation();
	if (bCanEverAffectNavigation)
	{
		return FProcessPhysicalObjectResult{
			.Usage = EHavokNavNavMeshGenerationPhysicalObjectUsage::WalkableSurface,
			.GeometrySource = EHavokNavGeometrySource::SimpleCollision
		};
	}
	return FProcessPhysicalObjectResult{
		.Usage = EHavokNavNavMeshGenerationPhysicalObjectUsage::None,
		.GeometrySource = EHavokNavGeometrySource::SimpleCollision
	};
}
