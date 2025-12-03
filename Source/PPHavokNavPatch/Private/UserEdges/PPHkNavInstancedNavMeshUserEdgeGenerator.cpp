// Copyright Pocketpair, Inc. All Rights Reserved.

#include "PPHkNavInstancedNavMeshUserEdgeGenerator.h"

#include "PPHkNavInstancedNavMeshComponent.h"
#include "HavokNavWorld.h"
#include "HavokNavNavMeshInstance.h"
#include "HavokNavNavMesh.h"
#include "TraversalAnalysis/HavokNavNavMeshJumpTraversal.h"
#include "NavData/PPHkNav_NavMeshDataAsset.h"
#include "HavokNavQueries.h"
#include "HavokNavUtilities.h"
#include "IPPHkNavMeshInstanceRoleInterface.h"

namespace PPHkNav::Private
{
	static FORCEINLINE FVector SafeNormalOr(const FVector& V, const FVector& Fallback)
	{
		return V.GetSafeNormal(UE_SMALL_NUMBER, Fallback);
	}

	// UserEdgeはデフォルトでY-forward
	static FORCEINLINE FVector ComputeRightVector(const FVector& Forward, const FVector& Up)
	{
		const FVector F = SafeNormalOr(Forward, FVector::YAxisVector);
		const FVector U = SafeNormalOr(Up, FVector::ZAxisVector);
		return FVector::CrossProduct(F, U).GetSafeNormal(UE_SMALL_NUMBER, FVector::XAxisVector);
	}

	static bool MakeEdgeDescFromSocket(
		const FTransform& InstanceWorld,
		const FPPHkNavBakedNavMeshUserEdgeSocket& Socket,
		FHavokNavNavMeshUserEdgePairDescription& OutDesc)
	{
		// ワールド化
		const FVector StartCenterWorld = InstanceWorld.TransformPosition(Socket.CenterStart);
		const FVector EndCenterWorld   = InstanceWorld.TransformPosition(Socket.CenterEnd);

		// 方向ベクトルは回転のみ適用（スケールは無視）
		const FVector StartForwardW = SafeNormalOr(InstanceWorld.TransformVectorNoScale(Socket.ForwardStart), FVector::YAxisVector);
		const FVector EndForwardW   = SafeNormalOr(InstanceWorld.TransformVectorNoScale(Socket.ForwardEnd), FVector::YAxisVector);
		const FVector StartUpW      = SafeNormalOr((Socket.UpVectorStart), FVector::ZAxisVector);
		const FVector EndUpW        = SafeNormalOr((Socket.UpVectorEnd), FVector::ZAxisVector);

		const FVector StartRightW = ComputeRightVector(StartForwardW, StartUpW);
		const FVector EndRightW   = ComputeRightVector(EndForwardW, EndUpW);

		const double HalfStart = FMath::Max(5.0, static_cast<double>(Socket.WidthStart) * 0.5);
		const double HalfEnd   = FMath::Max(5.0, static_cast<double>(Socket.WidthEnd) * 0.5);

		FHavokNavAny EdgeData;
		TSubclassOf<UHavokNavNavMeshUserEdge> EdgeClass = Socket.UserEdgeClass;
		if (!EdgeClass)
		{
			// 未設定の場合はフォールバック
			EdgeClass = UHavokNavNavMeshUserEdge::StaticClass();
		}
		if (EdgeClass)
		{
			if (Socket.UserEdgeData.IsValid())
			{
				const UScriptStruct* EdgeDataType = Socket.UserEdgeData.GetScriptStruct();
				check(EdgeClass.GetDefaultObject()->DataType == EdgeDataType);
				EdgeData.SetElementType(EdgeDataType);
				EdgeData.SetElementValue(FHavokNavAnyConstRef(EdgeDataType, Socket.UserEdgeData.GetMemory()));
			}
			else
			{
				// DataType can be null
				EdgeData.SetElementType(EdgeClass.GetDefaultObject()->DataType);
			}
		}

		FHavokNavNavMeshUserEdgePairDescription Desc;
		Desc.Points.ALeft  = StartCenterWorld - StartRightW * HalfStart;
		Desc.Points.ARight = StartCenterWorld + StartRightW * HalfStart;
		Desc.Points.BLeft  = EndCenterWorld   - EndRightW   * HalfEnd;
		Desc.Points.BRight = EndCenterWorld   + EndRightW   * HalfEnd;
		Desc.CostAToB = Socket.CostAtoB;
		Desc.CostBToA = Socket.CostBtoA;
		Desc.bTraversableAToB = true;
		Desc.bTraversableBToA = Socket.bBidirectional;
		Desc.UpVectorA = StartUpW;
		Desc.UpVectorB = EndUpW;
		Desc.VerticalTolerance = Socket.VerticalToleranceCm;
		Desc.Information.UserEdgeClass = EdgeClass;
		Desc.Information.Data = EdgeData.GetRef();

		OutDesc = MoveTemp(Desc);
		return true;
	}

	static bool CheckNavMeshPoint(UHavokNavWorld* NavWorld, const TSubclassOf<UHavokNavNavMeshLayer> Layer, const FVector& Point, const float QueryRadius)
	{
		const HavokNavQueries::FGetClosestPointOnNavMeshParameters P
		{
			.LayerClass = Layer,
			.FaceFilter = {},
			.UpVectorFilter = {},
			.Point = Point,
			.QueryRadius = (QueryRadius > 0.0f) ? QueryRadius : 150.0f,
		};
		const FHavokNavGetClosestPointOnNavMeshResult R = HavokNavQueries::GetClosestPointOnNavMesh(NavWorld, P);
		return NavWorld->IsNavMeshFaceKeyValid(R.FaceKey);
	}

	// LevelNavMeshに対して到達可能かをチェック
	static bool CheckNavMeshPointForLevelNavMesh(UHavokNavWorld* NavWorld, const TSubclassOf<UHavokNavNavMeshLayer> Layer, const FVector& Point, const float QueryRadius)
	{
		const HavokNavQueries::FGetClosestPointOnNavMeshParameters P
		{
			.LayerClass = Layer,
			.FaceFilter = {},
			.UpVectorFilter = {},
			.Point = Point,
			.QueryRadius = (QueryRadius > 0.0f) ? QueryRadius : 150.0f,
		};
		const FHavokNavGetClosestPointOnNavMeshResult R = HavokNavQueries::GetClosestPointOnNavMesh(NavWorld, P);
		if (!NavWorld->IsNavMeshFaceKeyValid(R.FaceKey))
		{
			return false;
		}
		
		UHavokNavNavMeshInstance const* Instance = FHavokNavUtilities::GetInstanceForFaceKey(NavWorld, R.FaceKey);
		if (auto* RoleProvider = Cast<IPPHkNavMeshInstanceRoleInterface>(Instance))
		{
			return RoleProvider->IsLevelNavMeshInstance();
		}
		// interfaceを 実装していないインスタンスはHavok側で自動生成されたやつなのでLevelNavMesh判定とする
		return Instance != nullptr;
	}
}// namespace PPHkNav::Private

UPPHkNavInstancedNavMeshUserEdgeGenerator::UPPHkNavInstancedNavMeshUserEdgeGenerator()
{
}

UPPHkNavInstancedNavMeshUserEdgeGenerator::FEdgeSetId UPPHkNavInstancedNavMeshUserEdgeGenerator::GenerateEdgesForInstanceSocket(UHavokNavWorld* NavWorld, UHavokNavNavMeshInstance* LoadedInstance,	const TConstArrayView<FPPHkNavBakedNavMeshUserEdgeSocket> Sockets)
{
	FEdgeSetId SetId = GenerateEdgesForInstanceSocketLogic(NavWorld, LoadedInstance, Sockets);
	if (SetId)
	{
		// 生成成功
		GeneratedEdgeSetIds.Emplace(SetId);
	}
	return SetId;
}

void UPPHkNavInstancedNavMeshUserEdgeGenerator::RemoveEdges(UHavokNavWorld* NavWorld, const FEdgeSetId EdgeSetId)
{
	if (!EdgeSetId) { return; }
	if (!IsValid(NavWorld)) { return; }

	NavWorld->RemoveNavMeshDynamicUserEdgeSet(EdgeSetId);
	GeneratedEdgeSetIds.Remove(EdgeSetId);
}

UPPHkNavInstancedNavMeshUserEdgeGenerator::FEdgeSetId UPPHkNavInstancedNavMeshUserEdgeGenerator::
GenerateEdgesForInstanceSocketLogic(UHavokNavWorld* NavWorld, UHavokNavNavMeshInstance* LoadedInstance,
	const TConstArrayView<FPPHkNavBakedNavMeshUserEdgeSocket> Sockets)
{
	if (!IsValid(NavWorld)) { return {}; }
	if (Sockets.IsEmpty()) { return {}; }
	if (!IsValid(LoadedInstance)) { return {}; }
	if (!LoadedInstance->IsLoaded())
	{
		// Havok上はユーザーエッジの遅延生成に対応しているので未ロードでもユーザーエッジ作ってよい
		// ただし未ロードだとfaceが取れない、カスタムデータが取れないため生成してよいかの判定が行えない
		ensure(false && TEXT("未ロードのNavMeshInstanceに対してエッジ生成を行うと事前判定できないため対応しません"));
		return {};
	}

	const FHavokNavNavMeshDynamicUserEdgeSetIdentifier SetId = NavWorld->CreateNavMeshDynamicUserEdgeSet(LoadedInstance->GetLayer());
	if (!SetId)
	{
		UE_LOG(LogTemp, Warning, TEXT("EdgeGenerator: Failed to create edge set for %s"), *LoadedInstance->GetName());
		return {};
	}

	int32 CreatedTotal = 0;

	// ベイク済みソケットからの生成を試みる
	for (const FPPHkNavBakedNavMeshUserEdgeSocket& Socket : Sockets)
	{
		FHavokNavNavMeshUserEdgePairDescription Desc;
		if (!PPHkNav::Private::MakeEdgeDescFromSocket(LoadedInstance->GetTransform(), Socket, Desc))
		{
			continue;
		}

		// ソケット始端/終端
		const FVector StartCenterW = 0.5f * (Desc.Points.ALeft + Desc.Points.ARight);
		const FVector EndCenterW   = 0.5f * (Desc.Points.BLeft + Desc.Points.BRight);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		// 始端のチェック
		// ロード済みなら始端はNavMeshに投影できるはず. BP設定を見直してください
		const bool bValidStart = PPHkNav::Private::CheckNavMeshPoint(NavWorld, LoadedInstance->GetLayer(), StartCenterW, Socket.MaxProjectionDistanceCm);
		ensure(bValidStart); 
#else
		constexpr bool bValidStart = true;
#endif // UE_BUILD_DEBUG

		// 終端のチェック
		const bool bValidEnd = PPHkNav::Private::CheckNavMeshPointForLevelNavMesh(NavWorld, LoadedInstance->GetLayer(), EndCenterW, Socket.MaxProjectionDistanceCm);

		// 両端がNavMeshに投影できる
		if (bValidStart && bValidEnd)
		{
			NavWorld->AddNavMeshDynamicUserEdgePair(SetId, Desc);
			++CreatedTotal;
		}
		else
		{
			UE_LOG(LogTemp, Verbose, TEXT("EdgeGenerator: Socket skipped for %s (projection invalid or same instance)."), *LoadedInstance->GetName());
		}
	}

	if (CreatedTotal == 0)
	{
		// 生成失敗
		NavWorld->RemoveNavMeshDynamicUserEdgeSet(SetId);
		UE_LOG(LogTemp, Verbose, TEXT("EdgeGenerator: No edges created for %s"), *LoadedInstance->GetName());
		return {};
	}

	UE_LOG(LogTemp, Log, TEXT("EdgeGenerator: Created %d dynamic edges for %s"), CreatedTotal, *LoadedInstance->GetName());
	return SetId;
}

void UPPHkNavInstancedNavMeshUserEdgeGenerator::FinishDestroy()
{
	// UserEdgeが leakしている恐れがあります. RemoveEdgesを実行してください
	ensure(GeneratedEdgeSetIds.IsEmpty());
	
	UObject::FinishDestroy();
}
