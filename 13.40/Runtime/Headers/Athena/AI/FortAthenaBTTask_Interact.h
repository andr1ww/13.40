#pragma once
#include "pch.h"

namespace FortAthenaBTTask_Interact
{
	DefineHook(void, OnTaskFinished, UFortAthenaBTTask_Interact* _this, UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, int32 TaskResult);

	void Init();
}
