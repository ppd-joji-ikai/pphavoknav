// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "IHavokNavNavMeshGenerationBoundsProvider.h"
#include "PPHkNav_ActorBoundsProvider.generated.h"

class AActor;
class UHavokNavNavMeshLayer;

/**
 * @brief 特定のアクターのバウンディングボックスをNavMesh生成範囲として提供するクラス
 *  主に床などの歩ける建築物などを対象にしており、ブロッキングボリューム内であっても生成する.
 *  そのため、TargetActorと他のオブジェクトの重なりも考慮しない
 */
UCLASS()
class PPHAVOKNAVRUNTIMEGEN_API UPPHkNav_ActorBoundsProvider : public UObject, public IHavokNavNavMeshGenerationBoundsProvider
{
    GENERATED_BODY()

public:
    /**
     * @brief 初期化する.
     * @param InActor 生成対象のアクター 
     * @param InLayer 生成対象レイヤー
     * @param InUpVector 生成NavMeshの法線ベクトル
     */
    void Initialize(const TObjectPtr<AActor>& InActor, const TSubclassOf<UHavokNavNavMeshLayer>& InLayer, const FVector& InUpVector)
    {
        TargetActor = InActor;
        TargetLayer = InLayer;
        UpVector = InUpVector.GetSafeNormal();
        if (UpVector.IsZero())
        {
            UE_LOG(LogTemp, Warning,
                   TEXT("UPPHkNav_ActorBoundsProvider: UpVector cannot be zero. Using default UpVector."));
            UpVector = FVector::UpVector;
        }
    }
    
    // IHavokNavNavMeshGenerationBoundsProvider interface
    virtual FHavokNavNavMeshGenerationBounds GetBounds(TSubclassOf<UHavokNavNavMeshLayer> Layer, FTransform const& GenerationTransform) const override;

private:
    // NavMeshを生成する対象アクタ
    UPROPERTY(Transient)
    TObjectPtr<const AActor> TargetActor;

    // NavMeshを生成する対象レイヤー
    UPROPERTY(Transient)
    TSubclassOf<UHavokNavNavMeshLayer> TargetLayer;

    // NavMesh面の法線ベクトル
    UPROPERTY()
    FVector UpVector = FVector::UpVector;
};