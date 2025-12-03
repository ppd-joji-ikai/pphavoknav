// Copyright Pocketpair, Inc. All Rights Reserved.

#include "Components/PPHkNavRampNavMeshAuthoringComponent_EditorOnly.h"
#include "PpCustom/HavokNavRampGeometryProvider.h"
#include "PrimitiveViewRelevance.h"
#include "SceneManagement.h"

namespace PPHkNav::Private
{
	/** Ramp 可視化用 SceneProxy */
	class FRampNavAuthoringSceneProxy final : public FPrimitiveSceneProxy
	{
	public:
		explicit FRampNavAuthoringSceneProxy(const UPPHkNavRampNavMeshAuthoringComponent_EditorOnly* InComponent, float InLineThickness)
			: FPrimitiveSceneProxy(InComponent)
			, RampColor(InComponent->RampColor)
			, NormalColor(InComponent->NormalColor)
			, BoxColor(InComponent->ShapeColor)
			, BoxExtents(InComponent->GetUnscaledBoxExtent())
			, LineThickness(InLineThickness)
		{
			bWillEverBeLit = false; // 発光不要
		}

		virtual SIZE_T GetTypeHash() const override
		{
			static size_t Unique = 0x8b3f65e1u; return Unique;
		}

		virtual void GetDynamicMeshElements(const TArray<const FSceneView *>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
		{
			// ローカル空間で Box 中心が原点。底面は Z = -Extents.Z、前端(+X) が高端。
			const FVector Ext = BoxExtents;
			const FVector BL(-Ext.X, -Ext.Y, -Ext.Z); // Back Left (低)
			const FVector BR(-Ext.X,  Ext.Y, -Ext.Z); // Back Right (低)
			const FVector FL( Ext.X, -Ext.Y,  Ext.Z); // Front Left (高)
			const FVector FR( Ext.X,  Ext.Y,  Ext.Z); // Front Right (高)

			// 法線（面 BL->FL と BL->BR の外積）
			FVector Normal = FVector::CrossProduct(FL - BL, BR - BL).GetSafeNormal();
			if(!Normal.IsNearlyZero())
			{
				// 上向きでなければ反転
				if(Normal.Z < 0.f) { Normal *= -1.f; }
			}

			const FMatrix ToWorld = GetLocalToWorld();
			const FVector WBL = ToWorld.TransformPosition(BL);
			const FVector WBR = ToWorld.TransformPosition(BR);
			const FVector WFL = ToWorld.TransformPosition(FL);
			const FVector WFR = ToWorld.TransformPosition(FR);
			const FVector Center = (WBL + WBR + WFL + WFR) * 0.25f;
			const FVector WNormal = ToWorld.TransformVector(Normal).GetSafeNormal();

			for(int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
			{
				if((VisibilityMap & (1 << ViewIndex)) == 0) { continue; }
				FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

				// Ramp輪郭 (台形側面 + 上下面必要線分)
				PDI->DrawLine(WBL, WBR, RampColor, SDPG_World, 2.f); // 後ろ底辺
				PDI->DrawLine(WBL, WFL, RampColor, SDPG_World, 2.f); // 左側面
				PDI->DrawLine(WBR, WFR, RampColor, SDPG_World, 2.f); // 右側面
				PDI->DrawLine(WFL, WFR, RampColor, SDPG_World, 2.f); // 前上辺
				// 斜面の対角 (オプション) 可視化: BL -> FR / BR -> FL (少しアルファを落とした線)
				const FLinearColor DiagonalColor = FLinearColor(RampColor) * 0.7f; // 明度を下げて区別
				PDI->DrawLine(WBL, WFR, DiagonalColor, SDPG_World, 1.f);
				PDI->DrawLine(WBR, WFL, DiagonalColor, SDPG_World, 1.f);

				// 法線ベクトル
				PDI->DrawLine(Center, Center + WNormal * 120.f, NormalColor, SDPG_World, 1.5f);

				// Box のワイヤ表示 (選択状態カラー)
				const FLinearColor DrawColor = GetViewSelectionColor(BoxColor, *Views[ViewIndex], IsSelected(), IsHovered(), false, IsIndividuallySelected());
				DrawOrientedWireBox(PDI, ToWorld.GetOrigin(), ToWorld.GetScaledAxis(EAxis::X), ToWorld.GetScaledAxis(EAxis::Y), ToWorld.GetScaledAxis(EAxis::Z), BoxExtents, DrawColor, SDPG_World, LineThickness);
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
		const FColor RampColor{};
		const FColor NormalColor{};
		const FColor BoxColor{};
		const FVector BoxExtents{};
		const float LineThickness{};
	};
} // namespace MassPalNavGen::Private

UPPHkNavRampNavMeshAuthoringComponent_EditorOnly::UPPHkNavRampNavMeshAuthoringComponent_EditorOnly()
	: Super()
{
	PrimaryComponentTick.bCanEverTick = false;
	bCanEverAffectNavigation = true;
	bIsEditorOnly = true;
	Super::SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetHiddenInGame(true);
	InitBoxExtent(FVector(200.0f, 200.0f, 162.5f));
}

void UPPHkNavRampNavMeshAuthoringComponent_EditorOnly::CalcRampGeometryParams(Pp::FHavokNavRampGeometryParams& OutParams) const
{
	const FVector UnscaledExtent = GetUnscaledBoxExtent();

	// ローカル寸法 (未スケール) を設定。Transform 側のスケールを適用させる。
	OutParams.HeightCentimeters = UnscaledExtent.Z * 2.0f;
	OutParams.DepthCentimeters  = UnscaledExtent.X * 2.0f;
	OutParams.WidthCentimeters  = UnscaledExtent.Y * 2.0f;
	OutParams.Transform = GetComponentTransform();
	OutParams.AntiErosion = bAntiErosionExpand ? TOptional<float>(AntiErosionRadius) : TOptional<float>();
}

FPrimitiveSceneProxy* UPPHkNavRampNavMeshAuthoringComponent_EditorOnly::CreateSceneProxy()
{
	return new PPHkNav::Private::FRampNavAuthoringSceneProxy(this, LineThickness);
}

#if WITH_EDITOR
void UPPHkNavRampNavMeshAuthoringComponent_EditorOnly::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	MarkRenderStateDirty();
	MarkRenderTransformDirty();
}
#endif
