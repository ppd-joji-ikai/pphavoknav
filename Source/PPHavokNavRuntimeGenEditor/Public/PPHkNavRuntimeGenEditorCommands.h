// Copyright Pocketpair, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

/**
 * 
 */
class PPHKNAVRUNTIMEGENEDITOR_API FPPHkNavRuntimeGenEditorCommands : public TCommands<FPPHkNavRuntimeGenEditorCommands>
{
public:
	FPPHkNavRuntimeGenEditorCommands();

	// TCommand<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> BakeNavMesh;
	
};
