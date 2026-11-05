#pragma once
#include "pch.h"

namespace FortAIGoalManager
{
	AFortAIGoalManager* GetCurrent(UObject* WorldContextObject);

	void Init();
}
