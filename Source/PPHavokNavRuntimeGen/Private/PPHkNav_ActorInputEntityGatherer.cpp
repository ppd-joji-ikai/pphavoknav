// Fill out your copyright notice in the Description page of Project Settings.


#include "PPHkNav_ActorInputEntityGatherer.h"

#include "HavokNavChaosGeometryProvider.h"
#include "HavokNavGeometryCollectionGeometryProvider.h"
#include "PPHkNavRampNavMeshAuthoringComponent_EditorOnly.h"
#include "PPHkNavTriangleNavMeshAuthoringComponent_EditorOnly.h"
#include "PPHkNavPyramidNavMeshAuthoringComponent_EditorOnly.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "PpCustom/HavokNavRampGeometryProvider.h"
#include "PpCustom/HavokNavTriangleGeometryProvider.h"
#include "PpCustom/HavokNavPyramidGeometryProvider.h"

namespace MassPal::Navigation::Private
{
	struct FPrimitive
	{
		TObjectPtr<UPrimitiveComponent> PrimitiveComponent = nullptr;
		int32 InstanceIndex = INDEX_NONE;

		bool operator==(const FPrimitive& Other) const
		{
			return (PrimitiveComponent == Other.PrimitiveComponent) && (InstanceIndex == Other.InstanceIndex);
		}

		friend uint32 GetTypeHash(const FPrimitive& Primitive)
		{
			return HashCombineFast(Primitive.PrimitiveComponent->GetUniqueID(), Primitive.InstanceIndex);
		}
	};

	static void ProcessPrimitive(
		FPrimitive const& Primitive,
		FTransform const& GenerationTransform,
		TScriptInterface<IHavokNavNavMeshGenerationController> Controller,
		FHavokNavNavMeshGenerationInputEntitySet& InputEntitiesOut)
	{
		FHavokNavAnyArray& ObjectDatasOut = InputEntitiesOut.PhysicalInputObjectDatas;
		check(ObjectDatasOut.GetElementType() == Controller->GetPhysicalObjectDataType());

		// Speculatively reserve space for the geometry data; this will be popped if not needed by the usage
		int32 DataIndex = ObjectDatasOut.Num();
		FHavokNavAnyRef ObjectData = ObjectDatasOut.Add_GetRef();

		UPrimitiveComponent* PrimitiveComponent = Primitive.PrimitiveComponent;
		using FPoResult = IHavokNavNavMeshGenerationController::FProcessPhysicalObjectResult;
		using FUsage = EHavokNavNavMeshGenerationPhysicalObjectUsage;
		FPoResult ProcessPhysicalObjectResult = Controller->ProcessPrimitiveComponent(PrimitiveComponent, ObjectData);

		if (ProcessPhysicalObjectResult.Usage == FUsage::PaintingVolume && ObjectDatasOut.GetElementType() == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Painting volumes cannot be used if there is no object data type for the controller."));
			ProcessPhysicalObjectResult.Usage = FUsage::None;
		}

		if (ProcessPhysicalObjectResult.Usage != FUsage::WalkableSurface && ProcessPhysicalObjectResult.Usage != FUsage::PaintingVolume)
		{
			ObjectDatasOut.Pop();
			DataIndex = -1;
		}

		if (ProcessPhysicalObjectResult.Usage == FUsage::None)
		{
			return;
		}

		FTransform WorldToGenerationSpaceTransform = GenerationTransform.Inverse();

		FHavokNavNavMeshGenerationPhysicalInputObject& PhysicalInputObject = InputEntitiesOut.PhysicalInputObjects.AddDefaulted_GetRef();

		using UTriangleProvider = UPPHkNavTriangleNavMeshAuthoringComponent_EditorOnly;
		using URampProvider = UPPHkNavRampNavMeshAuthoringComponent_EditorOnly;
		using UPyramidProvider = UPPHkNavPyramidNavMeshAuthoringComponent_EditorOnly;
		if (const UGeometryCollectionComponent* GeometryCollectionComponent = Cast<const UGeometryCollectionComponent>(PrimitiveComponent))
		{
			PhysicalInputObject.GeometryProvider = MakeUnique<FHavokNavGeometryCollectionGeometryProvider>(
				GeometryCollectionComponent, WorldToGenerationSpaceTransform,
				ProcessPhysicalObjectResult.GeometrySource);
		}
		else if ( const URampProvider* RampComponent = Cast<const URampProvider>(PrimitiveComponent))
		{
			Pp::FHavokNavRampGeometryParams RampParams;
			RampComponent->CalcRampGeometryParams(RampParams);
			PhysicalInputObject.GeometryProvider = MakeUnique<Pp::FHavokNavRampGeometryProvider>(RampParams);
		}
		else if ( const UTriangleProvider* TriangleComponent = Cast<const UTriangleProvider>(PrimitiveComponent))
		{
			Pp::FHavokNavTriangleGeometryParams TriangleParams;
			TriangleComponent->CalcTriangleGeometryParams(TriangleParams);
			PhysicalInputObject.GeometryProvider = MakeUnique<Pp::FHavokNavTriangleGeometryProvider>(TriangleParams);
		}
		else if ( const UPyramidProvider* PyramidComponent = Cast<const UPyramidProvider>(PrimitiveComponent))
		{
			Pp::FHavokNavPyramidGeometryParams PyramidParams;
			PyramidComponent->CalcPyramidGeometryParams(PyramidParams);
			PhysicalInputObject.GeometryProvider = MakeUnique<Pp::FHavokNavPyramidGeometryProvider>(PyramidParams);
		}
		else
		{
#if WITH_HAVOK_PHYSICS
			PhysicalInputObject.GeometryProvider = MakeUnique<FHavokNavPhysicsGeometryProvider>(World.Get(), WorldToGenerationSpaceTransform, PrimitiveComponent->GetBodyInstance(NAME_None, true, Primitive.InstanceIndex), ProcessPhysicalObjectResult.GeometrySource);
#else
			PhysicalInputObject.GeometryProvider = MakeUnique<FHavokNavChaosGeometryProvider>(
				PrimitiveComponent->GetBodyInstance(NAME_None, true, Primitive.InstanceIndex),
				WorldToGenerationSpaceTransform, ProcessPhysicalObjectResult.GeometrySource,
				FHavokNavChaosGeometryProvider::EShapeGeometryToCollect::SimpleAndNoCollision);
#endif
		}

		PhysicalInputObject.Usage = ProcessPhysicalObjectResult.Usage;
		PhysicalInputObject.DataIndex = DataIndex;
	}
};

FHavokNavNavMeshGenerationInputEntitySet UPPHkNav_ActorInputEntityGatherer::GatherInputEntities(
	TSubclassOf<UHavokNavNavMeshLayer> Layer, FTransform const& GenerationTransform,
	FHavokNavNavMeshGenerationBounds const& GenerationLocalBounds,
	TScriptInterface<IHavokNavNavMeshGenerationController> Controller, float InputGatheringBoundsExpansion) const
{
	AActor* Actor = TargetActor.Get();
	if (!Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPPHkNav_ActorInputEntityGatherer: TargetActor is not set."));
		return {};
	}

	FHavokNavNavMeshGenerationInputEntitySet Result;
	Result.PhysicalInputObjectDatas.SetElementType(Controller->GetPhysicalObjectDataType());

	// TargetActorのPrimitiveComponentsを収集
	constexpr bool bIncludeFromChildActors = false;
	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
	Actor->GetComponents<UPrimitiveComponent*>(PrimitiveComponents, bIncludeFromChildActors);

	// PrimitiveComponentsからジオメトリを収集
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		using namespace MassPal::Navigation::Private;
		const FPrimitive Prim {.PrimitiveComponent = PrimitiveComponent, .InstanceIndex = INDEX_NONE};
		ProcessPrimitive( Prim, GenerationTransform, Controller, Result);
	}

	// SeedPointなし
	Result.SeedPoints.Empty();
	// ユーザーエッジなし
	Result.UserEdgeDescriptions.EdgePairDescriptions.Empty();
	Result.UserEdgeDescriptions.InformationSet = {};
	return Result;
}
