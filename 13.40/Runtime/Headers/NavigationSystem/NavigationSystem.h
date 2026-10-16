#pragma once
#include "pch.h"

namespace NavigationSystem
{
	DefineHook(UNavigationSystemBase*, CreateAndConfigureNavigationSystem, UNavigationSystemModuleConfig* _this, UWorld* World);

	void Init();
}
