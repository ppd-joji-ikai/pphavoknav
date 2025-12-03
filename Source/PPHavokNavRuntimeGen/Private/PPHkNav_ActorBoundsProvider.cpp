// Fill out your copyright notice in the Description page of Project Settings.


#include "PPHkNav_ActorBoundsProvider.h"

#include "HavokNavGenerationUtilities.h"
#include "HavokNavNavMeshLayer.h"

namespace MassPal::Navigation
{
	// AffectNavigationなComponentのBoudingBoxを得る
	static FBox CalcAffectNavBounds(const AActor* Actor, const bool bIncludeFromChildActors)
	{
		FBox Box(ForceInit);
		Actor->ForEachComponent<UPrimitiveComponent>(bIncludeFromChildActors, [&Box](const UPrimitiveComponent* InPrimComp)
		{
			if (InPrimComp->IsRegistered() && InPrimComp->CanEverAffectNavigation())
			{
				Box += InPrimComp->Bounds.GetBox();
			}
		});
		return Box;
	}
}
FHavokNavNavMeshGenerationBounds UPPHkNav_ActorBoundsProvider::GetBounds(TSubclassOf<UHavokNavNavMeshLayer> Layer, FTransform const& GenerationTransform) const
{
	if (!TargetLayer)
	{
		return {};
	}
	
	if (Layer != TargetLayer)
	{
		return {};
	}
	
	const AActor* Actor = GetValid(TargetActor);
	if (!Actor)
	{
		return {};
	}

	check(GenerationTransform.GetScale3D().Equals(FVector::OneVector));
	const FTransform WorldToGenerationSpaceTransform = GenerationTransform.Inverse();

	const FTransform& ActorTransform = Actor->GetTransform();
	const FTransform ActorToLocalTransform = ActorTransform.Inverse();
	constexpr bool bIncludeFromChildActors = false;
	const FBox BoundingBoxWorld = MassPal::Navigation::CalcAffectNavBounds(Actor, bIncludeFromChildActors);
	const FBox LocalBounds = BoundingBoxWorld.TransformBy(ActorToLocalTransform);
	const FBox GenerationSpaceBounds = BoundingBoxWorld.TransformBy(WorldToGenerationSpaceTransform);

	// Positive bounds を収集
	FHavokNavLineLoop LineLoop;
	const FTransform LocalToGenerationSpaceTransform = ActorTransform * WorldToGenerationSpaceTransform;
	HavokNavGenerationUtilities::GetLineLoopPoints(LocalBounds, LocalToGenerationSpaceTransform, UpVector, LineLoop.Points);
	
	// TargetActorとの重なりは考慮しない
	// BlockingVolume等も考慮しない
	// Negative boundsは空とする
	return FHavokNavNavMeshGenerationBounds
	{
		.bEverywhereIsPositive = false,
		.BoundingBox = GenerationSpaceBounds,
		.PositiveBound = LineLoop,
		.NegativeBounds = {},
	};
}
