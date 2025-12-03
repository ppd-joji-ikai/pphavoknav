// Copyright Pocketpair, Inc. All Rights Reserved.
#pragma once

#include "ComponentVisualizer.h"

class UHavokNavNavMesh;

/**
 * UPPHkNavInstancedNavMeshComponent 用 ビジュアライザ
 */
class FPPHkNavInstancedNavMeshComponentVisualizer : public FComponentVisualizer
{
public:
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View,FPrimitiveDrawInterface* PDI) override;

private:
	void BuildLocalMeshIfNeeded(UHavokNavNavMesh* NavMesh);
	void InvalidateCache(UHavokNavNavMesh* NewNavMesh);

private:
	struct FTri
	{
		int32 A{};
		int32 B{};
		int32 C{};
	};

	TArray<FVector> CachedLocalVertices{};
	TArray<FTri> CachedTris{};
	FBox CachedLocalBounds = FBox(ForceInit);
	bool bMeshBuilt = false;

	// 変更検知用
	TWeakObjectPtr<UHavokNavNavMesh> CachedNavMesh{};
	FGuid CachedNavMeshGuid{};
};
