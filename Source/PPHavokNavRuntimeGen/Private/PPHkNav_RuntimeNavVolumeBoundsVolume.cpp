// Copyright Pocketpair, Inc. All Rights Reserved.

#include "PPHkNav_RuntimeNavVolumeBoundsVolume.h"

#include "HavokNavNavVolume.h"
#include "HavokNavNavVolumeInstanceSetComponent.h"
#include "IPPHkNavDynamicNavMeshGenerator.h"
#include "PPHkNav_DynamicNavMeshSubsystem.h"
#include "Components/BrushComponent.h"
#include "Engine/World.h"

APPHkNav_RuntimeNavVolumeBoundsVolume::APPHkNav_RuntimeNavVolumeBoundsVolume()
    :Super()
{
    // ボリュームの初期設定
    GetBrushComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetBrushComponent()->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
    GetBrushComponent()->SetGenerateOverlapEvents(false);
    
    // 半透明の色で表示（NavVolumeは青系統で区別）
    BrushColor = FColor(0, 128, 255, 255);
    bColored = true; // 描画設定
    Super::SetActorHiddenInGame(false); // ボリュームを半透明で表示

    // NavVolumeインスタンスを保持・表示するためのコンポーネントを作成
    InstanceSetComponent = CreateDefaultSubobject<UHavokNavNavVolumeInstanceSetComponent>(TEXT("InstanceSetComponent"));
    InstanceSetComponent->SetupAttachment(GetBrushComponent());

    // アクターとしての設定
    PrimaryActorTick.bCanEverTick = false;
}

void APPHkNav_RuntimeNavVolumeBoundsVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 生成されたNavVolumeをクリア
    if (InstanceSetComponent)
    {
        InstanceSetComponent->Reset();
    }
    Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void APPHkNav_RuntimeNavVolumeBoundsVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    
    // ボリュームの形状が変更された場合、自動的にNavVolumeを再生成しない
    // ユーザーが明示的にRequestGeneration()を呼び出す必要がある
}
#endif

void APPHkNav_RuntimeNavVolumeBoundsVolume::RequestGeneration()
{
    const UWorld* CurrentWorld = GetWorld();
    if (!CurrentWorld)
    {
        return;
    }

    const UPPHkNav_DynamicNavMeshSubsystem* NavSubsystem = CurrentWorld->GetSubsystem<UPPHkNav_DynamicNavMeshSubsystem>();
    if (!NavSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPPHkNav_DynamicNavMeshSubsystem not found."));
        return;
    }

    UBrushComponent* BrushComp = GetBrushComponent();
    if (!BrushComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("BrushComponent is not valid."));
        return;
    }

    // アクターのTransformを取得
    const FTransform ActorTransform = GetActorTransform();

    // ボリュームのローカル空間でのバウンディングボックスを取得
    const FBox LocalBounds = BrushComp->Bounds.GetBox();

    // NavVolume生成のためのパラメータを設定
    FPPHkNavBoxNavVolumeGenerationParams Params{};
    Params.GenerationBox = LocalBounds;
    Params.Layer = Layer;
    Params.ControllerOverride = ControllerOverride;
    Params.GenerationTransform = ActorTransform; // ボリュームのTransformを使用
    Params.NavVolumeBoundsActor = TWeakInterfacePtr(static_cast<IPPHkNavVolumeBoundsActor*>(this));
    Params.OnGeneratedCallback = [WeakThis = TWeakObjectPtr<APPHkNav_RuntimeNavVolumeBoundsVolume>(this)](const FHavokNavNavVolumeGeneratorResult& Result)
    {
        if (auto* This = WeakThis.Get())
        {
            This->OnNavVolumeGenerationCompleted(Result);
        }
    };

    // 既存のNavVolumeを削除します。
    InstanceSetComponent->Reset();

    // NavVolume生成を要求
    NavSubsystem->GetGeneratorInterface()->RequestBoxNavVolumeGeneration(MoveTemp(Params));
}

void APPHkNav_RuntimeNavVolumeBoundsVolume::OnNavVolumeGenerationCompleted(const FHavokNavNavVolumeGeneratorResult& Result) const
{
    if (Result.NavVolume)
    {
        Result.NavVolume->Rename(nullptr, InstanceSetComponent);
        InstanceSetComponent->AddNavVolume(Result.NavVolume);
        
        UE_LOG(LogTemp, Verbose, TEXT("NavVolume generated successfully for %s."), *GetName());
    }
}

FBox APPHkNav_RuntimeNavVolumeBoundsVolume::GetNavVolumeBounds() const
{
    // AVolume::GetBounds()を使うべきか?
    return GetBrushComponent() ? GetBrushComponent()->Bounds.GetBox() : FBox(EForceInit::ForceInit);
}

FGuid APPHkNav_RuntimeNavVolumeBoundsVolume::GetDominationGuid() const
{
    return Guid;
}

bool APPHkNav_RuntimeNavVolumeBoundsVolume::IsInNavVolumeLayer(TSubclassOf<UHavokNavNavVolumeLayer> InLayer) const
{
    return Layer != nullptr && InLayer == Layer;
}
