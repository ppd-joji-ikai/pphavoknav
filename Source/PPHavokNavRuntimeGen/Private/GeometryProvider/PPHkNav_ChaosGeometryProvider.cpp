// Copyright Pocketpair, Inc. All Rights Reserved.

#include "PPHkNav_ChaosGeometryProvider.h"

#if !WITH_HAVOK_PHYSICS
#include "PhysicsPublic.h"
#include "Engine/World.h"
#include "Physics/PhysicsFiltering.h"
#include "PhysicsProxy/SingleParticlePhysicsProxy.h"
#include "Runtime/Experimental/Chaos/Private/Chaos/PhysicsObjectInternal.h"

// Copied from HavokNavChaosGeometryProvider.cpp
namespace
{
}

FPPHkNav_ChaosGeometryProvider::FPPHkNav_ChaosGeometryProvider(const Chaos::FPhysicsObjectHandle PhysicsObject, FTransform const& Transform, EHavokNavGeometrySource Source, FHavokNavChaosGeometryProvider::EShapeGeometryToCollect ShapeGeometryToCollect)
	: Super()
{
	using namespace Chaos;

	this->Source = Source;
	const IPhysicsProxyBase* PProxy = PhysicsObject->PhysicsProxy();
	if (PProxy->GetType() != EPhysicsProxyType::SingleParticleProxy)
	{
		GeometryTransform = Transform;
		Geometries.Reset();
		unimplemented(); // This provider is only for single particle proxies, so we don't handle other types here.
		return;
	}
	
	const FSingleParticlePhysicsProxy* ParticleProxy = static_cast<const FSingleParticlePhysicsProxy*>(PProxy);
	const FTransform BodyTransform = FRigidTransform3(ParticleProxy->GetGameThreadAPI().X(), ParticleProxy->GetGameThreadAPI().R());
	
	GeometryTransform = BodyTransform * Transform;

	switch (Source)
	{
	case EHavokNavGeometrySource::BoundingBox:
	{
		// Retrieve Geometry's bounding box and create TBox from it
		const FAABB3 BoundingBox = ParticleProxy->GetGameThreadAPI().GetGeometry()->BoundingBox();

		Geometries.Add(MakeImplicitObjectPtr<FImplicitBox3>(BoundingBox.Min(), BoundingBox.Max()));
		break;
	}
	case EHavokNavGeometrySource::SingleConvex:
		// Currently, handle as SimpleCollision
	case EHavokNavGeometrySource::SimpleCollision:
	{
		const FShapesArray& Shapes = ParticleProxy->GetGameThreadAPI().ShapesArray();

		for (const TUniquePtr<FPerShapeData>& ShapeData : Shapes)
		{
			// Filter out complex-only geometry, but potentially allow "NoCollision" geometry depending on enum.
			FCollisionFilterData FilterData = ShapeData->GetQueryData();
			const bool bShapeIsComplex = (FilterData.Word3 & EPDF_ComplexCollision) != 0;
			const bool bShapeIsSimple = (FilterData.Word3 & EPDF_SimpleCollision) != 0;

			switch(ShapeGeometryToCollect)
			{
				case FHavokNavChaosGeometryProvider::EShapeGeometryToCollect::Simple:
				{
					if (!bShapeIsSimple)
					{
						continue;
					}
					break;
				}
				case FHavokNavChaosGeometryProvider::EShapeGeometryToCollect::SimpleAndNoCollision:
				{
					if (!bShapeIsSimple && bShapeIsComplex)
					{
						continue;
					}
					break;
				}
				default:
				{
					checkf(false, TEXT("Unsupported type for shape geometry to collect."));
					break;
				}
			}

			// filter out shapes that are marked as only being visible
			FCollisionFilterData SimData = ShapeData->GetSimData();
			if (SimData.Word1 == (1 << ECollisionChannel::ECC_Visibility) &&
				(SimData.Word3 & (ECollisionChannel::ECC_Visibility << NumFilterDataFlagBits)) > 0)
			{
				continue;
			}

			// Save geometry for further processing
			Geometries.Add(ShapeData->GetGeometry());
		}
		break;
	}
	default:
		unimplemented();
		break;
	}
}

// FHavokNavUtilities::THkReferencedObject<class hkaiVolume> FPPHkNav_ChaosGeometryProvider::GetVolume() const
// {
// 	// there has to be a better way to do this
// 	hkGeometry Geom;
// 	AppendGeometry(Geom, 0);
//
// 	hkRefPtr<hkaiPlaneVolume> PlaneVolume = hk::makeRef<hkaiPlaneVolume>();
// 	hkaiPlaneVolume::createConvexVolume(Geom.m_vertices.begin(), Geom.m_vertices.getSize(), *PlaneVolume);
// 	return PlaneVolume;
// }
//
// void FPPHkNav_ChaosGeometryProvider::AppendGeometry(struct hkGeometry& GeometryOut, int Material, FHeightfieldGeometrySimplificationSettings const& HeightfieldSimplificationSettings) const
// {
// 	for (const Chaos::FImplicitObject* Geometry : Geometries)
// 	{
// 		AppendChaosGeometryToHavokGeometry(Geometry, GeometryTransform, Material, HeightfieldSimplificationSettings, GeometryOut);
// 	}
// }

#endif // !WITH_HAVOK_PHYSICS

