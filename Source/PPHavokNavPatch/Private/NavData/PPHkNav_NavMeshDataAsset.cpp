// Copyright Pocketpair, Inc. All Rights Reserved.


#include "PPHkNav_NavMeshDataAsset.h"

#include "HavokNavNavMesh.h"
#include "PPHkNavNavMeshUserEdgeSocket.h"

bool UPPHkNav_NavMeshDataAsset::IsValid() const
{
	// Emptyデータを差してはだめ
	return PreBakedNavMesh && !PreBakedNavMesh->IsHkNavMeshEmpty();
}

void UPPHkNav_NavMeshDataAsset::SetNewNavMesh(UHavokNavNavMesh* NewNavMesh, TConstArrayView<FPPHkNavBakedNavMeshUserEdgeSocket> NewSockets)
{
#if WITH_EDITOR
	Modify();
#endif
	PreBakedNavMesh = NewNavMesh;
	BakedEdgeSockets.Reset();
	BakedEdgeSockets.Append(NewSockets.GetData(), NewSockets.Num());
}
