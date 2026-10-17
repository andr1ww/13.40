#include "pch.h"
#include "Core.h"
#include "NavigationSystem/NavigationSystem.h"

DEFINE_INIT(NavigationSystem)

UNavigationSystemBase* NavigationSystem::CreateAndConfigureNavigationSystem(UAthenaNavSystemConfig* _this, UWorld* World)
{
	_this->bCreateOnClient = true;
	_this->bPrioritizeNavigationAroundSpawners = true;
	_this->bAutoSpawnMissingNavData = true;
	_this->bAllowAutoRebuild = true;
	_this->bUsesStreamedInNavLevel = false;
	_this->bSupportRuntimeNavmeshDisabling = false;

	UNavigationSystemBase* NavSystemBase = CreateAndConfigureNavigationSystemOG(_this, World);
	UNavigationSystemV1* NavSystem = Cast<UNavigationSystemV1>(NavSystemBase);

	if (!NavSystem)
		return NavSystemBase;

	for (int32 i = 0; i < NavSystem->SupportedAgents.Num(); i++)
	{
		UClass* NavDataClass = NavSystem->SupportedAgents[i].NavDataClass.Get();

		ANavigationData* NavData = NavDataClass ? Cast<ANavigationData>(NavDataClass->ClassDefaultObject) : nullptr;

		if (!NavData)
			continue;

		NavData->RuntimeGeneration = ERuntimeGenerationType::Dynamic;

		if (ARecastNavMesh* NavMesh = Cast<ARecastNavMesh>(NavData))
		{
			NavMesh->CellSize = 24.f;
			NavMesh->CellHeight = 32.f;
			NavMesh->TileSizeUU = 1008.f;
		}
	}

	return NavSystem;
}

void NavigationSystem::OnWorldInitDone(UAthenaNavSystem* _this, EFNavigationSystemRunMode Mode)
{
	OnWorldInitDoneOG(_this, Mode);

	_this->RemoveNavigationLocks();

	for (ANavigationData* NavData : _this->NavDataSet)
	{
		if (!NavData)
			continue;

		NavData->RebuildAll();
		NavData->EnsureBuildCompletion();
	}
}

void NavigationSystem::Init()
{
	Hooking::Hook(ImageBase + 0x50681D0, CreateAndConfigureNavigationSystem, CreateAndConfigureNavigationSystemOG);
	Hooking::Hook(ImageBase + 0x1C65C00, OnWorldInitDone, OnWorldInitDoneOG);
}
