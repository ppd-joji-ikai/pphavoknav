// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "HavokNavTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "PPHkNavNavMeshUserEdgeSocket.generated.h"

struct FPPHkNavPatch_CustomDataBase;
class UHavokNavNavMeshUserEdge;

/**
 * ベイク済みユーザーエッジソケット
 * Actor相対で保持しランタイムでワールド化してユーザーエッジ生成を試みる
 */
USTRUCT()
struct PPHKNAVPATCH_API FPPHkNavBakedNavMeshUserEdgeSocket
{
	GENERATED_BODY()

public:
	/** デバッグ / 参照用名 */
	UPROPERTY(EditAnywhere, Category="User Edge")
	FName SocketName{NAME_None};

	/** このソケットが実行時に生成すべきユーザーエッジクラス */
	UPROPERTY(EditAnywhere, Category="User Edge")
	TSubclassOf<UHavokNavNavMeshUserEdge> UserEdgeClass{};

	/** ユーザーエッジクラスに対応するエッジカスタムデータ*/
	UPROPERTY(EditAnywhere, Category="User Edge")
	TInstancedStruct<FPPHkNavPatch_CustomDataBase> UserEdgeData{};

	/** 始端の中心位置(Local)*/
	UPROPERTY(EditAnywhere, Category="User Edge")
	FVector CenterStart{FVector::ZeroVector};
	
	/** 終端の中心位置(Local)*/
	UPROPERTY(EditAnywhere, Category = "User Edge", meta=(MakeEditWidget = true))
	FVector CenterEnd{FVector{0, 200, 0}};
	
	/** 始端の幅*/ 
	UPROPERTY(EditAnywhere, Category = "User Edge", meta = (ClampMin = "5.0", UIMin = "5.0"))
	float WidthStart = 100.f;
	
	/** 終端の幅*/
	UPROPERTY(EditAnywhere, Category = "User Edge", meta = (ClampMin = "5.0", UIMin = "5.0"))
	float WidthEnd = 100.f;

	/** 始端のフォワードベクトル(Local)
	 * ユーザーエッジはデフォルトでYフォワード
	 */
	UPROPERTY(EditAnywhere, Category="User Edge")
	FVector ForwardStart{FVector::YAxisVector};

	/** 終端のフォワードベクトル(Local)
	 * ユーザーエッジはデフォルトでYフォワード
	 */
	UPROPERTY(EditAnywhere, Category="User Edge")
	FVector ForwardEnd{FVector::YAxisVector};

	/** 始端のアップベクトル(World). NavMeshのUpベクトルと一致するときのみ接合する*/
	UPROPERTY(EditAnywhere, Category = "User Edge", AdvancedDisplay)
	FVector UpVectorStart = FVector::ZAxisVector;

	/** 終端のアップベクトル(World). NavMeshのUpベクトルと一致するときのみ接合する*/
	UPROPERTY(EditAnywhere, Category = "User Edge", AdvancedDisplay)
	FVector UpVectorEnd = FVector::ZAxisVector;

	/** 双方向か */
	UPROPERTY(EditAnywhere, Category="User Edge")
	bool bBidirectional{true};
	
	/** 経路探索時の横断コスト スタート->エンド*/
	UPROPERTY(EditAnywhere, Category="User Edge")
	float CostAtoB{0.0f};

	/** 経路探索時の横断コスト エンド->スタート*/
	UPROPERTY(EditAnywhere, Category="User Edge")
	float CostBtoA{0.0f};

	/** 垂直差許容[cm] */
	UPROPERTY(EditAnywhere, Category="User Edge", meta=(ClampMin="0.0"))
	float VerticalToleranceCm{100.0f};

	/** レベル NavMesh 探索最大距離[cm] (下方/投影) */
	UPROPERTY(EditAnywhere, Category="User Edge", meta=(ClampMin="0.0"))
	float MaxProjectionDistanceCm{150.0f};
};
