#pragma once
#include "pch.h"

namespace FortPoiVolume
{
	DefineHook(void, PostInitializeComponents, AFortPoiVolume* _this);

	void Init();
}
