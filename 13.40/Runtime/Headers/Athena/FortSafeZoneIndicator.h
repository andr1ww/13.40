#pragma once
#include "pch.h"

namespace FortSafeZoneIndicator
{
	void SetShrinkDuration(AFortSafeZoneIndicator* _this, float WaitTime);
	void SetFinishShrinkDuration(AFortSafeZoneIndicator* _this, float ShrinkTime);

	void Init();
}
