// Copyright (c) 2024 Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if !WITH_HAVOK_PHYSICS
#include "IHavokNavGeometryProvider.h"
#include "Chaos/ImplicitObject.h"
#include "HavokNavGeometrySource.h"
#include "HavokNavChaosGeometryProvider.h" // For EShapeGeometryToCollect

namespace Chaos
{
	struct FPhysicsObject;
	using FPhysicsObjectHandle = FPhysicsObject*;
}

/**
 * @brief Provides geometry from a Chaos FPhysicsObjectHandle for Havok NavMesh generation.
 * This class is suitable for generating NavMesh based on the collision geometry of a physics object.
 */
class PPHKNAVRUNTIMEGEN_API FPPHkNav_ChaosGeometryProvider final : public FHavokNavChaosGeometryProvider
{
public:
	using Super = FHavokNavChaosGeometryProvider;
	FPPHkNav_ChaosGeometryProvider(const Chaos::FPhysicsObjectHandle PhysicsObject, FTransform const& Transform, EHavokNavGeometrySource Source, FHavokNavChaosGeometryProvider::EShapeGeometryToCollect ShapeGeometryToCollect = FHavokNavChaosGeometryProvider::EShapeGeometryToCollect::Simple);

	//~ Begin IHavokNavGeometryProvider Interface
	// virtual FHavokNavUtilities::THkReferencedObject<class hkaiVolume> GetVolume() const override;
	// virtual void AppendGeometry(struct hkGeometry& GeometryOut, int Material, FHeightfieldGeometrySimplificationSettings const& HeightfieldSimplificationSettings = FHeightfieldGeometrySimplificationSettings()) const override;
	//~ End IHavokNavGeometryProvider Interface

protected:
	// // The source of the geometry to be collected.
	// EHavokNavGeometrySource Source;
	//
	// // The transform to apply to the collected geometry.
	// FTransform GeometryTransform;
	//
	// // The collected geometry objects.
	// TArray<Chaos::FImplicitObjectPtr, TInlineAllocator<2>> Geometries;
};
#endif // !WITH_HAVOK_PHYSICS

