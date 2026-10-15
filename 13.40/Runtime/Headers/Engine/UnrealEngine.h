#pragma once
#include "pch.h"

namespace UnrealEngine
{
	DefineHook(float, GetTimeBetweenGarbageCollectionPasses, UEngine* _this);

	void Init();
}
