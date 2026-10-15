#pragma once
#include "pch.h"

namespace BuildingFoundation
{
	void execSetDynamicFoundationEnabled(UObject* Context, FFrame& Stack, void* const Result);
	void execSetDynamicFoundationTransform(UObject* Context, FFrame& Stack, void* const Result);

	DefineHook(void, SelectAndSetupMyBuildingLevel, ABuildingFoundation* _this, void* RandomStream);
	bool StreamAdditionalWorld(ABuildingFoundation* _this);

	void Init();
}
