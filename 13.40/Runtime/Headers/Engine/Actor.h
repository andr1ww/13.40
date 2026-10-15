#pragma once
#include "pch.h"

namespace Actor
{
	DefineHook(void, ReceiveTick, AActor* _this, float DeltaSeconds);

	void Init();
}
