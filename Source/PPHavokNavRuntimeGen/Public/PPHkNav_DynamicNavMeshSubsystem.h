// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IPPHkNavDynamicNavMeshGenerator.h"
#include "Subsystems/WorldSubsystem.h"
#include "PPHkNav_DynamicNavMeshSubsystem.generated.h"

struct FHavokNavNavMeshGenerationControllerOverride;
class UHavokNavNavMeshLayer;
/**
 * 動的NavMesh生成を担うサブシステム
 */
UCLASS()
class PPHKNAVRUNTIMEGEN_API UPPHkNav_DynamicNavMeshSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 生成機能本体への interface
	IPPHkNavDynamicNavMeshGenerator* GetGeneratorInterface() const { return GeneratorInterface.GetInterface(); }

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	/** NavMesh管理インターフェースを持つオブジェクトへの参照 */
	UPROPERTY(Transient)
	TScriptInterface<IPPHkNavDynamicNavMeshGenerator> GeneratorInterface;
};
