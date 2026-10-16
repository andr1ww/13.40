#include "pch.h"
#include "Core.h"
#include "NavigationSystem/NavigationSystem.h"

DEFINE_INIT(NavigationSystem)

UNavigationSystemBase* NavigationSystem::CreateAndConfigureNavigationSystem(UNavigationSystemModuleConfig* _this, UWorld* World)
{
	_this->bAutoSpawnMissingNavData = true;

	UNavigationSystemBase* NavSystemBase = CreateAndConfigureNavigationSystemOG(_this, World);
	UNavigationSystemV1* NavSystem = Cast<UNavigationSystemV1>(NavSystemBase);

	if (!NavSystem)
		return NavSystemBase;

	for (int32 i = 0; i < NavSystem->SupportedAgents.Num(); i++)
	{
		UClass* NavDataClass = NavSystem->SupportedAgents[i].NavDataClass.Get();

		if (ANavigationData* NavData = NavDataClass ? Cast<ANavigationData>(NavDataClass->ClassDefaultObject) : nullptr)
			NavData->RuntimeGeneration = ERuntimeGenerationType::Dynamic;
	}

	NavSystem->ReleaseInitialBuildingLock();

	return NavSystem;
}

void NavigationSystem::Init()
{
	Hooking::Hook(ImageBase + 0x50681D0, CreateAndConfigureNavigationSystem, CreateAndConfigureNavigationSystemOG);
}
