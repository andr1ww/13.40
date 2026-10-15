#include "pch.h"
#include "Core.h"
#include "Engine/TickTaskManager.h"

DEFINE_INIT(TickTaskManager)

void TickTaskManager::RegisterTickFunction(FTickFunction* _this, ULevel* Level)
{
	if (!_this->IsTickFunctionRegistered() && !_this->bAllowTickOnDedicatedServer && Level && Level->OwningWorld)
		return;

	RegisterTickFunctionOG(_this, Level);
}

void TickTaskManager::Init()
{
	Hooking::Hook(ImageBase + 0x4DA61C0, RegisterTickFunction, RegisterTickFunctionOG);
}
