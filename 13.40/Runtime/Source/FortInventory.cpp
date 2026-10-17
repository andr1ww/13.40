#include "pch.h"
#include "Core.h"
#include "FortInventory.h"
DEFINE_ENGINE_INIT(FortInventory)

UFortWorldItem* AFortInventory::GiveItemToInventoryOwner(FFortItemEntry ItemEntry)
{
	if (!UFortGlobals::IsInAthena(GWorld))
		bAllowMcpItemCreation = true;

	ItemEntry.SetItemGuid(NewGuid());

	UFortWorldItem* Item = UFortWorldItem::New(this, &ItemEntry);

	if (!Item)
		return nullptr;

	if (AFortPlayerController* PC = Cast<AFortPlayerController>(GetOwner()))
		Item->SetOwningControllerForTemporaryItem(PC);

	int32 NumEntries = Inventory.ReplicatedEntries.Num();
	int32 NumInstances = Inventory.ItemInstances.Num();

	InitializeExistingItem(Item);

	if (Inventory.ReplicatedEntries.Num() == NumEntries && Inventory.ItemInstances.Num() == NumInstances)
		return nullptr;

	if (FFortItemEntry* Entry = Inventory.ReplicatedEntries.Search([&](FFortItemEntry& Replicated) { return Replicated.ItemGuid == Item->ItemEntry.ItemGuid; }))
		Inventory.MarkItemDirty(*Entry, false);

	SetRequiresUpdate();

	if (IFortInventoryOwnerInterface* Interface = GetInterfaceAddress<IFortInventoryOwnerInterface>(GetOwner()))
		Item->OnItemInstanceAdded(Interface);

	AFortAthenaAIBotController* BotController = Cast<AFortAthenaAIBotController>(GetOwner());

	if (BotController && Item->ItemEntry.ItemDefinition->bInventorySizeLimited)
	{
		for (FFortBotInventoryInfo& Slot : BotController->SlotItems)
		{
			if (Slot.FortItem)
				continue;

			Slot.SetItem(Item);
			break;
		}
	}

	return Item;
}

bool AFortInventory::RemoveInventoryItem(const FGuid& ItemGuid, int32 Count, bool bForceRemoval)
{
	IFortInventoryOwnerInterface* Owner = GetInterfaceAddress<IFortInventoryOwnerInterface>(GetOwner());
	UFortWorldItem* Item = GetItem(ItemGuid);

	if (!Item || !Owner)
		return false;

	int32 NewCount = Item->ItemEntry.Count - (Count > 0 ? Count : 0);

	if (Count >= 0 && NewCount > 0 && !bForceRemoval)
	{
		Item->SetNumInStack(NewCount, true);
		Item->OnItemInstanceRemoved(Owner, NewCount);
		return true;
	}

	UFortWorldItemDefinition* ItemDefinition = Cast<UFortWorldItemDefinition>(Item->ItemEntry.ItemDefinition);

	bool bKeepStack = ItemDefinition && ItemDefinition->bPersistInInventoryWhenFinalStackEmpty && Count > 0
		&& !Inventory.ReplicatedEntries.Search([&](FFortItemEntry& Entry)
			{
				return Entry.ItemDefinition == ItemDefinition && Entry.ItemGuid != ItemGuid;
			});

	if (bKeepStack)
	{
		Item->SetNumInStack(NewCount, true);
		Item->OnItemInstanceRemoved(Owner, NewCount);
		return true;
	}

	int32 EntryIndex = Inventory.ReplicatedEntries.SearchIndex([&](FFortItemEntry& Entry) { return Entry.ItemGuid == ItemGuid; });
	Inventory.ReplicatedEntries.Remove(EntryIndex);
	Inventory.ItemInstances.Remove(Item);

	for (int32 i = EntryIndex; i >= 0 && i < Inventory.ReplicatedEntries.Num(); i++)
	{
		Inventory.MarkItemDirty(Inventory.ReplicatedEntries[i], false);
	}

	SetRequiresUpdate();
	HandleInventoryLocalUpdate();

	Item->OnItemInstanceRemoved(Owner, Item->ItemEntry.Count);

	if (AFortAthenaAIBotController* BotController = Cast<AFortAthenaAIBotController>(GetOwner()))
	{
		for (FFortBotInventoryInfo& Slot : BotController->SlotItems)
		{
			if (Slot.FortItem == Item)
				Slot.SetItem(nullptr);
		}
	}

	return true;
}

bool FortInventory::RemoveInventoryItem(IFortInventoryOwnerInterface* Interface, const FGuid& ItemGuid, int32 Count, bool bForceRemoveFromQuickBars, bool bForceRemoval)
{
	AFortInventory* Inventory = Interface->GetWorldInventory();

	if (!Inventory)
		return false;

	return Inventory->RemoveInventoryItem(ItemGuid, Count, bForceRemoval);
}

bool FortInventory::SetLoadedAmmo(UFortWorldItem* Item, int32 LoadedAmmo)
{
	if (!Item || !Item->OwnerInventory)
		return false;

	Item->ItemEntry.LoadedAmmo = LoadedAmmo;
	Item->OwnerInventory->SetItemRequiresUpdate(&Item->ItemEntry);
	Item->ItemEntry.bIsDirty = true;

	return true;
}

bool FortInventory::SetPhantomReserveAmmo(UFortWorldItem* Item, int32 PhantomReserveAmmo)
{
	if (!Item || !Item->OwnerInventory)
		return false;

	Item->ItemEntry.PhantomReserveAmmo = PhantomReserveAmmo;
	Item->OwnerInventory->SetItemRequiresUpdate(&Item->ItemEntry);
	Item->ItemEntry.bIsDirty = true;

	return true;
}

int32 FortInventory::GetInventoryCapacity(void* InventoryOwner, int32 InventoryType)
{
	int32 Capacity = GetInventoryCapacityOG(InventoryOwner, InventoryType);

	if (Capacity > 0)
		return Capacity;

	return 50;
}

static void MarkInventoryInitialized(UFortMcpProfileWorld* Profile)
{
	if (!Profile || !Profile->FortInventory)
		return;

	Profile->bIsInventoryInitialized = true;
	Profile->bProfileWriteLocked = true;
	Profile->bProfileLockOperationPending = false;
}

bool FortInventory::IsSafeToSave(UFortMcpProfileWorld* _this)
{
	MarkInventoryInitialized(_this);

	return IsSafeToSaveOG(_this);
}

__int64 FortInventory::BindToPlayerController(UFortMcpProfileWorld* _this, AFortPlayerController* PlayerController, bool bIsOutpost)
{
	__int64 Result = BindToPlayerControllerOG(_this, PlayerController, bIsOutpost);

	if (_this && _this->FortInventory)
		_this->FortInventory->bAllowMcpItemCreation = true;

	return Result;
}

void FortInventory::AttemptUnregistrationSave(UFortRegisteredPlayerInfo* _this)
{
	if (_this)
	{
		MarkInventoryInitialized(_this->WorldProfile);
		MarkInventoryInitialized(_this->OutpostProfile);
	}

	AttemptUnregistrationSaveOG(_this);
}

void FortInventory::Init()
{
	for (UObject* DefaultObject : { static_cast<UObject*>(AFortPlayerControllerAthena::GetDefaultObj()), static_cast<UObject*>(AFortPlayerControllerZone::GetDefaultObj()), static_cast<UObject*>(AFortPlayerController::GetDefaultObj()), static_cast<UObject*>(AFortAthenaAIBotController::GetDefaultObj()) })
	{
		if (IFortInventoryOwnerInterface* Interface = GetInterfaceAddress<IFortInventoryOwnerInterface>(DefaultObject))
			Hooking::VirtualSwap(*reinterpret_cast<void***>(Interface), 0x1A, RemoveInventoryItem);
	}

	Hooking::HookEvery<UFortWorldItem>(0x518 / 8, SetLoadedAmmo);
	Hooking::HookEvery<UFortWorldItem>(0x520 / 8, SetPhantomReserveAmmo);

	Hooking::Hook(ImageBase + 0x25FD8C0, GetInventoryCapacity, GetInventoryCapacityOG);
	Hooking::Hook(ImageBase + 0x253C290, AttemptUnregistrationSave, AttemptUnregistrationSaveOG);
	Hooking::Hook(ImageBase + 0x2B28A40, IsSafeToSave, IsSafeToSaveOG);
	Hooking::Hook(ImageBase + 0x2B129E0, BindToPlayerController, BindToPlayerControllerOG);
}
