#pragma once
#include "pch.h"

namespace FortPickup
{
	void GivePickupTo(AFortPickup* _this, IFortInventoryOwnerInterface* InventoryOwner, bool DestroyAfterPickup);

	void Init();
}
