#pragma once
#include "pch.h"

namespace CheatManager
{
	DefineHook(void, SpawnServerStatReplicator, UCheatManager* _this);

	void Init();
}
