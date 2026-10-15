#pragma once
#include "pch.h"

namespace NetDriver
{
	DefineHook(void, TickFlush, UNetDriver* NetDriver, float DeltaSeconds);

	void Init();
}