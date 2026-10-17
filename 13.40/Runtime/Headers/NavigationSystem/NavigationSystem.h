#pragma once
#include "pch.h"

namespace NavigationSystem
{
	DefineHook(UNavigationSystemBase*, CreateAndConfigureNavigationSystem, UAthenaNavSystemConfig* _this, UWorld* World);
	DefineHook(void, OnWorldInitDone, UNavigationSystemV1* _this, EFNavigationSystemRunMode Mode);

	void Init();
}
