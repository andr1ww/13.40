#pragma once
#include "pch.h"

namespace FortAthenaAIBotController
{
	bool CreateInventory(IFortInventoryOwnerInterface* _this);

	DefineHook(void, InitializeInventory, IFortInventoryOwnerInterface* _this, const TArray<FItemAndCount>& ItemList);

	void Init();
}
