// Copyright Pocketpair, Inc. All Rights Reserved.

#include "PPHkNav_BoxVolumeBoundsProvider.h"
#include "HavokNavNavVolumeLayer.h"
#include "HavokNavNavVolumeGenerationBounds.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "IPPHkNavVolumeBoundsActor.h"

FHavokNavNavVolumeGenerationBounds UPPHkNav_BoxVolumeBoundsProvider::GetBounds(TSubclassOf<UHavokNavNavVolumeLayer> Layer, FHavokNavAxialTransform const& GenerationAxialTransform) const
{
    FHavokNavNavVolumeGenerationBounds Bounds{};

    // Layer validation
    if (Layer != TargetLayer)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPPHkNav_BoxVolumeBoundsProvider::GetBounds: Layer mismatch. Expected %s, got %s"),
               TargetLayer ? *TargetLayer->GetName() : TEXT("None"),
               Layer ? *Layer->GetName() : TEXT("None"));
        return Bounds; // Return empty bounds
    }

    if (!BoundingBox.IsValid)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPPHkNav_BoxVolumeBoundsProvider::GetBounds: BoundingBox is invalid"));
        return Bounds; // Return empty bounds
    }

    check(NavVolumeBoundsActor.IsValid());
    IPPHkNavVolumeBoundsActor* BoundsActor = NavVolumeBoundsActor.ToScriptInterface().GetInterface();
    if (!BoundsActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPPHkNav_BoxVolumeBoundsProvider::GetBounds: NavVolumeBoundsActor is not valid"));
        return Bounds; // Return empty bounds
    }
    
    // Transform the bounding box to local space
    FTransform GenerationTransform;
    GenerationAxialTransform.ToTransform(GenerationTransform);

    // Set PositiveBound
    const FBox QuantizedBounds = Layer.GetDefaultObject()->QuantizeBounds(BoundingBox);
    Bounds.PositiveBound = QuantizedBounds.InverseTransformBy(GenerationTransform);


    const UWorld* World = NavVolumeBoundsActor.GetObject()->GetWorld();
    // Gather NegativeBounds
    for (AActor const* OtherActor : TActorRange<AActor>(World))
    {
        IPPHkNavVolumeBoundsActor const* OtherNavVolumeBoundsActor = Cast<IPPHkNavVolumeBoundsActor>(OtherActor);
        if (OtherNavVolumeBoundsActor && OtherNavVolumeBoundsActor != BoundsActor && OtherNavVolumeBoundsActor->IsInNavVolumeLayer(Layer) && OtherNavVolumeBoundsActor->Dominates(BoundsActor))
        {
            const FBox OtherRawBound = OtherNavVolumeBoundsActor->GetNavVolumeBounds();
            const FBox OtherQuantizedBounds = Layer.GetDefaultObject()->QuantizeBounds(OtherRawBound);

            // Don't cut out degenerate bounds, or bounds that are only adjacent
            if (QuantizedBounds.Overlap(OtherQuantizedBounds).GetVolume() > 0)
            {
                Bounds.NegativeBounds.Add(OtherQuantizedBounds.InverseTransformBy(GenerationTransform));
            }
        }
    }
    return Bounds;
}
