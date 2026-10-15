#pragma once
#include "pch.h"

namespace AnimNotifyQueue
{
	DefineHook(void, AddAnimNotifiesToDest, FAnimNotifyQueue* _this, bool bSrcIsLeader, const TArray<FAnimNotifyEventReference>& NewNotifies, TArray<FAnimNotifyEventReference>& DestArray, float InstanceWeight);

	void Init();
}
