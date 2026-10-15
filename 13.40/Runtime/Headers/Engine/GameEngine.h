#pragma once
#include "pch.h"

namespace GameEngine
{
	DefineHook(void, Start, UGameEngine* _this);
	DefineHook(void, Tick, UGameEngine* _this, float DeltaSeconds, bool bIdleMode);

	void Init();
}