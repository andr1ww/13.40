#pragma once
#include "pch.h"

namespace NavigationSystem
{
	DefineHook(UNavigationSystemBase*, CreateAndConfigureNavigationSystem, UAthenaNavSystemConfig* _this, UWorld* World);
	DefineHook(void, OnWorldInitDone, UAthenaNavSystem* _this, EFNavigationSystemRunMode Mode);

	void Init();
}
