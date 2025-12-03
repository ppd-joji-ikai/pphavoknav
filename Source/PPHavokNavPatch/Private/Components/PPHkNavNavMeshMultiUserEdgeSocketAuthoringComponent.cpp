// Copyright Pocketpair, Inc. All Rights Reserved.
#include "Components/PPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent.h"

#if WITH_EDITOR
#include "DrawDebugHelpers.h"
#endif

UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly::UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    bIsEditorOnly = true;
}

#if WITH_EDITOR
bool UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly::IsEditorOnly() const
{
    return true;
}

void UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly::OnComponentCreated()
{
    Super::OnComponentCreated();
    ValidateAll();
}

void UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly::OnRegister()
{
    Super::OnRegister();
    ValidateAll();
}

void UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    ValidateAll();
}

int32 UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly::AddSocket(FName OptionalName)
{
    FPPHkNavBakedNavMeshUserEdgeSocket NewSocket; // 既定値
    if(OptionalName != NAME_None)
    {
        NewSocket.SocketName = OptionalName;
    }
    SocketDefinitions.Add(NewSocket);
    const int32 NewIndex = SocketDefinitions.Num() - 1;
    ValidateSocket(SocketDefinitions[NewIndex]);
    MarkPackageDirty();
    return NewIndex;
}

bool UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly::RemoveSocket(int32 Index)
{
    if(!SocketDefinitions.IsValidIndex(Index)) { return false; }
    SocketDefinitions.RemoveAt(Index);
    MarkPackageDirty();
    return true;
}

int32 UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly::DuplicateSocket(int32 Index)
{
    if(!SocketDefinitions.IsValidIndex(Index)) { return INDEX_NONE; }
    const FPPHkNavBakedNavMeshUserEdgeSocket Copy = SocketDefinitions[Index];
    SocketDefinitions.Add(Copy);
    const int32 NewIndex = SocketDefinitions.Num() - 1;
    // 名前に連番を付与
    if(Copy.SocketName != NAME_None)
    {
        const FString Base = Copy.SocketName.ToString();
        SocketDefinitions[NewIndex].SocketName = FName(FString::Format(TEXT("{0}_Copy{1}"), {Base, NewIndex}));
    }
    ValidateSocket(SocketDefinitions[NewIndex]);
    MarkPackageDirty();
    return NewIndex;
}

void UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly::ValidateAll()
{
    for(FPPHkNavBakedNavMeshUserEdgeSocket& S : SocketDefinitions)
    {
        ValidateSocket(S);
    }
}

void UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly::ValidateSocket(FPPHkNavBakedNavMeshUserEdgeSocket& InOutSocket) const
{
    static constexpr float MinWidth = 5.f;
    InOutSocket.WidthStart = FMath::Max(InOutSocket.WidthStart, MinWidth);
    InOutSocket.WidthEnd   = FMath::Max(InOutSocket.WidthEnd,   MinWidth);

    if(!InOutSocket.UpVectorStart.IsNearlyZero())
    {
        InOutSocket.UpVectorStart = InOutSocket.UpVectorStart.GetSafeNormal();
    }
    else
    {
        InOutSocket.UpVectorStart = FVector::ZAxisVector;
    }
    if(!InOutSocket.UpVectorEnd.IsNearlyZero())
    {
        InOutSocket.UpVectorEnd = InOutSocket.UpVectorEnd.GetSafeNormal();
    }
    else
    {
        InOutSocket.UpVectorEnd = FVector::ZAxisVector;
    }

    // Forward ベクトル正規化と既定
    if(!InOutSocket.ForwardStart.IsNearlyZero())
    {
        InOutSocket.ForwardStart = InOutSocket.ForwardStart.GetSafeNormal();
    }
    else
    {
        InOutSocket.ForwardStart = FVector::ForwardVector;
    }
    if(!InOutSocket.ForwardEnd.IsNearlyZero())
    {
        InOutSocket.ForwardEnd = InOutSocket.ForwardEnd.GetSafeNormal();
    }
    else
    {
        InOutSocket.ForwardEnd = FVector::ForwardVector;
    }
}

bool UPPHkNavNavMeshMultiUserEdgeSocketAuthoringComponent_EditorOnly::SetSocketTransform(int32 Index, bool bEnd, const FTransform& NewLocalTransform)
{
    if(!SocketDefinitions.IsValidIndex(Index)) { return false; }
    FPPHkNavBakedNavMeshUserEdgeSocket& S = SocketDefinitions[Index];

    // Clean rotation/translation, ignore scale
    const FVector NewLoc = NewLocalTransform.GetLocation();
    const FQuat   NewRot = NewLocalTransform.GetRotation();

    // Forward/Up は回転から再構築（X=Forward, Z=Up）
    const FVector NewForward = NewRot.GetAxisX().GetSafeNormal();
    const FVector NewUp      = NewRot.GetAxisZ().GetSafeNormal();

    if(bEnd)
    {
        S.CenterEnd   = NewLoc;
        S.ForwardEnd  = NewForward;
        S.UpVectorEnd = NewUp;
    }
    else
    {
        S.CenterStart   = NewLoc;
        S.ForwardStart  = NewForward;
        S.UpVectorStart = NewUp;
    }
    // 正規化等
    ValidateSocket(S);
    MarkPackageDirty();
    return true;
}
#endif // WITH_EDITOR
