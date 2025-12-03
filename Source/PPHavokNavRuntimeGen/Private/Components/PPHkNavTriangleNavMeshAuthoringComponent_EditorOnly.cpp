// Copyright Pocketpair, Inc. All Rights Reserved.

#include "Components/PPHkNavTriangleNavMeshAuthoringComponent_EditorOnly.h"
#include "PpCustom/HavokNavTriangleGeometryProvider.h"
#include "PrimitiveViewRelevance.h"
#include "SceneManagement.h"

namespace PPHkNav::Private
{
	/** SceneProxy */
	class FTriangleNavAuthoringSceneProxy final : public FPrimitiveSceneProxy
	{
	public:
		explicit FTriangleNavAuthoringSceneProxy(const UPPHkNavTriangleNavMeshAuthoringComponent_EditorOnly* InComponent, const float InLineThickness)
			: FPrimitiveSceneProxy(InComponent)
			, TriangleColor(InComponent->TriangleColor)
			, PrismColor(InComponent->PrismColor)
			, NormalColor(InComponent->NormalColor)
			, VertexColor(InComponent->VertexColor)
			, bGeneratePrism(InComponent->bGeneratePrism)
			, PrismThickness(InComponent->PrismHeight)
			, BoxColor(InComponent->ShapeColor)
			, BoxExtents(InComponent->GetUnscaledBoxExtent())
			, LineThickness(InLineThickness)
		{
			Vertices[0] = InComponent->VertexA;
			Vertices[1] = InComponent->VertexB;
			Vertices[2] = InComponent->VertexC;
			UpVector = InComponent->NavMeshUpVector;
			bWillEverBeLit = false; // 発光不要
		}

		virtual SIZE_T GetTypeHash() const override
		{
			static size_t Unique = 0x3c9f1d71u; return Unique;
		}

		virtual void GetDynamicMeshElements(const TArray<const FSceneView *>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
		{
			const FVector A = GetLocalToWorld().TransformPosition(Vertices[0]);
			const FVector B = GetLocalToWorld().TransformPosition(Vertices[1]);
			const FVector C = GetLocalToWorld().TransformPosition(Vertices[2]);

			FVector Normal;
			if(!UpVector.IsNearlyZero())
			{
				Normal = UpVector.GetSafeNormal();
			}
			else
			{
				Normal = FVector::CrossProduct((B - A), (C - A)).GetSafeNormal();
			}
			if(!Normal.IsNearlyZero())
			{
				Normal = FVector::UpVector;
			}
			const FVector Center = (A + B + C) / 3.f;

			for(int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
			{
				if((VisibilityMap & (1 << ViewIndex)) == 0) { continue; }
				FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

				// 三角形エッジ
				PDI->DrawLine(A, B, TriangleColor, SDPG_World, 2.f);
				PDI->DrawLine(B, C, TriangleColor, SDPG_World, 2.f);
				PDI->DrawLine(C, A, TriangleColor, SDPG_World, 2.f);

				// 頂点を小さなクロスで表現
				DrawCross(PDI, A, 6.f, VertexColor);
				DrawCross(PDI, B, 6.f, VertexColor);
				DrawCross(PDI, C, 6.f, VertexColor);

				// 法線
				PDI->DrawLine(Center, Center + Normal * 80.f, NormalColor, SDPG_World, 1.5f);

				if(bGeneratePrism && PrismThickness > KINDA_SMALL_NUMBER)
				{
					const FVector Offset = Normal * PrismThickness;
					const FVector ATop = A + Offset;
					const FVector BTop = B + Offset;
					const FVector CTop = C + Offset;

					// 上面
					PDI->DrawLine(ATop, BTop, PrismColor, SDPG_World, 1.5f);
					PDI->DrawLine(BTop, CTop, PrismColor, SDPG_World, 1.5f);
					PDI->DrawLine(CTop, ATop, PrismColor, SDPG_World, 1.5f);
					// 側面
					PDI->DrawLine(A, ATop, PrismColor, SDPG_World, 1.0f);
					PDI->DrawLine(B, BTop, PrismColor, SDPG_World, 1.0f);
					PDI->DrawLine(C, CTop, PrismColor, SDPG_World, 1.0f);
				}

				// Box
				const FLinearColor DrawColor = GetViewSelectionColor(BoxColor, *Views[ViewIndex], IsSelected(), IsHovered(), false, IsIndividuallySelected() );
				const FMatrix& LToW = GetLocalToWorld();
				DrawOrientedWireBox(PDI, LToW.GetOrigin(), LToW.GetScaledAxis( EAxis::X ), LToW.GetScaledAxis( EAxis::Y ), LToW.GetScaledAxis( EAxis::Z ), BoxExtents, DrawColor, SDPG_World, LineThickness);
			}
		}

		virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
		{
			FPrimitiveViewRelevance Result;
			Result.bDrawRelevance = IsShown(View);
			Result.bDynamicRelevance = true;
			Result.bOpaque = false;
			Result.bSeparateTranslucency = false;
			Result.bVelocityRelevance = false;
			return Result;
		}

		virtual bool CanBeOccluded() const override { return false; }
		virtual uint32 GetMemoryFootprint() const override { return sizeof(*this) + GetAllocatedSize(); }

	private:
		static void DrawCross(FPrimitiveDrawInterface* PDI, const FVector& Pos, float Size, const FLinearColor& Color)
		{
			const FVector X(Size,0,0); const FVector Y(0,Size,0); const FVector Z(0,0,Size);
			PDI->DrawLine(Pos - X, Pos + X, Color, SDPG_World, 1.f);
			PDI->DrawLine(Pos - Y, Pos + Y, Color, SDPG_World, 1.f);
			PDI->DrawLine(Pos - Z, Pos + Z, Color, SDPG_World, 1.f);
		}

	private:
		FVector Vertices[3]{};
		FVector UpVector{}; 
		FColor TriangleColor{};
		FColor PrismColor{};
		FColor NormalColor{};
		FColor VertexColor{};
		bool bGeneratePrism{};
		float PrismThickness{};
		const FColor BoxColor{};
		const FVector BoxExtents{};
		const float LineThickness{};
	};
} // namespace MassPalNavGen::Private

/**
 * UPPHkNavTriangleNavMeshAuthoringComponent_EditorOnly
 */
UPPHkNavTriangleNavMeshAuthoringComponent_EditorOnly::UPPHkNavTriangleNavMeshAuthoringComponent_EditorOnly(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	bHiddenInGame = true; // ゲーム中は非表示 (エディタ用)
	Super::SetCollisionEnabled(ECollisionEnabled::NoCollision);
	bIsEditorOnly = true;
	bSelectable = true;
	bCanEverAffectNavigation = true;
	InitBoxExtent(FVector(200.0f, 200.0f, 200.0f));
}

void UPPHkNavTriangleNavMeshAuthoringComponent_EditorOnly::CalcTriangleGeometryParams(Pp::FHavokNavTriangleGeometryParams& OutParams) const
{
	OutParams.VerticesLocal[0] = VertexA;
	OutParams.VerticesLocal[1] = VertexB;
	OutParams.VerticesLocal[2] = VertexC;
	OutParams.Transform = GetComponentTransform();
	OutParams.UpVector = NavMeshUpVector;
	OutParams.PrismHeight = bGeneratePrism ? TOptional<float>(PrismHeight) : TOptional<float>();
	OutParams.AntiErosion = bAntiErosionExpand ? TOptional<float>(AntiErosionRadius) : TOptional<float>();
}

FPrimitiveSceneProxy* UPPHkNavTriangleNavMeshAuthoringComponent_EditorOnly::CreateSceneProxy()
{
	return new PPHkNav::Private::FTriangleNavAuthoringSceneProxy(this, LineThickness);
}

FBoxSphereBounds UPPHkNavTriangleNavMeshAuthoringComponent_EditorOnly::CalcBounds(const FTransform& LocalToWorld) const
{
	FBox Box(ForceInit);
	Box += LocalToWorld.TransformPosition(VertexA);
	Box += LocalToWorld.TransformPosition(VertexB);
	Box += LocalToWorld.TransformPosition(VertexC);

	FVector Normal = NavMeshUpVector;
	if(Normal.IsNearlyZero())
	{
		const FVector A = VertexA; const FVector B = VertexB; const FVector C = VertexC;
		Normal = FVector::CrossProduct((B - A),(C - A)).GetSafeNormal();
	}
	if(bGeneratePrism && PrismHeight > KINDA_SMALL_NUMBER)
	{
		Box += LocalToWorld.TransformPosition(VertexA + Normal * PrismHeight);
		Box += LocalToWorld.TransformPosition(VertexB + Normal * PrismHeight);
		Box += LocalToWorld.TransformPosition(VertexC + Normal * PrismHeight);
	}

	if(!Box.IsValid)
	{
		Box = FBox(LocalToWorld.GetLocation() - FVector(10.f), LocalToWorld.GetLocation() + FVector(10.f));
	}
	return FBoxSphereBounds(Box);
}

#if WITH_EDITOR
void UPPHkNavTriangleNavMeshAuthoringComponent_EditorOnly::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// プロパティ変更により再描画
	MarkRenderStateDirty();
	MarkRenderTransformDirty();
}
#endif
