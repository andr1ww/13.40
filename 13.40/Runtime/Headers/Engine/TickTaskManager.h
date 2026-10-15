#pragma once
#include "pch.h"

namespace TickTaskManager
{
	DefineHook(void, RegisterTickFunction, FTickFunction* _this, ULevel* Level);

	void Init();
}
