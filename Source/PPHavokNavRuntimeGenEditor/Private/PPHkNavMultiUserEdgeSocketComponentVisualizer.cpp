// Copyright Pocketpair, Inc. All Rights Reserved.
#include "PPHkNavMultiUserEdgeSocketComponentVisualizer.h"

#include "PPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent.h"
#include "SceneManagement.h"
#include "EditorViewportClient.h"
#include "HitProxies.h"
#include "Editor/UnrealEdEngine.h"

// 矢印描画ヘルパ (Havok 実装を直接参照せず、同等のビジュアルを目指した独自版)
namespace PPHkNavRuntimeGenEditor::Private
{
    static void DrawWireArrow(FPrimitiveDrawInterface* PDI,
                              const FVector& Base,
                              const FVector& LengthExtent,
                              const FVector& WidthHalfExtent,
                              const FColor& Color,
                              const uint8 DepthPriority)
    {
        FVector ArrowPoints[7];
        //base points
        ArrowPoints[0] = Base - WidthHalfExtent * .5f;
        ArrowPoints[1] = Base + WidthHalfExtent * .5f;
        //inner head
        ArrowPoints[2] = ArrowPoints[0] + LengthExtent * 0.333f;
        ArrowPoints[3] = ArrowPoints[1] + LengthExtent * 0.333f;
        //outer head
        ArrowPoints[4] = ArrowPoints[2] - WidthHalfExtent;
        ArrowPoints[5] = ArrowPoints[3] + WidthHalfExtent;
        //tip
        ArrowPoints[6] = Base + LengthExtent;

        //base
        PDI->DrawLine(ArrowPoints[0], ArrowPoints[1], Color, DepthPriority);
        //base sides
        PDI->DrawLine(ArrowPoints[0], ArrowPoints[2], Color, DepthPriority);
        PDI->DrawLine(ArrowPoints[1], ArrowPoints[3], Color, DepthPriority);
        //head base
        PDI->DrawLine(ArrowPoints[2], ArrowPoints[4], Color, DepthPriority);
        PDI->DrawLine(ArrowPoints[3], ArrowPoints[5], Color, DepthPriority);
        //head sides
        PDI->DrawLine(ArrowPoints[4], ArrowPoints[6], Color, DepthPriority);
        PDI->DrawLine(ArrowPoints[5], ArrowPoints[6], Color, DepthPriority);
    }

    // 太めの双方向矢印を描画する
    // <==>
    static void DrawBidirectionalWireArrow(FPrimitiveDrawInterface* PDI,
                                           const FVector& Base1,
                                           const FVector& Base2,
                                           const FVector& WidthHalfExtent,
                                           const FColor& Color,
                                           const uint8 DepthPriority)
    {
        FVector ArrowPoints[10];
        FVector Base1To2 = Base2 - Base1;
        if(Base1To2.IsNearlyZero()) { return; }
        
        //base1 points
        ArrowPoints[0] = Base1 - WidthHalfExtent * .5f + Base1To2 * .3f;
        ArrowPoints[1] = Base1 + WidthHalfExtent * .5f + Base1To2 * .3f;
        //base2 points
        ArrowPoints[2] = Base2 - WidthHalfExtent * .5f - Base1To2 * .3f;
        ArrowPoints[3] = Base2 + WidthHalfExtent * .5f - Base1To2 * .3f;
        //outer head1
        ArrowPoints[4] = ArrowPoints[0] - WidthHalfExtent * .5f;
        ArrowPoints[5] = ArrowPoints[1] + WidthHalfExtent * .5f;
        //outer head2
        ArrowPoints[6] = ArrowPoints[2] - WidthHalfExtent * .5f;
        ArrowPoints[7] = ArrowPoints[3] + WidthHalfExtent * .5f;
        //tip1
        ArrowPoints[8] = Base1;
        //tip2
        ArrowPoints[9] = Base2;

        //base sides
        PDI->DrawLine(ArrowPoints[0], ArrowPoints[2], Color, DepthPriority);
        PDI->DrawLine(ArrowPoints[1], ArrowPoints[3], Color, DepthPriority);
        //head bases
        PDI->DrawLine(ArrowPoints[0], ArrowPoints[4], Color, DepthPriority);
        PDI->DrawLine(ArrowPoints[1], ArrowPoints[5], Color, DepthPriority);
        PDI->DrawLine(ArrowPoints[2], ArrowPoints[6], Color, DepthPriority);
        PDI->DrawLine(ArrowPoints[3], ArrowPoints[7], Color, DepthPriority);
        //head sides
        PDI->DrawLine(ArrowPoints[8], ArrowPoints[4], Color, DepthPriority);
        PDI->DrawLine(ArrowPoints[8], ArrowPoints[5], Color, DepthPriority);
        PDI->DrawLine(ArrowPoints[9], ArrowPoints[6], Color, DepthPriority);
        PDI->DrawLine(ArrowPoints[9], ArrowPoints[7], Color, DepthPriority);
    }
}

#define LOCTEXT_NAMESPACE "FMultiUserEdgeSocketComponentVisualizer"

void FPPHkNavMultiUserEdgeSocketComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
    const auto* Comp = Cast<UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly>(Component);
    if(!Comp) { return; }

    const TArray<FPPHkNavBakedNavMeshUserEdgeSocket>& Sockets = Comp->GetSocketDefinitions();
    if(Sockets.IsEmpty()) { return; }

    const FTransform CompWorld = Comp->GetComponentTransform();
    const bool bOwnerSelected = Comp->GetOwner() && Comp->GetOwner()->IsSelected();

    for(int32 i=0;i<Sockets.Num();++i)
    {
        const FPPHkNavBakedNavMeshUserEdgeSocket& S = Sockets[i];

        // 位置+ Forward (ローカル->World)
        const FVector StartLocW = CompWorld.TransformPosition(S.CenterStart);
        const FVector EndLocW   = CompWorld.TransformPosition(S.CenterEnd);
        const FVector StartFwdW = CompWorld.TransformVectorNoScale(S.ForwardStart).GetSafeNormal();
        const FVector EndFwdW   = CompWorld.TransformVectorNoScale(S.ForwardEnd).GetSafeNormal();

        // Upベクトルはワールド指定
        const FVector StartUpW = (S.UpVectorStart).GetSafeNormal();
        const FVector EndUpW   = (S.UpVectorEnd).GetSafeNormal();

        // 幅 (左右方向 = Up x Forward)
        const float HalfWidthStart = S.WidthStart * 0.5f;
        const float HalfWidthEnd   = S.WidthEnd   * 0.5f;
        const FVector StartRightDir = FVector::CrossProduct(StartUpW, StartFwdW).GetSafeNormal();
        const FVector EndRightDir   = FVector::CrossProduct(EndUpW,   EndFwdW).GetSafeNormal();
        const FVector StartRight = StartRightDir * HalfWidthStart;
        const FVector EndRight   = EndRightDir   * HalfWidthEnd;

        // 左右端点
        const FVector ALeft  = StartLocW - StartRight;
        const FVector ARight = StartLocW + StartRight;
        const FVector BLeft  = EndLocW   - EndRight;
        const FVector BRight = EndLocW   + EndRight;

        // 垂直許容 (Havok 実装に倣い上下にオフセットするライン群)
        const float VerticalTolerance = S.VerticalToleranceCm; // [cm]
        const FVector StartVerticalToleranceVector = StartUpW * VerticalTolerance;
        const FVector EndVerticalToleranceVector   = EndUpW   * VerticalTolerance;

        // 平均トレランスベクトル (矢印の“幅”として利用)
        const FVector AverageVerticalToleranceVector = (StartVerticalToleranceVector + EndVerticalToleranceVector)
            .GetSafeNormal(SMALL_NUMBER, FVector::ZAxisVector) * VerticalTolerance;

        // 選択判定 (スターのみ色変え / 矢印は常に白)
        constexpr bool bSelectedStart = false;
        constexpr bool bSelectedEnd   = false;

        // カラー (Havok に近づけた色構成)
        const FLinearColor StartColor = bSelectedStart ? FLinearColor::Yellow : FLinearColor::Green;
        const FLinearColor EndColor   = bSelectedEnd   ? FLinearColor::Yellow : FLinearColor::Red;
        const FLinearColor WidthLineStartColor = FLinearColor::Green;
        const FLinearColor WidthLineEndColor   = FLinearColor::Red;
        const FLinearColor VerticalLineColor   = FLinearColor::Gray;
        const FColor ArrowColor = bOwnerSelected ? FColor::White : FColor(230,230,230);

        // --- 幅ライン + トレランス矩形 (Start) ---
        PDI->DrawLine(ALeft  - StartVerticalToleranceVector, ARight - StartVerticalToleranceVector, WidthLineStartColor, SDPG_Foreground, 1.5f);
        PDI->DrawLine(ALeft  + StartVerticalToleranceVector, ARight + StartVerticalToleranceVector, WidthLineStartColor, SDPG_Foreground, 1.5f);
        PDI->DrawLine(ALeft  - StartVerticalToleranceVector, ALeft  + StartVerticalToleranceVector, VerticalLineColor,    SDPG_Foreground, 1.0f);
        PDI->DrawLine(ARight - StartVerticalToleranceVector, ARight + StartVerticalToleranceVector, VerticalLineColor,    SDPG_Foreground, 1.0f);

        // --- 幅ライン + トレランス矩形 (End) ---
        PDI->DrawLine(BLeft  - EndVerticalToleranceVector, BRight - EndVerticalToleranceVector, WidthLineEndColor, SDPG_Foreground, 1.5f);
        PDI->DrawLine(BLeft  + EndVerticalToleranceVector, BRight + EndVerticalToleranceVector, WidthLineEndColor, SDPG_Foreground, 1.5f);
        PDI->DrawLine(BLeft  - EndVerticalToleranceVector, BLeft  + EndVerticalToleranceVector, VerticalLineColor, SDPG_Foreground, 1.0f);
        PDI->DrawLine(BRight - EndVerticalToleranceVector, BRight + EndVerticalToleranceVector, VerticalLineColor, SDPG_Foreground, 1.0f);

        // --- 矢印 (左右それぞれ) ---
        if(S.bBidirectional)
        {
            PPHkNavRuntimeGenEditor::Private::DrawBidirectionalWireArrow(PDI, ALeft,  BLeft,  AverageVerticalToleranceVector, ArrowColor, SDPG_Foreground);
            PPHkNavRuntimeGenEditor::Private::DrawBidirectionalWireArrow(PDI, ARight, BRight, AverageVerticalToleranceVector, ArrowColor, SDPG_Foreground);
        }
        else
        {
            PPHkNavRuntimeGenEditor::Private::DrawWireArrow(PDI, ALeft,  BLeft  - ALeft,  AverageVerticalToleranceVector, ArrowColor, SDPG_Foreground);
            PPHkNavRuntimeGenEditor::Private::DrawWireArrow(PDI, ARight, BRight - ARight, AverageVerticalToleranceVector, ArrowColor, SDPG_Foreground);
        }

        // 中心スター
        {
            constexpr float Size = 12.0f;
            DrawWireStar(PDI, StartLocW, Size, StartColor, SDPG_Foreground);
            DrawWireStar(PDI, EndLocW, Size, EndColor, SDPG_Foreground);
        }
    }
}

#undef LOCTEXT_NAMESPACE
