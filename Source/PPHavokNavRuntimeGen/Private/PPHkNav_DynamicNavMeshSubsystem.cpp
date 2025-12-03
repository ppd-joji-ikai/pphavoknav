// Fill out your copyright notice in the Description page of Project Settings.


#include "PPHkNav_DynamicNavMeshSubsystem.h"

#include "PPHkNav_DynamicNavMeshManager.h"
#include "PPHkNav_RuntimeGenerationSubsystem.h"
#include "Engine/World.h"

void UPPHkNav_DynamicNavMeshSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UPPHkNav_RuntimeGenerationSubsystem* RuntimeGenSubsystem = Collection.InitializeDependency<UPPHkNav_RuntimeGenerationSubsystem>();
	UPPHkNav_DynamicNavMeshManager* Manager = NewObject<UPPHkNav_DynamicNavMeshManager>(this);
	Manager->Initialize(GetWorld(), RuntimeGenSubsystem);
	GeneratorInterface = Manager;
}

void UPPHkNav_DynamicNavMeshSubsystem::Deinitialize()
{
	if (GeneratorInterface && GeneratorInterface.GetObject())
	{
		Cast<UPPHkNav_DynamicNavMeshManager>(GeneratorInterface.GetObject())->Deinitialize();
	}
	GeneratorInterface = nullptr;
	
	Super::Deinitialize();
}
