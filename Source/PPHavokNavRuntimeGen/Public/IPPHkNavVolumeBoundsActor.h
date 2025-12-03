// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IPPHkNavVolumeBoundsActor.generated.h"

class UHavokNavNavVolumeLayer;

UINTERFACE()
class UPPHkNavVolumeBoundsActor : public UInterface
{
	GENERATED_BODY()
};

/**
 * IHavokNavNavVolumeBoundsActor をランタイム実行できるように拡張したもの
 * AVolume や AActor から派生したアクターのナビゲーションボリューム境界を定義するためのインターフェースです
 * NavVolumeの生成範囲を決定するアクターが実装してください
 */
class PPHKNAVRUNTIMEGEN_API IPPHkNavVolumeBoundsActor
{
	GENERATED_BODY()

public:
	/**
	 * @brief ワールドスペース上のNavVolume生成範囲
	 * @return 
	 */
	virtual FBox GetNavVolumeBounds() const = 0;

	/// The primary determinant of domination. If two volumes have different priorities, the one with the higher-numbered priority
	/// will dominate the one with the lower-numbered priority.
	virtual int GetPriority() const = 0;

	/// The secondary determinant of domination. If two volumes have equal priorities, the one with the lexically higher Domination GUID will
	/// dominate the one with the lexically lower Domination GUID.
	/// No two logical nav volume bounds in the same world should have the same Domination GUID, however a single logical nav volume with multiple
	/// independent sections (as is used for streaming) can have the same Domination GUID.
	virtual FGuid GetDominationGuid() const = 0;

	/// Returns whether these bounds can be used to generate Nav Volume for the given Layer
	virtual bool IsInNavVolumeLayer(TSubclassOf<UHavokNavNavVolumeLayer> Layer) const = 0;

	/// Determines how two bounds actors should interact when overlapping. The bounds that "dominate" will be used as a cutter in the other nav volume.
	virtual bool Dominates(IPPHkNavVolumeBoundsActor const* BoundsB) const
	{
		check(GetDominationGuid().IsValid() && BoundsB->GetDominationGuid().IsValid());
		if (GetPriority() != BoundsB->GetPriority())
		{
			return GetPriority() > BoundsB->GetPriority();
		}

		// When multiple nav volumes are part of a single logical non-overlapping nav volume, as is the case with grid generation, they all use the same domination Id.
		// In those cases there is no domination relationship
		if (GetDominationGuid() != BoundsB->GetDominationGuid())
		{
			return GetDominationGuid() < BoundsB->GetDominationGuid();
		}

		return false;
	}
};