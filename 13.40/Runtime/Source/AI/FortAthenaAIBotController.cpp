#include "pch.h"
#include "Core.h"
#include "AI/FortAthenaAIBotController.h"
DEFINE_ENGINE_INIT(FortAthenaAIBotController)

void FFortBotInventoryInfo::SetItem(UFortWorldItem* NewItem)
{
	FortItem = NewItem;
	ItemDefinition = NewItem ? NewItem->ItemEntry.ItemDefinition : nullptr;
	ItemType = ItemDefinition ? ItemDefinition->GetItemType() : EFortItemType::Max_None;
	WeaponType = AFortWeapon::FindWeaponTypeForStats(ItemDefinition);
}

bool FortAthenaAIBotController::CreateInventory(IFortInventoryOwnerInterface* _this)
{
	AFortAthenaAIBotController* Controller = Cast<AFortAthenaAIBotController>(_this->_getUObject());

	if (!Controller || Controller->Role != ENetRole::ROLE_Authority)
		return false;

	if (!Controller->Inventory)
	{
		FActorSpawnParameters SpawnParameters{};
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParameters.Owner = Controller;

		Controller->Inventory = GWorld->SpawnActor<AFortInventory>({}, SpawnParameters);
	}

	return Controller->Inventory != nullptr;
}

void FortAthenaAIBotController::InitializeInventory(IFortInventoryOwnerInterface* _this, const TArray<FItemAndCount>& ItemList)
{
	InitializeInventoryOG(_this, ItemList);

	AFortAthenaAIBotController* Controller = Cast<AFortAthenaAIBotController>(_this->_getUObject());
	AFortInventory* Inventory = Controller ? Controller->Inventory : nullptr;

	if (!Inventory)
		return;

	TArray<FItemAndCount> Items{};
	AFortGameModeAthena* GameMode = Cast<AFortGameModeAthena>(GWorld->AuthorityGameMode);

	if (GameMode && Controller->bIsAnAthenaGameParticipant)
		GameMode->GetStartingItems(&Items, true, Controller);

	for (const FItemAndCount& Item : ItemList)
		Items.Add(Item);

	UAthenaPickaxeItemDefinition* Pickaxe = Controller->CosmeticLoadoutBC.Pickaxe;

	if (Pickaxe && Pickaxe->WeaponDefinition)
	{
		FItemAndCount PickaxeEntry{};
		PickaxeEntry.Count = 1;
		PickaxeEntry.Item = Pickaxe->WeaponDefinition;

		Items.Add(PickaxeEntry);
	}

	UFortWorldItem* PickaxeItem = nullptr;

	for (const FItemAndCount& Item : Items)
	{
		if (!Item.Item || Item.Count <= 0)
			continue;

		UFortWorldItem* WorldItem = Inventory->FindExistingItemForDefinition(Item.Item, false);

		if (!WorldItem)
		{
			FFortItemEntry ItemEntry(Item.Item, Item.Count);
			ItemEntry.LoadedAmmo = GetWeaponLoadedAmmo(Cast<UFortWeaponItemDefinition>(Item.Item), 0);

			WorldItem = Inventory->GiveItemToInventoryOwner(ItemEntry);
		}

		if (Pickaxe && Item.Item == Pickaxe->WeaponDefinition)
			PickaxeItem = WorldItem;
	}

	Items.Free();

	UFortWorldItem* PrimaryWeapon = nullptr;
	size_t SlotIndex = 0;

	for (UFortWorldItem* Item : Inventory->GetItems())
	{
		if (SlotIndex >= std::size(Controller->SlotItems))
			break;

		UFortItemDefinition* ItemDefinition = Item ? Item->ItemEntry.ItemDefinition : nullptr;

		if (!ItemDefinition || !ItemDefinition->bInventorySizeLimited)
			continue;

		Controller->SlotItems[SlotIndex++].SetItem(Item);

		if (!PrimaryWeapon && Cast<UFortWeaponItemDefinition>(ItemDefinition))
			PrimaryWeapon = Item;
	}

	UFortWorldItem* WeaponToEquip = PrimaryWeapon ? PrimaryWeapon : PickaxeItem;
	AFortPawn* Pawn = Cast<AFortPawn>(Controller->Pawn);

	if (Pawn && WeaponToEquip)
		Pawn->EquipWeaponDefinition(Cast<UFortWeaponItemDefinition>(WeaponToEquip->ItemEntry.ItemDefinition), WeaponToEquip->ItemEntry.ItemGuid);
}

void FortAthenaAIBotController::Init()
{
	if (IFortInventoryOwnerInterface* Interface = GetInterfaceAddress<IFortInventoryOwnerInterface>(AFortAthenaAIBotController::GetDefaultObj()))
		Hooking::VirtualSwap(*reinterpret_cast<void***>(Interface), 0x3, CreateInventory);

	Hooking::Hook(ImageBase + 0x1BC52B0, InitializeInventory, InitializeInventoryOG);
}
