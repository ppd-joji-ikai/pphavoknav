// Copyright Pocketpair, Inc. All Rights Reserved.

#include "PPHkNavRuntimeGen_NavVolumeGenerationController.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "HavokNavModifierVolume.h"
#include "HavokNavBlockingVolume.h"
#include "Engine/World.h"

UPPHkNavRuntimeGen_NavVolumeGenerationController::UPPHkNavRuntimeGen_NavVolumeGenerationController()
	: Super()
{
	// デフォルト設定
	bProcessDynamicObjects = true;
	bProcessMovableObjects = true;
	MinimumSizeThresholdCm = 50.0f;
}

IHavokNavNavVolumeGenerationController::FProcessPhysicalObjectResult UPPHkNavRuntimeGen_NavVolumeGenerationController::ProcessPrimitiveComponent(const UPrimitiveComponent* Component, FHavokNavAnyRef DataOut, int& ResolutionMultiplierOut) const
{
	constexpr FProcessPhysicalObjectResult NonResult{ EHavokNavNavVolumeGenerationPhysicalObjectUsage::None, EHavokNavGeometrySource::SimpleCollision };
	
	// 無効なコンポーネントは処理しない
	if (!IsValid(Component))
	{
		return NonResult;
	}

	// コリジョンが無効な場合は処理しない
	if (!Component->IsCollisionEnabled())
	{
		return NonResult;
	}

	// サイズが閾値未満の場合は処理しない
	if (!IsValidSizeForNavigation(Component))
	{
		return NonResult;
	}

	// ブロッキングサーフェスとして処理すべきかを判定
	if (ShouldProcessAsBlockingSurface(Component))
	{
		FProcessPhysicalObjectResult Result;
		Result.Usage = EHavokNavNavVolumeGenerationPhysicalObjectUsage::BlockingSurface;
		Result.GeometrySource = EHavokNavGeometrySource::BoundingBox;
		
		// StaticMeshComponentの場合はシンプルコリジョンを使用
		// HavokNavigationではSimpleCollision、SingleConvex、BoundingBoxのみサポート
		if (const auto* StaticMeshComp = Cast<UStaticMeshComponent>(Component))
		{
			// StaticMeshが存在する場合はSimpleCollisionを使用、そうでなければBoundingBoxを使用
			Result.GeometrySource = StaticMeshComp->GetStaticMesh() ? 
				EHavokNavGeometrySource::SimpleCollision : 
				EHavokNavGeometrySource::BoundingBox;
		}
		return Result;
	}
	else
	{
		return NonResult;
	}
}

IHavokNavNavVolumeGenerationController::FProcessPhysicalObjectResult UPPHkNavRuntimeGen_NavVolumeGenerationController::ProcessModifierVolume(const AHavokNavModifierVolume* Volume, FHavokNavAnyRef DataOut, int& ResolutionMultiplierOut) const
{
	// 無効なボリュームは処理しない
	if (!IsValid(Volume))
	{
		return { EHavokNavNavVolumeGenerationPhysicalObjectUsage::None, EHavokNavGeometrySource::BoundingBox };
	}

	// ブロッキングボリューム
	if (Volume->IsA<AHavokNavBlockingVolume>())
	{
		return { EHavokNavNavVolumeGenerationPhysicalObjectUsage::BlockingVolume, EHavokNavGeometrySource::BoundingBox };
	}

	// PaintingVolumeは未対応

	return { EHavokNavNavVolumeGenerationPhysicalObjectUsage::None, EHavokNavGeometrySource::BoundingBox };
}

bool UPPHkNavRuntimeGen_NavVolumeGenerationController::IsDynamicObject(const UPrimitiveComponent* Component) const
{
	// 無効なコンポーネント
	if (!IsValid(Component))
	{
		return false;
	}

	// Mobilityが動的な場合
	if (bProcessMovableObjects && Component->Mobility == EComponentMobility::Movable)
	{
		return true;
	}

	// 物理シミュレーションが有効な場合
	if (Component->IsSimulatingPhysics())
	{
		return true;
	}

	// アクターが動的にスポーンされた場合の判定
	if (const AActor* Owner = Component->GetOwner())
	{
		// アクターがレベル配置ではなく、実行時に作成された場合
		// レベルに配置されたアクターではない動的生成アクターかどうかを判定
		if (!Owner->IsActorBeingDestroyed() && 
			Owner->GetLevel() && 
			!Owner->GetLevel()->bIsAssociatingLevel &&
			!Owner->IsNetStartupActor())
		{
			return true;
		}
	}

	return false;
}

bool UPPHkNavRuntimeGen_NavVolumeGenerationController::IsValidSizeForNavigation(const UPrimitiveComponent* Component) const
{
	// 無効なコンポーネント
	if (!IsValid(Component))
	{
		return false;
	}

	const FVector BoundsSize = Component->Bounds.BoxExtent * 2.0f; // BoxExtentは半径なので2倍
	const float MaxDimensionCm = FMath::Max3(BoundsSize.X, BoundsSize.Y, BoundsSize.Z);

	return MaxDimensionCm >= MinimumSizeThresholdCm;
}

bool UPPHkNavRuntimeGen_NavVolumeGenerationController::ShouldProcessAsBlockingSurface(const UPrimitiveComponent* Component) const
{
	// ガード節: 無効なコンポーネント
	if (!IsValid(Component))
	{
		return false;
	}

	// Navに関連しないなら処理しない
	if (!Component->CanEverAffectNavigation())
	{
		return false;
	}

	const bool bIsStaticGeometry = Component->IsWorldGeometry();
	const bool bBlocksPawns = Component->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block;
	const bool bIsDynamic = IsDynamicObject(Component);

	// 従来のワールドジオメトリ処理
	if (bIsStaticGeometry && bBlocksPawns)
	{
		return true;
	}

	// 動的オブジェクトの処理
	if (bProcessDynamicObjects && bIsDynamic && bBlocksPawns)
	{
		return true;
	}

	return false;
}
