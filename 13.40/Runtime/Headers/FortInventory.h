#pragma once
#include "pch.h"

namespace FortInventory
{
	bool RemoveInventoryItem(IFortInventoryOwnerInterface* Interface, const FGuid& ItemGuid, int32 Count, bool bForceRemoveFromQuickBars, bool bForceRemoval);
	bool SetLoadedAmmo(UFortWorldItem* Item, int32 LoadedAmmo);
	bool SetPhantomReserveAmmo(UFortWorldItem* Item, int32 PhantomReserveAmmo);

	DefineHook(int32, GetInventoryCapacity, void* InventoryOwner, int32 InventoryType);

	DefineHook(void, AttemptUnregistrationSave, UFortRegisteredPlayerInfo* _this);
	DefineHook(bool, IsSafeToSave, UFortMcpProfileWorld* _this);
	DefineHook(__int64, BindToPlayerController, UFortMcpProfileWorld* _this, AFortPlayerController* PlayerController, bool bIsOutpost);

	void Init();
}
