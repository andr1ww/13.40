#include "pch.h"
#include "Core.h"
#include "Items/FortPickup.h"
DEFINE_ENGINE_INIT(FortPickup)

void FortPickup::GivePickupTo(AFortPickup* _this, IFortInventoryOwnerInterface* InventoryOwner, bool DestroyAfterPickup)
{
	if (_this->Role != ENetRole::ROLE_Authority || !InventoryOwner)
		return;

	AFortInventory* Inventory = InventoryOwner->GetWorldInventory();

	if (!Inventory)
		return;

	AFortPawn* PickupTarget = _this->PickupLocationData.PickupTarget;
	AFortPlayerController* PlayerController = PickupTarget ? Cast<AFortPlayerController>(PickupTarget->Controller) : nullptr;

	FVector DropOrigin = _this->K2_GetActorLocation();
	FVector DropTarget = DropOrigin;

	if (PickupTarget)
	{
		FVector Forward = PickupTarget->GetActorForwardVector();
		Forward.Z = 0.f;
		Forward.Normalize();

		DropOrigin = PickupTarget->K2_GetActorLocation() + Forward * 70.f + FVector{ 0.f, 0.f, 50.f };
		DropTarget = PickupTarget->K2_GetActorLocation() + Forward * 300.f;
	}

	TArray<FFortItemEntry*> PickupEntries{};
	PickupEntries.Add(&_this->PrimaryPickupItemEntry);

	for (int32 i = 0; i < _this->MultiItemPickupEntries.Num(); i++)
		PickupEntries.Add(&_this->MultiItemPickupEntries[i]);

	bool bSwappedHeldWeapon = false;

	for (int32 i = 0; i < PickupEntries.Num(); i++)
	{
		FFortItemEntry* PickupEntry = PickupEntries[i];
		UFortWorldItemDefinition* ItemDefinition = Cast<UFortWorldItemDefinition>(PickupEntry->ItemDefinition);

		if (!ItemDefinition || PickupEntry->Count <= 0)
			continue;

		int32 MaxStackSize = max(1, AFortInventory::GetItemMaxStackSize(PlayerController, ItemDefinition));
		int32 CountToDrop = Inventory->GetOverflowFromAddingItem(*PickupEntry);

		bool bTakesQuickBarSlot = !ItemDefinition->IsA<UFortResourceItemDefinition>() && !ItemDefinition->IsA<UFortAmmoItemDefinition>();

		if (CountToDrop >= PickupEntry->Count && bTakesQuickBarSlot && !bSwappedHeldWeapon && PlayerController && PickupTarget->CurrentWeapon)
		{
			FGuid HeldGuid = PickupTarget->CurrentWeapon->ItemEntryGuid;
			UFortWorldItem* HeldItem = Inventory->GetItem(HeldGuid);

			if (HeldItem && PlayerController->CanRemoveInventoryItem(HeldGuid))
			{
				FFortCreatePickupData CreateHeldPickupData{};
				CreateHeldPickupData.World = GWorld;
				CreateHeldPickupData.ItemEntry = HeldItem->ItemEntry;
				CreateHeldPickupData.SpawnLocation = DropOrigin;
				CreateHeldPickupData.PickupSourceTypeFlags = EFortPickupSourceTypeFlag::Player;

				if (AFortPickup* SwappedPickup = AFortPickup::CreateFromData(CreateHeldPickupData))
					SwappedPickup->TossPickup(DropTarget, PickupTarget, -1, true, true, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset);

				bSwappedHeldWeapon = true;
				Inventory->RemoveInventoryItem(HeldGuid, -1, true);

				CountToDrop = Inventory->GetOverflowFromAddingItem(*PickupEntry);
			}
		}

		CountToDrop = std::clamp(CountToDrop, 0, PickupEntry->Count);

		int32 CountToGive = PickupEntry->Count - CountToDrop;
		UFortWorldItem* ExistingItem = Inventory->FindExistingItemForDefinition(ItemDefinition, false);

		if (ExistingItem && CountToGive > 0)
		{
			const int32 AmountToStack = min(CountToGive, MaxStackSize - ExistingItem->ItemEntry.Count);

			if (AmountToStack > 0)
			{
				ExistingItem->ItemEntry.SetCount(ExistingItem->ItemEntry.Count + AmountToStack);
				Inventory->SetItemRequiresUpdate(&ExistingItem->ItemEntry);
				CountToGive -= AmountToStack;
			}
		}

		UFortWorldItem* GivenItem = nullptr;

		if (CountToGive > 0)
		{
			FFortItemEntry NewEntry = *PickupEntry;
			NewEntry.SetCount(min(CountToGive, MaxStackSize));

			GivenItem = Inventory->GiveItemToInventoryOwner(NewEntry);

			if (GivenItem)
				CountToGive -= NewEntry.Count;
		}

		CountToDrop += CountToGive;

		if (bSwappedHeldWeapon && GivenItem)
		{
			PlayerController->ServerExecuteInventoryItem(GivenItem->ItemEntry.ItemGuid);
			PlayerController->ClientEquipItem(GivenItem->ItemEntry.ItemGuid, true);
		}

		if (CountToDrop > 0)
		{
			FFortCreatePickupData CreatePickupData{};
			CreatePickupData.World = GWorld;
			CreatePickupData.ItemEntry = *PickupEntry;
			CreatePickupData.ItemEntry.SetCount(CountToDrop);
			CreatePickupData.ItemEntry.SetItemGuid(NewGuid());
			CreatePickupData.SpawnLocation = DropOrigin;
			CreatePickupData.PickupSourceTypeFlags = EFortPickupSourceTypeFlag::Player;

			if (AFortPickup* OverflowPickup = AFortPickup::CreateFromData(CreatePickupData))
				OverflowPickup->TossPickup(DropTarget, PickupTarget, -1, true, true, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset);
		}
	}

	PickupEntries.Free();

	Inventory->SetRequiresUpdate();
	Inventory->HandleInventoryLocalUpdate();

	_this->bPickedUp = true;
	_this->OnRep_bPickedUp();

	_this->UpdateSpecialActorStat(ESpecialActorStatType::PickupNumTaken, 1.f);

	if (DestroyAfterPickup)
		_this->K2_DestroyActor();
}

void FortPickup::Init()
{
	Hooking::HookEvery<AFortPickup>(0x650 / 8, GivePickupTo);
}
