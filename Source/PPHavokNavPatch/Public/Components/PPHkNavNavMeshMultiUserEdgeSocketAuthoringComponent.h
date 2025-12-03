// Copyright Pocketpair, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "NavData/PPHkNavNavMeshUserEdgeSocket.h"
#include "PPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent.generated.h"

/**
 * 複数ユーザーエッジソケットを 1 コンポーネントで管理する Editor 専用 Authoring コンポーネント
 * - ランタイムでは参照のみ (IsEditorOnly)
 * - BakeUtility 側は GetSocketDefinitions() で全要素を取得してベイク配列へ転写する想定
 * - 将来的に ComponentVisualizer (ハンドル個別操作) を追加/差し替え予定
 *
 * @note: Actor側で保持する場合は WITH_EDITORONLY_DATA でくくること
 */
UCLASS(ClassGroup=(PPHkNav), meta=(BlueprintSpawnableComponent, DisplayName="PPHkNav NavMesh Multi UserEdge Sockets Authoring Component"))
class PPHAVOKNAVPATCH_API UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly : public USceneComponent
{
    GENERATED_BODY()
public:
    UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly();

#if WITH_EDITOR
    /** 全ソケット取得 (BakeUtility など用) */
    const TArray<FPPHkNavBakedNavMeshUserEdgeSocket>& GetSocketDefinitions() const { return SocketDefinitions; }

    /** 指定 Index の参照 (安全な失敗戻り値無し: 呼び出し前に IsValidIndex 確認) */
    const FPPHkNavBakedNavMeshUserEdgeSocket& GetSocketDefinitionChecked(int32 Index) const { return SocketDefinitions[Index]; }

    /** 新規ソケット追加 (初期 Transform は Start=(0,0,0) End=(0,200,0)) */
    UFUNCTION(CallInEditor, BlueprintCallable, Category="User Edge|Edit")
    int32 AddSocket(FName OptionalName = NAME_None);

    /** ソケット削除 */
    UFUNCTION(CallInEditor, BlueprintCallable, Category="User Edge|Edit")
    bool RemoveSocket(int32 Index);

    /** ソケット複製 */
    UFUNCTION(CallInEditor, BlueprintCallable, Category="User Edge|Edit")
    int32 DuplicateSocket(int32 Index);

    /** 全ソケット正規化/検証 */
    UFUNCTION(CallInEditor, BlueprintCallable, Category="User Edge|Edit")
    void ValidateAll();

    virtual bool IsEditorOnly() const override; // パッケージ時はデータのみ
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void OnComponentCreated() override;
    virtual void OnRegister() override;

    /** Visualizer から局所的に Start/End Transform を設定するためのヘルパ (検証と正規化込み) */
    bool SetSocketTransform(int32 Index, bool bEnd, const FTransform& NewLocalTransform);
#endif
    
private:
#if WITH_EDITOR
    void ValidateSocket(FPPHkNavBakedNavMeshUserEdgeSocket& InOutSocket) const;
#endif

protected:
#if WITH_EDITORONLY_DATA
    /** 管理するソケット配列 */
    UPROPERTY(EditAnywhere, Category="User Edge")
    TArray<FPPHkNavBakedNavMeshUserEdgeSocket> SocketDefinitions{};    
#endif
};
