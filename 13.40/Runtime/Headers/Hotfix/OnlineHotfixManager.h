#pragma once
#include "pch.h"

namespace OnlineHotfixManager
{
	DefineHook(bool, StartsWithDefaultPrefix, const FString& _this, const FString& DefaultPrefix, int32 SearchCase);

	void Init();
}
