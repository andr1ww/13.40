#pragma once
#include "pch.h"

namespace Settings
{
	DefineHook(void, GetGameDefaultMap, FString* ret, char a2);

	void Init();
}