#pragma once
#include "pch.h"

namespace BodySetup
{
	DefineHook(void*, GetCookedData, UBodySetup* _this, FName Format, bool bRuntimeOnlyOptimizedVersion);

	void Init();
}
