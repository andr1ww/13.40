#include "pch.h"
#include "Core.h"
#include "FortInventory.h"
DEFINE_ENGINE_INIT(FortInventory)

UFortWorldItem* AFortInventory::GiveItemToInventoryOwner(FFortItemEntry ItemEntry)
{
	bAllowMcpItemCreation = true;

	ItemEntry.SetItemGuid(NewGuid());

	UFortWorldItem* Item = UFortWorldItem::New(this, &ItemEntry);

	if (!Item)
		return nullptr;

	if (AFortPlayerController* PC = Cast<AFortPlayerController>(GetOwner()))
		Item->SetOwningControllerForTemporaryItem(PC);

	InitializeExistingItem(Item);

	if (!Inventory.ItemInstances.Contains(Item))
		return nullptr;

	HandleInventoryLocalUpdate();

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
	UFortWorldItem* Item = GetItem(ItemGuid);

	if (!Item)
		return false;

	int32 NewCount = Item->ItemEntry.Count - (Count > 0 ? Count : 0);
	UFortWorldItemDefinition* ItemDefinition = Cast<UFortWorldItemDefinition>(Item->ItemEntry.ItemDefinition);

	bool bKeepStack = (Count >= 0 && NewCount > 0) || (!bForceRemoval && ItemDefinition && ItemDefinition->bPersistInInventoryWhenFinalStackEmpty && Count > 0
		&& !Inventory.ReplicatedEntries.Search([&](FFortItemEntry& Entry)
			{
				return Entry.ItemDefinition == ItemDefinition && Entry.ItemGuid != ItemGuid;
			}));

	if (bKeepStack)
	{
		if (!Item->SetNumInStack(NewCount, true))
			return false;

		HandleInventoryLocalUpdate();
		return true;
	}

	int32 EntryIndex = Inventory.ReplicatedEntries.SearchIndex([&](FFortItemEntry& Entry) { return Entry.ItemGuid == ItemGuid; });

	if (EntryIndex == -1)
		return false;

	Inventory.ReplicatedEntries.RemoveAtSwap(EntryIndex);

	SetRequiresUpdate();
	HandleInventoryLocalUpdate();

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

bool FortInventory::SetDurability(UFortWorldItem* Item, float Durability)
{
	if (!Item || !Item->OwnerInventory)
		return false;

	Item->ItemEntry.SetDurability(Durability);

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
	for (UObject* DefaultObject : { static_cast<UObject*>(AFortPlayerControllerAthena::GetDefaultObj()), static_cast<UObject*>(AFortPlayerControllerZone::GetDefaultObj()), static_cast<UObject*>(AFortPlayerController::GetDefaultObj()), static_cast<UObject*>(AFortAthenaAIBotController::GetDefaultObj()), static_cast<UObject*>(AFortAIPawn::GetDefaultObj()) })
	{
		if (IFortInventoryOwnerInterface* Interface = GetInterfaceAddress<IFortInventoryOwnerInterface>(DefaultObject))
			Hooking::VirtualSwap(*reinterpret_cast<void***>(Interface), 0x1A, RemoveInventoryItem);
	}

	Hooking::HookEvery<UFortWorldItem>(0x4E0 / 8, SetDurability);
	Hooking::HookEvery<UFortWorldItem>(0x518 / 8, SetLoadedAmmo);
	Hooking::HookEvery<UFortWorldItem>(0x520 / 8, SetPhantomReserveAmmo);

	Hooking::Hook(ImageBase + 0x25FD8C0, GetInventoryCapacity, GetInventoryCapacityOG);
	Hooking::Hook(ImageBase + 0x253C290, AttemptUnregistrationSave, AttemptUnregistrationSaveOG);
	Hooking::Hook(ImageBase + 0x2B28A40, IsSafeToSave, IsSafeToSaveOG);
	Hooking::Hook(ImageBase + 0x2B129E0, BindToPlayerController, BindToPlayerControllerOG);
}
