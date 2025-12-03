// Copyright Pocketpair, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"

class UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly;

/**
 * 複数 UserEdge ソケット用ビジュアライザ
 * @see UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly
 */
class FPPHkNavMultiUserEdgeSocketComponentVisualizer : public FComponentVisualizer
{
public:
    virtual ~FPPHkNavMultiUserEdgeSocketComponentVisualizer() override = default;

    virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
};

