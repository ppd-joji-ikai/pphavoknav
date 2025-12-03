// Copyright Pocketpair, Inc. All Rights Reserved.

#include "PPHkNav_BoundsVolumeInputEntityGatherer.h"
#include "HavokNavChaosGeometryProvider.h"
#include "HavokNavGeometryCollectionGeometryProvider.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "HavokNavSeedPointComponent.h"
#include "HavokNavNavVolumeUserEdgeComponent.h"
#include "HavokNavNavVolumeUserEdgeInformationSet.h"
#include "EngineUtils.h"
#include "PPHkNav_ChaosGeometryProvider.h"

struct UPPHkNav_BoundsVolumeInputEntityGatherer::FPrimitive
{
	TObjectPtr<UPrimitiveComponent> PrimitiveComponent = nullptr;
	int32 InstanceIndex = INDEX_NONE;
	Chaos::FPhysicsObjectHandle PhysicsObjectHandle = nullptr;

	bool operator==(const FPrimitive& Other) const
	{
		return (PrimitiveComponent == Other.PrimitiveComponent) && (InstanceIndex == Other.InstanceIndex)
			&& (PhysicsObjectHandle == Other.PhysicsObjectHandle);
	}

	friend uint32 GetTypeHash(const FPrimitive& Primitive)
	{
		const uint32 Hash = Primitive.PrimitiveComponent != nullptr
			                    ? HashCombineFast(Primitive.PrimitiveComponent->GetUniqueID(), Primitive.InstanceIndex)
			                    : HashCombineFast(INDEX_NONE, INDEX_NONE);
		return PointerHash(Primitive.PhysicsObjectHandle, Hash);
	}
};

FHavokNavNavVolumeGenerationInputEntitySet UPPHkNav_BoundsVolumeInputEntityGatherer::GatherInputEntities(
	TSubclassOf<UHavokNavNavVolumeLayer> Layer,
	FHavokNavAxialTransform const& GenerationTransform,
	FHavokNavNavVolumeGenerationBounds const& GenerationLocalBounds,
	TScriptInterface<IHavokNavNavVolumeGenerationController> Controller,
	FBox const& InputGatheringBoundsExpansion) const
{
	UWorld* CurrentWorld = World.Get();
	if (!CurrentWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("UPPHkNav_BoundsVolumeInputEntityGatherer: World is not set."));
		return {};
	}

	FHavokNavNavVolumeGenerationInputEntitySet Result;
	Result.PhysicalInputObjectDatas.SetElementType(Controller->GetCellDataType());

	// Put the default object/cell data at index 0
	Controller->GetDefaultCellData(Result.PhysicalInputObjectDatas.Add_GetRef());

	// Collect primitives in the specified bounds
	TArray<FPrimitive> Primitives;
	CollectPrimitives(Layer, GenerationTransform, GenerationLocalBounds, InputGatheringBoundsExpansion, Primitives);

	// Process each primitive
	for (const FPrimitive& Primitive : Primitives)
	{
		ProcessPrimitive(Primitive, GenerationTransform, Controller, Result);
	}

	// Process seed points and user edges
	FHavokNavNavVolumeUserEdgeStaticInformationSet::FBuilder UserEdgeDataSetBuilder;
	for (TActorIterator<AActor> ActorIterator(CurrentWorld); ActorIterator; ++ActorIterator)
	{
		AActor* Actor = *ActorIterator;
		if (Actor && IsValid(Actor))
		{
			ProcessSeedPointsAndUserEdges(Actor, Layer, GenerationTransform, GenerationLocalBounds, Controller, Result,
			                              UserEdgeDataSetBuilder);
		}
	}

	// Build user edge data set
	Result.UserEdgeDescriptions.InformationSet = UserEdgeDataSetBuilder.Finalize();

	return Result;
}

void UPPHkNav_BoundsVolumeInputEntityGatherer::CollectPrimitives(
	TSubclassOf<UHavokNavNavVolumeLayer> Layer,
	FHavokNavAxialTransform const& GenerationAxialTransform,
	const FHavokNavNavVolumeGenerationBounds& GenerationLocalBounds,
	FBox const& InputGatheringBoundsExpansion,
	TArray<FPrimitive>& PrimitivesInOut) const
{
	UWorld* CurrentWorld = World.Get();
	if (!CurrentWorld)
	{
		return;
	}

	FTransform GenerationTransform;
	GenerationAxialTransform.ToTransform(GenerationTransform);

	// Expand the bounds for input gathering
	FBox ExpandedLocalBounds(GenerationLocalBounds.PositiveBound.Min + InputGatheringBoundsExpansion.Min,
	                         GenerationLocalBounds.PositiveBound.Max + InputGatheringBoundsExpansion.Max);

	// Transform to world space for overlap queries
	FBox WorldBounds = ExpandedLocalBounds.TransformBy(GenerationTransform);

	// Perform overlap query
	const FCollisionShape OverlapShape = FCollisionShape::MakeBox(WorldBounds.GetExtent());
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HavokNav_GatherInputOverlaps), /*bTraceComplex =*/ false);

	TArray<FOverlapResult> OverlapResults;
	if (World->OverlapMultiByObjectType(OverlapResults, WorldBounds.GetCenter(), FQuat::Identity,
	                                    FCollisionObjectQueryParams::AllObjects, OverlapShape, QueryParams))
	{
		TSet<FPrimitive> UniquePrimitives;
		UniquePrimitives.Reserve(OverlapResults.Num());

		for (const FOverlapResult& OverlapResult : OverlapResults)
		{
			UPrimitiveComponent* PrimitiveComponent = OverlapResult.GetComponent();
			if (IsValid(PrimitiveComponent))
			{
				FPrimitive Primitive{
					.PrimitiveComponent = PrimitiveComponent,
					.InstanceIndex = OverlapResult.ItemIndex,
					.PhysicsObjectHandle = nullptr
				};
				UniquePrimitives.Add(Primitive);
			}
			// Entityベースで生成されたPhysicObject は PrimitiveComponent を持たないので、PhysicsObjectHandle をチェック
			else if (OverlapResult.PhysicsObject)
			{
				FPrimitive Primitive{
					.PrimitiveComponent = nullptr,
					.InstanceIndex = INDEX_NONE,
					.PhysicsObjectHandle = OverlapResult.PhysicsObject,
				};
				UniquePrimitives.Add(Primitive);
			}
		}

		PrimitivesInOut.Append(UniquePrimitives.Array());
	}
}

void UPPHkNav_BoundsVolumeInputEntityGatherer::ProcessPrimitive(
	const FPrimitive& Primitive,
	FHavokNavAxialTransform const& GenerationAxialTransform,
	TScriptInterface<IHavokNavNavVolumeGenerationController> Controller,
	FHavokNavNavVolumeGenerationInputEntitySet& InputEntitiesOut) const
{
	FHavokNavAnyArray& ObjectDatasOut = InputEntitiesOut.PhysicalInputObjectDatas;
	check(ObjectDatasOut.GetElementType() == Controller->GetCellDataType());

	// PhysicsObjectHandle 指定の場合はEntityベースのはずなので必ず含める
	if (Primitive.PhysicsObjectHandle)
	{
		// Add a dummy data for the physical object data
		// const int32 DataIndex = ObjectDatasOut.Num(); // カスタムデータは必ず必要なのでダミーデータを持たせる
		// FHavokNavAnyRef _ = ObjectDatasOut.Add_GetRef();

		FTransform GenerationTransform;
		GenerationAxialTransform.ToTransform(GenerationTransform);
		const FTransform WorldToGenerationSpaceTransform = GenerationTransform.Inverse();

		TUniquePtr<FPPHkNav_ChaosGeometryProvider> GeometryProvider = MakeUnique<FPPHkNav_ChaosGeometryProvider>(
			Primitive.PhysicsObjectHandle,
			WorldToGenerationSpaceTransform,
			EHavokNavGeometrySource::BoundingBox);
		InputEntitiesOut.PhysicalInputObjects.Emplace(FHavokNavNavVolumeGenerationPhysicalInputObject
			{
				.GeometryProvider = MoveTemp(GeometryProvider),
				.Usage = EHavokNavNavVolumeGenerationPhysicalObjectUsage::BlockingVolume,
				.DataIndex = -1,
				.ResolutionMultiplier = -1,
			});
		return;
	}

	if (TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Primitive.PrimitiveComponent;
		IsValid(PrimitiveComponent))
	{
		int32 DataIndex = ObjectDatasOut.Num(); // カスタムデータは必ず必要なのでダミーデータを持たせる
		FHavokNavAnyRef CustomDataOut = ObjectDatasOut.Add_GetRef();

		int ResolutionMultiplier = -1;
		IHavokNavNavVolumeGenerationController::FProcessPhysicalObjectResult ProcessPhysicalObjectResult =
			Controller->ProcessPrimitiveComponent(PrimitiveComponent, CustomDataOut, ResolutionMultiplier);

		if (ProcessPhysicalObjectResult.Usage == EHavokNavNavVolumeGenerationPhysicalObjectUsage::None)
		{
			ObjectDatasOut.Pop();
			DataIndex = -1;
			return; // skip if no usage
		}
		else if (ProcessPhysicalObjectResult.Usage == EHavokNavNavVolumeGenerationPhysicalObjectUsage::PaintingVolume)
		{
			if (ResolutionMultiplier != -1)
			{
				UE_LOG(LogTemp, Error, TEXT("The resolution multiplier can only be modified by painting volumes."));
				ResolutionMultiplier = -1;
			}
		}
		else
		{
			// PhysicalObjectDataはPaintingVolume以外では使用しないので削除
			ObjectDatasOut.Pop();
			DataIndex = -1;
		}

		FTransform GenerationTransform;
		GenerationAxialTransform.ToTransform(GenerationTransform);
		const FTransform WorldToGenerationSpaceTransform = GenerationTransform.Inverse();
		const FBodyInstance* BodyInstance = PrimitiveComponent->GetBodyInstance(NAME_None, true, Primitive.InstanceIndex);
		TUniquePtr<FHavokNavChaosGeometryProvider> GeometryProvider = MakeUnique<FHavokNavChaosGeometryProvider>(
			BodyInstance, WorldToGenerationSpaceTransform, ProcessPhysicalObjectResult.GeometrySource);
		InputEntitiesOut.PhysicalInputObjects.Emplace(FHavokNavNavVolumeGenerationPhysicalInputObject
			{
				.GeometryProvider = MoveTemp(GeometryProvider),
				.Usage = ProcessPhysicalObjectResult.Usage,
				.DataIndex = DataIndex,
				.ResolutionMultiplier = ResolutionMultiplier,
			});
			
		return;
	}
}

void UPPHkNav_BoundsVolumeInputEntityGatherer::ProcessSeedPointsAndUserEdges(
	AActor const* Actor,
	TSubclassOf<UHavokNavNavVolumeLayer> Layer,
	FHavokNavAxialTransform const& GenerationAxialTransform,
	FHavokNavNavVolumeGenerationBounds const& GenerationLocalBounds,
	TScriptInterface<IHavokNavNavVolumeGenerationController> Controller,
	FHavokNavNavVolumeGenerationInputEntitySet& InputEntitiesOut,
	FHavokNavNavVolumeUserEdgeStaticInformationSet::FBuilder& UserEdgeDataSetBuilder) const
{
	if (!IsValid(Actor))
	{
		return;
	}

	FTransform GenerationTransform;
	GenerationAxialTransform.ToTransform(GenerationTransform);
	const FBox WorldPositiveBound = GenerationLocalBounds.PositiveBound.TransformBy(GenerationTransform);

	// SeedPointを収集
	TArray<UHavokNavSeedPointComponent*> SeedPointComponents;
	Actor->GetComponents<UHavokNavSeedPointComponent>(SeedPointComponents);
	for (UHavokNavSeedPointComponent* SeedPointComponent : SeedPointComponents)
	{
		if (!SeedPointComponent || !IsValid(SeedPointComponent))
		{
			continue;
		}

		// Check if this seed point is for the target layer
		if (!SeedPointComponent->RelevantNavVolumeLayers.Contains(Layer))
		{
			continue;
		}

		const FVector WorldSeedPoint = SeedPointComponent->GetComponentLocation();
		if (!WorldPositiveBound.IsInside(WorldSeedPoint))
		{
			continue; // Skip seed points outside the generation bounds
		}

		// SeedPointが生成範囲内にあるならローカル座標で追加
		const FVector LocalSeedPoint = GenerationTransform.InverseTransformPositionNoScale(WorldSeedPoint);
		InputEntitiesOut.SeedPoints.Add(FVector3f(LocalSeedPoint));
	}

	// UserEdgeを収集
	TArray<UHavokNavNavVolumeUserEdgeComponent*> UserEdgeComponents;
	Actor->GetComponents<UHavokNavNavVolumeUserEdgeComponent>(UserEdgeComponents);
	for (UHavokNavNavVolumeUserEdgeComponent* UserEdgeComponent : UserEdgeComponents)
	{
		if (!IsValid(UserEdgeComponent))
		{
			continue;
		}

		// UserEdgeのレイヤー指定があるならそのレイヤーのみに含める
		// レイヤー指定がないならば全てのレイヤーに含める
		if (UserEdgeComponent->LayerClass != nullptr && UserEdgeComponent->LayerClass != Layer)
		{
			continue;
		}

		// ユーザーエッジの両端が生成範囲に入っていないとダメ
		const FHavokNavNavVolumeUserEdgePoints WorldUserEdgePoints = UserEdgeComponent->GetWorldPoints();
		if (!WorldPositiveBound.IsInside(WorldUserEdgePoints.Start) || !WorldPositiveBound.IsInside(
			WorldUserEdgePoints.End))
		{
			continue; // Skip user edges outside the generation bounds
		}

		FHavokNavNavVolumeUserEdgePairDescriptionBase UserEdgeDescriptionBase = UserEdgeComponent->
			MakeDescriptionBase();
		UserEdgeDescriptionBase.Points = UserEdgeDescriptionBase.Points.TransformWorldToLocal(GenerationTransform);

		FHavokNavNavVolumeStaticUserEdgePairDescriptionSet::FEdgePairDescription& EdgePairDescriptionRef =
			InputEntitiesOut.UserEdgeDescriptions.EdgePairDescriptions.Add_GetRef(
				FHavokNavNavVolumeStaticUserEdgePairDescriptionSet::FEdgePairDescription(UserEdgeDescriptionBase));
		UserEdgeComponent->GetData(
			UserEdgeDataSetBuilder.Add_GetRef(UserEdgeComponent->UserEdgeClass, EdgePairDescriptionRef.Locator));

		if (UserEdgeComponent->bBidirectional)
		{
			UserEdgeDescriptionBase.Points.Reverse();

			FHavokNavNavVolumeStaticUserEdgePairDescriptionSet::FEdgePairDescription& OppositeEdgePairDescriptionRef =
				InputEntitiesOut.UserEdgeDescriptions.EdgePairDescriptions.Add_GetRef(
					FHavokNavNavVolumeStaticUserEdgePairDescriptionSet::FEdgePairDescription(UserEdgeDescriptionBase));
			UserEdgeComponent->GetData(
				UserEdgeDataSetBuilder.Add_GetRef(UserEdgeComponent->UserEdgeClass,
				                                  OppositeEdgePairDescriptionRef.Locator));
		}
	}
}
