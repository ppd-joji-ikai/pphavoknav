// Copyright Pocketpair, Inc. All Rights Reserved.

#include "Components/PPHkNavPyramidNavMeshAuthoringComponent_EditorOnly.h"

#include "PpCustom/HavokNavPyramidGeometryProvider.h"
#include "PrimitiveViewRelevance.h"
#include "SceneManagement.h"

namespace PPHkNav::Private
{
	/** SceneProxy */
	class FPyramidNavAuthoringSceneProxy final : public FPrimitiveSceneProxy
	{
	public:
		explicit FPyramidNavAuthoringSceneProxy(const UPPHkNavPyramidNavMeshAuthoringComponent_EditorOnly* InComponent, const float InLineThickness)
			: FPrimitiveSceneProxy(InComponent)
			, Width(InComponent->WidthCentimeters)
			, Depth(InComponent->DepthCentimeters)
			, Height(InComponent->HeightCentimeters)
			, ApexOffset(InComponent->ApexOffsetXYCentimeters)
			, UpVector(InComponent->NavMeshUpVector)
			, BaseColor(InComponent->BaseColor)
			, EdgeColor(InComponent->EdgeColor)
			, ApexColor(InComponent->ApexColor)
			, NormalColor(InComponent->NormalColor)
			, BoxColor(InComponent->ShapeColor)
			, BoxExtents(InComponent->GetUnscaledBoxExtent())
			, LineThickness(InLineThickness)
		
		{
			bWillEverBeLit = false; // 発光不要
		}

		virtual SIZE_T GetTypeHash() const override
		{
			static size_t Unique = 0x9d12a5b3; return Unique;
		}

		virtual void GetDynamicMeshElements(const TArray<const FSceneView *>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
		{
			const float HalfDepth = Depth * 0.5f;
			const float HalfWidth = Width * 0.5f;
			const FVector LocalCorners[4] = {
				FVector(+HalfDepth, +HalfWidth, 0.f),
				FVector(+HalfDepth, -HalfWidth, 0.f),
				FVector(-HalfDepth, -HalfWidth, 0.f),
				FVector(-HalfDepth, +HalfWidth, 0.f)
			};
			const FVector LocalApex( ApexOffset.X, ApexOffset.Y, Height );

			for(int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
			{
				if((VisibilityMap & (1 << ViewIndex)) == 0) { continue; }
				FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

				FVector WorldCorners[4];
				for(int32 i=0;i<4;++i) { WorldCorners[i] = GetLocalToWorld().TransformPosition(LocalCorners[i]); }
				const FVector WorldApex = GetLocalToWorld().TransformPosition(LocalApex);

				// 底面 (ライン)
				for(int32 i=0;i<4;++i)
				{
					const FVector& A = WorldCorners[i];
					const FVector& B = WorldCorners[(i+1)%4];
					PDI->DrawLine(A, B, BaseColor, SDPG_World, 2.f);
				}

				// 側面 (Apex へのエッジ)
				for(int32 i=0;i<4;++i)
				{
					PDI->DrawLine(WorldCorners[i], WorldApex, EdgeColor, SDPG_World, 1.5f);
				}

				// Apex マーカー
				DrawCross(PDI, WorldApex, 8.f, ApexColor);

				// 法線 (UpVector 指定 or UpVector がゼロなら +Z)
				FVector Normal = UpVector.IsNearlyZero() ? FVector::UpVector : UpVector.GetSafeNormal();
				const FVector Center = (WorldCorners[0] + WorldCorners[1] + WorldCorners[2] + WorldCorners[3]) * 0.25f;
				PDI->DrawLine(Center, Center + Normal * 80.f, NormalColor, SDPG_World, 1.5f);

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
		float Width{};
		float Depth{};
		float Height{};
		FVector2D ApexOffset{};
		FVector UpVector{};
		FColor BaseColor{};
		FColor EdgeColor{};
		FColor ApexColor{};
		FColor NormalColor{};
		const FColor BoxColor{};
		const FVector BoxExtents{};
		const float LineThickness{};
	};
}

UPPHkNavPyramidNavMeshAuthoringComponent_EditorOnly::UPPHkNavPyramidNavMeshAuthoringComponent_EditorOnly(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	bHiddenInGame = true; // ゲーム中は非表示 (エディタ用)
	Super::SetCollisionEnabled(ECollisionEnabled::NoCollision);
	bIsEditorOnly = true;
	bSelectable = true;
	bCanEverAffectNavigation = true;
	// 初期 BoxExtent は視認性のため設定 (Depth/2, Width/2, |Height|/2)
	InitBoxExtent(FVector(DepthCentimeters * 0.5f, WidthCentimeters * 0.5f, FMath::Abs(HeightCentimeters) * 0.5f));
}

void UPPHkNavPyramidNavMeshAuthoringComponent_EditorOnly::CalcPyramidGeometryParams(Pp::FHavokNavPyramidGeometryParams& OutParams) const
{
	OutParams.WidthCentimeters = WidthCentimeters;
	OutParams.DepthCentimeters = DepthCentimeters;
	OutParams.HeightCentimeters = HeightCentimeters;
	OutParams.Transform = GetComponentTransform();
	OutParams.UpVector = NavMeshUpVector;
	OutParams.ApexOffsetXYCentimeters = ApexOffsetXYCentimeters;
	OutParams.AntiErosion = bAntiErosionExpand ? TOptional<float>(AntiErosionRadius) : TOptional<float>();
}

FPrimitiveSceneProxy* UPPHkNavPyramidNavMeshAuthoringComponent_EditorOnly::CreateSceneProxy()
{
	return new PPHkNav::Private::FPyramidNavAuthoringSceneProxy(this, LineThickness);
}

FBoxSphereBounds UPPHkNavPyramidNavMeshAuthoringComponent_EditorOnly::CalcBounds(const FTransform& LocalToWorld) const
{
	FBox Box(ForceInit);
	const float HalfDepth = DepthCentimeters * 0.5f;
	const float HalfWidth = WidthCentimeters * 0.5f;
	const FVector Corners[4] = {
		FVector(+HalfDepth, +HalfWidth, 0.f),
		FVector(+HalfDepth, -HalfWidth, 0.f),
		FVector(-HalfDepth, -HalfWidth, 0.f),
		FVector(-HalfDepth, +HalfWidth, 0.f)
	};
	for(const FVector& C : Corners) { Box += LocalToWorld.TransformPosition(C); }
	Box += LocalToWorld.TransformPosition(FVector(ApexOffsetXYCentimeters.X, ApexOffsetXYCentimeters.Y, HeightCentimeters));

	if(!Box.IsValid)
	{
		Box = FBox(LocalToWorld.GetLocation() - FVector(10.f), LocalToWorld.GetLocation() + FVector(10.f));
	}
	return FBoxSphereBounds(Box);
}

#if WITH_EDITOR
void UPPHkNavPyramidNavMeshAuthoringComponent_EditorOnly::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// BoxExtent を寸法に同期
	InitBoxExtent(FVector(DepthCentimeters * 0.5f, WidthCentimeters * 0.5f, FMath::Abs(HeightCentimeters) * 1.0f));
	MarkRenderStateDirty();
	MarkRenderTransformDirty();
}
#endif
