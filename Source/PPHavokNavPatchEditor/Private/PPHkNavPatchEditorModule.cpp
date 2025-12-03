// Copyright Pocketpair, Inc. All Rights Reserved.
#include "Modules/ModuleManager.h"
#include "ComponentVisualizer.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "PPHkNavInstancedNavMeshComponentVisualizer.h"
#include "Components/PPHkNavInstancedNavMeshComponent.h"

class FPPHkNavPatchEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		if (GUnrealEd)
		{
			Visualizer = MakeShareable(new FPPHkNavInstancedNavMeshComponentVisualizer());
			GUnrealEd->RegisterComponentVisualizer(UPPHkNavInstancedNavMeshComponent::StaticClass()->GetFName(), Visualizer);
			Visualizer->OnRegister();
		}
#endif
	}
	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		if (GUnrealEd && Visualizer.IsValid())
		{
			GUnrealEd->UnregisterComponentVisualizer(UPPHkNavInstancedNavMeshComponent::StaticClass()->GetFName());
			Visualizer.Reset();
		}
#endif
	}
private:
	TSharedPtr<FComponentVisualizer> Visualizer;
};

IMPLEMENT_MODULE(FPPHkNavPatchEditorModule, PPHavokNavPatchEditor)

