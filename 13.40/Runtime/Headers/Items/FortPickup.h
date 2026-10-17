#pragma once
#include "pch.h"

namespace FortPickup
{
	void GivePickupTo(AFortPickup* _this, IFortInventoryOwnerInterface* InventoryOwner, bool DestroyAfterPickup);
	DefineHook(void, BeginPlay, AFortPickup* _this);

	void Init();
}
