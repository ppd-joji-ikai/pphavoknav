// Copyright Pocketpair, Inc. All Rights Reserved.
#include "PPHkNavInstancedNavMeshComponentVisualizer.h"

#include "../../HavokNavigation/Private/HavokNavIncludes.h"
#include "HavokNavNavMesh.h"
#include "CustomData/PPHkNavPatchCustomDataNavMeshLayer.h"
#include "PPHkNav_NavMeshDataAsset.h"
#include "SceneManagement.h"
#include "Components/PPHkNavInstancedNavMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Materials/MaterialRenderProxy.h"

// ワイヤ重複排除用キー
namespace MassPal::Navigation
{
struct FNavMeshEdgeKey
{
	int32 A{-1};
	int32 B{-1};
	bool operator==(const FNavMeshEdgeKey& Other) const
	{
		return (A == Other.A && B == Other.B) || (A == Other.B && B == Other.A);
	}
};
	
FORCEINLINE uint32 GetTypeHash(const FNavMeshEdgeKey& K)
{
	const int32 MinV = FMath::Min(K.A, K.B);
	const int32 MaxV = FMath::Max(K.A, K.B);
	return HashCombine(::GetTypeHash(MinV), ::GetTypeHash(MaxV));
}
} // namespace MassPal::Navigation


#define LOCTEXT_NAMESPACE "FPPHkNavInstancedNavMeshComponentVisualizer"

void FPPHkNavInstancedNavMeshComponentVisualizer::BuildLocalMeshIfNeeded(UHavokNavNavMesh* NavMesh)
{
	if (bMeshBuilt || !NavMesh)
	{
		return;
	}
	const hkaiNavMesh* HkNavMesh = NavMesh->GetHkNavMesh();
	if (!HkNavMesh)
	{
		return;
	}

	// 頂点キャッシュ
	const int32 NumVerts = HkNavMesh->getNumVertices();
	CachedLocalVertices.Reset(NumVerts);
	CachedLocalVertices.AddDefaulted(NumVerts);
	for (int32 VIdx = 0; VIdx < NumVerts; ++VIdx)
	{
		hkVector4 HkPos; HkNavMesh->getVertexPosition(VIdx, HkPos);
		FVector UEPos; Hk2ULengthVector(HkPos, UEPos); // Havok長さ単位 -> UE 単位
		CachedLocalVertices[VIdx] = UEPos;
		CachedLocalBounds += UEPos;
	}

	// フェース -> 三角形 (扇形三角化)
	const int32 NumFaces = HkNavMesh->getNumFaces();
	CachedTris.Reset();
	for (int32 FaceIndex = 0; FaceIndex < NumFaces; ++FaceIndex)
	{
		const hkaiNavMesh::Face& Face = HkNavMesh->getFace(FaceIndex);
		if (Face.m_numEdges < 3) { continue; }
		// 最初のエッジから開始
		const int32 StartEdge = Face.m_startEdgeIndex;
		const hkaiNavMesh::Edge& FirstEdge = HkNavMesh->getEdge(StartEdge);
		const int32 FirstVertex = FirstEdge.m_a; // 扇の中心
		int32 PrevVertex = FirstEdge.m_b;
		for (int32 EdgeOffset = 1; EdgeOffset < Face.m_numEdges; ++EdgeOffset)
		{
			const int32 EdgeIdx = StartEdge + EdgeOffset;
			const hkaiNavMesh::Edge& Edge = HkNavMesh->getEdge(EdgeIdx);
			const int32 CurrVertex = Edge.m_b; // 連続辺の b が次頂点
			// 三角形 (First, Prev, Curr)
			if (FirstVertex != PrevVertex && FirstVertex != CurrVertex && PrevVertex != CurrVertex)
			{
				CachedTris.Add({ FirstVertex, PrevVertex, CurrVertex });
			}
			PrevVertex = CurrVertex;
		}
	}

	bMeshBuilt = true;
	if (!CachedLocalBounds.IsValid)
	{
		CachedLocalBounds = NavMesh->GetBoundingBox();
	}
}

void FPPHkNavInstancedNavMeshComponentVisualizer::InvalidateCache(UHavokNavNavMesh* NewNavMesh)
{
	CachedNavMesh = NewNavMesh;
	CachedNavMeshGuid = (NewNavMesh ? NewNavMesh->GetGuid() : FGuid());
	CachedLocalVertices.Reset();
	CachedTris.Reset();
	CachedLocalBounds = FBox(ForceInit);
	bMeshBuilt = false;
}

void FPPHkNavInstancedNavMeshComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const UPPHkNavInstancedNavMeshComponent* Comp = Cast<UPPHkNavInstancedNavMeshComponent>(Component);
	if (!Comp)
	{
		return;
	}
#if WITH_EDITOR
	UPPHkNav_NavMeshDataAsset* DataAsset = Comp->GetNavMeshDataAsset();
	if (!DataAsset || !DataAsset->PreBakedNavMesh)
	{
		return;
	}

	// 差分検知
	UHavokNavNavMesh* NavMesh = DataAsset->PreBakedNavMesh;
	bool bNeedInvalidate = false;
	if (NavMesh != CachedNavMesh.Get()) { bNeedInvalidate = true; }
	else if (NavMesh && NavMesh->GetGuid() != CachedNavMeshGuid) { bNeedInvalidate = true; }
	if (bNeedInvalidate)
	{
		InvalidateCache(NavMesh);
	}

	BuildLocalMeshIfNeeded(NavMesh);

	if (CachedLocalVertices.Num() == 0 || CachedTris.Num() == 0)
	{
		// フォールバック: バウンディングボックスのみ
		const FBox LocalBox = DataAsset->PreBakedNavMesh->GetBoundingBox();
		if (!LocalBox.IsValid)
		{
			return;
		}
		FVector BMin = LocalBox.Min; FVector BMax = LocalBox.Max;
		FVector V[8] = {
			{BMin.X,BMin.Y,BMin.Z},{BMax.X,BMin.Y,BMin.Z},{BMax.X,BMax.Y,BMin.Z},{BMin.X,BMax.Y,BMin.Z},
			{BMin.X,BMin.Y,BMax.Z},{BMax.X,BMin.Y,BMax.Z},{BMax.X,BMax.Y,BMax.Z},{BMin.X,BMax.Y,BMax.Z}
		};
		int32 E[12][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
		const FTransform WorldXf = Comp->GetComponentTransform();
		const FLinearColor BoxColor = FColor::Cyan;
		for (int i=0;i<12;++i)
		{
			PDI->DrawLine(WorldXf.TransformPosition(V[E[i][0]]), WorldXf.TransformPosition(V[E[i][1]]), BoxColor, SDPG_Foreground, 1.5f);
		}
		return;
	}

	// 描画カラー (固定)
	const FLinearColor FillColor = UPPHkNavPatchCustomDataNavMeshLayer::GetFaceDisplayColorStatic();
	const FLinearColor WireColor = UPPHkNavPatchCustomDataNavMeshLayer::GetEdgeDisplayColorStatic();
	const FTransform WorldXf = Comp->GetComponentTransform();

	// 塗りつぶし (三角ごと) - 透明度低
	if (GEngine && GEngine->DebugMeshMaterial)
	{
		static FColoredMaterialRenderProxy* CachedMatProxy = nullptr;
		if (!CachedMatProxy || CachedMatProxy->Parent != GEngine->DebugMeshMaterial->GetRenderProxy())
		{
			// 再生成（リーク防止のため旧オブジェクトを破棄: Editor 終了時 OS 解放だが一応）
			delete CachedMatProxy;
			CachedMatProxy = new FColoredMaterialRenderProxy(GEngine->DebugMeshMaterial->GetRenderProxy(), FillColor);
		}
		for (const FTri& Tri : CachedTris)
		{
			const FVector A = WorldXf.TransformPosition(CachedLocalVertices[Tri.A]);
			const FVector B = WorldXf.TransformPosition(CachedLocalVertices[Tri.B]);
			const FVector C = WorldXf.TransformPosition(CachedLocalVertices[Tri.C]);
			::DrawTriangle(PDI, A,B,C, CachedMatProxy, SDPG_Foreground);
		}
	}

	// ワイヤ (エッジ重複を避けるためセット化)
	TSet<MassPal::Navigation::FNavMeshEdgeKey> DrawnEdges;
	DrawnEdges.Reserve(CachedTris.Num()*3);
	for (const FTri& Tri : CachedTris)
	{
		int32 Idx[3] = {Tri.A, Tri.B, Tri.C};
		for (int i=0;i<3;++i)
		{
			const MassPal::Navigation::FNavMeshEdgeKey K{Idx[i], Idx[(i+1)%3]};
			if (DrawnEdges.Contains(K))
			{
				continue;
			}
			DrawnEdges.Add(K);
			
			const FVector A = WorldXf.TransformPosition(CachedLocalVertices[K.A]);
			const FVector B = WorldXf.TransformPosition(CachedLocalVertices[K.B]);
			PDI->DrawLine(A,B,WireColor, SDPG_Foreground, 1.0f);
		}
	}

	// ユーザーエッジ描画
	const FTransform CompWorld = Comp->GetComponentTransform();
	const TArray<FPPHkNavBakedNavMeshUserEdgeSocket>& Sockets = DataAsset->BakedEdgeSockets;
	for (const FPPHkNavBakedNavMeshUserEdgeSocket& S : Sockets)
	{
		// 位置+ Forward (ローカル->World)
		const FVector StartLocW = CompWorld.TransformPosition(S.CenterStart);
		const FVector EndLocW   = CompWorld.TransformPosition(S.CenterEnd);
		PDI->DrawLine(StartLocW, EndLocW, WireColor, SDPG_Foreground, 1.0f);
	}
	
#endif // WITH_EDITOR
}

#undef LOCTEXT_NAMESPACE
