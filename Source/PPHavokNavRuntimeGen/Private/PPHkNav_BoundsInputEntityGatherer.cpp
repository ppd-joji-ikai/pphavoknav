// Copyright Pocketpair, Inc. All Rights Reserved.

#include "PPHkNav_BoundsInputEntityGatherer.h"
#include "HavokNavChaosGeometryProvider.h"
#include "HavokNavGeometryCollectionGeometryProvider.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "HavokNavSeedPointComponent.h"
#include "HavokNavNavMeshUserEdgeComponent.h"
#include "HavokNavNavMeshUserEdgeInformationSet.h"
#include "EngineUtils.h"
#include "PPHkNav_ChaosGeometryProvider.h"

struct UPPHkNav_BoundsInputEntityGatherer::FPrimitive
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
		const uint32 Hash = Primitive.PrimitiveComponent != nullptr ? HashCombineFast(Primitive.PrimitiveComponent->GetUniqueID(), Primitive.InstanceIndex)
		: HashCombineFast(INDEX_NONE, INDEX_NONE);
		return PointerHash(Primitive.PhysicsObjectHandle, Hash);
	}
};

FHavokNavNavMeshGenerationInputEntitySet UPPHkNav_BoundsInputEntityGatherer::GatherInputEntities(
	TSubclassOf<UHavokNavNavMeshLayer> Layer, 
	FTransform const& GenerationTransform,
	FHavokNavNavMeshGenerationBounds const& GenerationLocalBounds,
	TScriptInterface<IHavokNavNavMeshGenerationController> Controller, 
	float InputGatheringBoundsExpansion) const
{
	UWorld* CurrentWorld = World.Get();
	if (!CurrentWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("UPPHkNav_BoundsInputEntityGatherer: World is not set."));
		return {};
	}

	FHavokNavNavMeshGenerationInputEntitySet Result;
	Result.PhysicalInputObjectDatas.SetElementType(Controller->GetPhysicalObjectDataType());

	// 範囲内のプリミティブを収集
	TArray<FPrimitive> Primitives;
	CollectPrimitives(Layer, GenerationTransform, GenerationLocalBounds, InputGatheringBoundsExpansion, Primitives);

	// 収集したプリミティブを処理
	for (const auto& Primitive : Primitives)
	{
		ProcessPrimitive(Primitive, GenerationTransform, Controller, Result);
	}

	// SeedPointとUserEdgeを処理
	FHavokNavNavMeshUserEdgeStaticInformationSet::FBuilder UserEdgeDataSetBuilder;
	
	// バウンド内のアクターを処理
	FBox GenerationBoundingBoxWorldSpace = GenerationLocalBounds.BoundingBox.TransformBy(GenerationTransform);
	
	// 範囲内のすべてのアクターに対してSeedPointとUserEdgeを検索
	for (AActor const* Actor : TActorRange<AActor>(CurrentWorld))
	{
		// アクターのバウンドがジェネレーション範囲と交差するか確認
		if (Actor && Actor->GetComponentsBoundingBox().Intersect(GenerationBoundingBoxWorldSpace))
		{
			ProcessSeedPointsAndUserEdges(Actor, Layer, GenerationTransform, GenerationLocalBounds, Controller, Result, UserEdgeDataSetBuilder);
		}
	}
	
	Result.UserEdgeDescriptions.InformationSet = UserEdgeDataSetBuilder.Finalize();
	
	return Result;
}

void UPPHkNav_BoundsInputEntityGatherer::CollectPrimitives(
	TSubclassOf<UHavokNavNavMeshLayer> Layer, 
	FTransform const& GenerationTransform,
	const FHavokNavNavMeshGenerationBounds& GenerationLocalBounds, 
	float InputGatheringBoundsExpansion,
	TArray<FPrimitive>& PrimitivesInOut) const
{
	UWorld* CurrentWorld = World.Get();
	if (!CurrentWorld)
	{
		return;
	}

	// ローカル境界ボックスをワールド空間に変換
	FBox GenerationBoundingBoxWorldSpace = GenerationLocalBounds.BoundingBox.TransformBy(GenerationTransform);
	
	// オーバーラップ形状を設定
	FCollisionShape OverlapShape = FCollisionShape::MakeBox(GenerationBoundingBoxWorldSpace.GetExtent() + FVector(InputGatheringBoundsExpansion));
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PPHkNav_GatherInputOverlaps), /*bTraceComplex =*/ false);

	// ワールド内の全オブジェクトとのオーバーラップをチェック
	TArray<FOverlapResult> OverlapResults;
	CurrentWorld->OverlapMultiByObjectType(
		OverlapResults, 
		GenerationBoundingBoxWorldSpace.GetCenter(), 
		FQuat::Identity, 
		FCollisionObjectQueryParams::AllObjects, 
		OverlapShape, 
		Params
	);

	// 検出されたオーバーラップを処理
	PrimitivesInOut.Reserve(OverlapResults.Num());
	for (FOverlapResult const& OverlapResult : OverlapResults)
	{
		if (UPrimitiveComponent* PrimitiveComponent = OverlapResult.GetComponent())
		{
			const FPrimitive PrimitiveToAdd = {
				.PrimitiveComponent = PrimitiveComponent,
				.InstanceIndex = OverlapResult.ItemIndex,
				.PhysicsObjectHandle = nullptr,
			};
			PrimitivesInOut.AddUnique(PrimitiveToAdd);
		}
		// Entityベースで作成したMassPalPhysicsのPhysicsActorはPrimitiveComponentを持たないため、PhysicsObjectHandleをチェック
		else if (OverlapResult.PhysicsObject != nullptr)
		{
			const FPrimitive PrimitiveToAdd = {
				.PrimitiveComponent = nullptr,
				.InstanceIndex = INDEX_NONE,
				.PhysicsObjectHandle = OverlapResult.PhysicsObject,
			};
			PrimitivesInOut.AddUnique(PrimitiveToAdd);
		}
	}

	// ジオメトリコレクションの特別処理（エディタでの動作を保証）
	// ジオメトリコレクション は PIE/SIE モードでは物理エンティティを持つが、エディタモードでは持たないため、オーバーラップテストでは検出されない。
#if WITH_EDITOR
	if (!CurrentWorld->IsGameWorld())
	{
		// ジオメトリコレクションのチェック
		TInlineComponentArray<UGeometryCollectionComponent*> GeometryCollectionComponents;
		for (AActor const* Actor : TActorRange<AActor>(CurrentWorld))
		{
			Actor->GetComponents<UGeometryCollectionComponent>(GeometryCollectionComponents);
			for (UGeometryCollectionComponent* GeometryCollectionComponent : GeometryCollectionComponents)
			{
				if (GeometryCollectionComponent && GenerationBoundingBoxWorldSpace.Intersect(GeometryCollectionComponent->Bounds.GetBox()))
				{
					FPrimitive PrimitiveToAdd = { GeometryCollectionComponent };
					if (!PrimitivesInOut.Contains(PrimitiveToAdd))
					{
						PrimitivesInOut.Add(PrimitiveToAdd);
					}
				}
			}
			GeometryCollectionComponents.Reset();
		}
	}
#endif // WITH_EDITOR
}

void UPPHkNav_BoundsInputEntityGatherer::ProcessPrimitive(
	const FPrimitive& Primitive, 
	FTransform const& GenerationTransform,
	TScriptInterface<IHavokNavNavMeshGenerationController> Controller, 
	FHavokNavNavMeshGenerationInputEntitySet& InputEntitiesOut) const
{
	using FPoResult = IHavokNavNavMeshGenerationController::FProcessPhysicalObjectResult;
    using FUsage = EHavokNavNavMeshGenerationPhysicalObjectUsage;

	if (!World.Get())
	{
		return;
	}

	// PhysicsObjectHandle 指定の場合はEntityベースのはずなので必ず含める
	if (Primitive.PhysicsObjectHandle != nullptr)
	{
		FHavokNavAnyArray& ObjectDatasOut = InputEntitiesOut.PhysicalInputObjectDatas;
		check(ObjectDatasOut.GetElementType() == Controller->GetPhysicalObjectDataType());
		// 物理オブジェクトはデータを持たないが、DataIndexは必ず必要なのでダミーデータを追加
		const int32 DataIndex = ObjectDatasOut.Num();
		FHavokNavAnyRef _ = ObjectDatasOut.Add_GetRef(); //no custom data

		const FTransform WorldToGenerationSpaceTransform = GenerationTransform.Inverse();
		TUniquePtr<FPPHkNav_ChaosGeometryProvider> GeometryProvider = MakeUnique<FPPHkNav_ChaosGeometryProvider>(
			Primitive.PhysicsObjectHandle, 
			WorldToGenerationSpaceTransform, 
			EHavokNavGeometrySource::BoundingBox);
		InputEntitiesOut.PhysicalInputObjects.Emplace(FHavokNavNavMeshGenerationPhysicalInputObject
			{
				.GeometryProvider = MoveTemp(GeometryProvider),
				.Usage = FUsage::WalkableSurface,
				.DataIndex = DataIndex,
			});
		return;
	}

	if (!Primitive.PrimitiveComponent)
	{
		return;
	}

	FHavokNavAnyArray& ObjectDatasOut = InputEntitiesOut.PhysicalInputObjectDatas;
	check(ObjectDatasOut.GetElementType() == Controller->GetPhysicalObjectDataType());

	// 初期データインデックスを記録
	int32 DataIndex = ObjectDatasOut.Num();
	FHavokNavAnyRef ObjectData = ObjectDatasOut.Add_GetRef();

	// コントローラーを使用してプリミティブコンポーネントを処理
	UPrimitiveComponent* PrimitiveComponent = Primitive.PrimitiveComponent;
	FPoResult ProcessPhysicalObjectResult = Controller->ProcessPrimitiveComponent(PrimitiveComponent, ObjectData);
	

	// ペインティングボリュームの特別処理
	if (ProcessPhysicalObjectResult.Usage == FUsage::PaintingVolume && ObjectDatasOut.GetElementType() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Painting volumes cannot be used if there is no object data type for the controller."));
		ProcessPhysicalObjectResult.Usage = FUsage::None;
	}

	// ウォーカブルサーフェスかペインティングボリューム以外は無視
	if (ProcessPhysicalObjectResult.Usage != FUsage::WalkableSurface && ProcessPhysicalObjectResult.Usage != FUsage::PaintingVolume)
	{
		ObjectDatasOut.Pop();
		DataIndex = -1;
	}

	// 使用しないプリミティブは無視
	if (ProcessPhysicalObjectResult.Usage == FUsage::None)
	{
		return;
	}

	// ワールド→生成空間への変換
	FTransform WorldToGenerationSpaceTransform = GenerationTransform.Inverse();

	// 物理入力オブジェクトを作成
	FHavokNavNavMeshGenerationPhysicalInputObject& PhysicalInputObject = InputEntitiesOut.PhysicalInputObjects.AddDefaulted_GetRef();

	// コンポーネントの種類に応じたジオメトリプロバイダーを設定
	if (const UGeometryCollectionComponent* GeometryCollectionComponent = Cast<const UGeometryCollectionComponent>(PrimitiveComponent))
	{
		PhysicalInputObject.GeometryProvider = MakeUnique<FHavokNavGeometryCollectionGeometryProvider>(
			GeometryCollectionComponent, 
			WorldToGenerationSpaceTransform,
			ProcessPhysicalObjectResult.GeometrySource
		);
	}
	else
	{
#if WITH_HAVOK_PHYSICS
		PhysicalInputObject.GeometryProvider = MakeUnique<FHavokNavPhysicsGeometryProvider>(
			CurrentWorld, 
			WorldToGenerationSpaceTransform, 
			PrimitiveComponent->GetBodyInstance(NAME_None, true, Primitive.InstanceIndex), 
			ProcessPhysicalObjectResult.GeometrySource
		);
#else
		PhysicalInputObject.GeometryProvider = MakeUnique<FHavokNavChaosGeometryProvider>(
			PrimitiveComponent->GetBodyInstance(NAME_None, true, Primitive.InstanceIndex),
			WorldToGenerationSpaceTransform, 
			ProcessPhysicalObjectResult.GeometrySource
		);
#endif
	}

	// 物理入力オブジェクトの属性を設定
	PhysicalInputObject.Usage = ProcessPhysicalObjectResult.Usage;
	PhysicalInputObject.DataIndex = DataIndex;
}

void UPPHkNav_BoundsInputEntityGatherer::ProcessSeedPointsAndUserEdges(
	AActor const* Actor,
	TSubclassOf<UHavokNavNavMeshLayer> Layer,
	FTransform const& GenerationTransform,
	FHavokNavNavMeshGenerationBounds const& GenerationLocalBounds,
	TScriptInterface<IHavokNavNavMeshGenerationController> Controller,
	FHavokNavNavMeshGenerationInputEntitySet& InputEntitiesOut,
	FHavokNavNavMeshUserEdgeStaticInformationSet::FBuilder& UserEdgeDataSetBuilder) const
{
	if (!Actor)
	{
		return;
	}

	UWorld* CurrentWorld = World.Get();
	if (!CurrentWorld)
	{
		return;
	}

	// ワールド空間でのバウンディングボックスを取得
	const FBox GenerationBoundingBoxWorldSpace = GenerationLocalBounds.BoundingBox.TransformBy(GenerationTransform);

	// アクターの各コンポーネントを確認
	for (UActorComponent const* Component : Actor->GetComponents())
	{
		// SeedPointコンポーネントの処理
		if (UHavokNavSeedPointComponent const* SeedPointComponent = Cast<const UHavokNavSeedPointComponent>(Component))
		{
			// レイヤーの確認
			if (!SeedPointComponent->RelevantNavMeshLayers.Contains(Layer))
			{
				continue;
			}

			// 位置の確認
			FVector SeedPoint = SeedPointComponent->GetComponentLocation();
			if (!GenerationBoundingBoxWorldSpace.IsInside(SeedPoint))
			{
				continue;
			}

			// ローカル空間に変換して追加
			SeedPoint = GenerationTransform.InverseTransformPositionNoScale(SeedPoint);
			InputEntitiesOut.SeedPoints.Add(FVector3f(SeedPoint));
		}
		// UserEdgeコンポーネントの処理
		else if (UHavokNavNavMeshUserEdgeComponent const* UserEdgeComponent = Cast<const UHavokNavNavMeshUserEdgeComponent>(Component))
		{
			// 動的エッジは無視
			if (UserEdgeComponent->bDynamic)
			{
				continue;
			}

			// レイヤーの確認
			if (UserEdgeComponent->LayerClass != nullptr && UserEdgeComponent->LayerClass != Layer)
			{
				continue;
			}

			// 有効性確認
			bool bIsValid = UserEdgeComponent->Validate();
			if (!bIsValid)
			{
				continue;
			}

			// エッジがバウンド内にあるか確認
			FBox UserEdgeABoundingBox, UserEdgeBBoundingBox;
			UserEdgeComponent->GetWorldSpaceEdgeBoundingBoxes(UserEdgeABoundingBox, UserEdgeBBoundingBox);

			if (!GenerationBoundingBoxWorldSpace.Intersect(UserEdgeABoundingBox) ||
				!GenerationBoundingBoxWorldSpace.Intersect(UserEdgeBBoundingBox))
			{
				continue;
			}

			// エッジペア情報を追加
			FHavokNavNavMeshUserEdgePairDescription_InSet& EdgePairDescription = InputEntitiesOut.UserEdgeDescriptions.EdgePairDescriptions.AddDefaulted_GetRef();
			static_cast<FHavokNavNavMeshUserEdgePairDescriptionBase&>(EdgePairDescription) = UserEdgeComponent->MakeDescriptionBase();
			EdgePairDescription.Points = EdgePairDescription.Points.TransformWorldToLocal(GenerationTransform);

			// エッジデータの追加
			FHavokNavAnyRef DataRef = UserEdgeDataSetBuilder.Add_GetRef(
				UserEdgeComponent->UserEdgeClass,
				EdgePairDescription.SubsetIndex,
				EdgePairDescription.DataIndex
			);

			UserEdgeComponent->GetData(DataRef);
		}
	}
}
