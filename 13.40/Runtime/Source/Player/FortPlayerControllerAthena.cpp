#include "pch.h"
#include "Core.h"
#include "Player/FortPlayerControllerAthena.h"
DEFINE_ENGINE_INIT(FortPlayerControllerAthena)

void FortPlayerControllerAthena::ServerLoadingScreenDropped_Implementation(AFortPlayerControllerAthena* _this)
{
	ServerLoadingScreenDropped_ImplementationOG(_this);

	if (!_this->WorldInventory || !_this->CosmeticLoadoutPC.Pickaxe)
		return;

	UFortWeaponMeleeItemDefinition* WeaponDefinition = _this->CosmeticLoadoutPC.Pickaxe->WeaponDefinition;

	if (!WeaponDefinition)
		return;

	TArray<UFortWorldItem*>& Items = _this->WorldInventory->GetItems();

	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (Items[i] && Items[i]->ItemEntry.ItemDefinition == WeaponDefinition)
			return;
	}

	_this->WorldInventory->GiveItemToInventoryOwner(FFortItemEntry(WeaponDefinition, 1));
}

void FortPlayerControllerAthena::OnReadyToStartMatch(AFortPlayerControllerAthena* _this)
{
	if (!_this->WorldInventory || _this->WorldInventory->GetItems().Num() > 0)
		return OnReadyToStartMatchOG(_this);

	if (AFortGameModeAthena* GameMode = Cast<AFortGameModeAthena>(GWorld->AuthorityGameMode))
	{
		TArray<FItemAndCount> StartingItems{};
		GameMode->GetStartingItems(&StartingItems, true, _this);

		for (int32 i = 0; i < StartingItems.Num(); i++)
		{
			FItemAndCount& Item = StartingItems[i];

			if (Item.Count <= 0)
				continue;

			_this->WorldInventory->GiveItemToInventoryOwner(FFortItemEntry(Item.Item, Item.Count));
		}
	}

	OnReadyToStartMatchOG(_this);
}

void FortPlayerControllerAthena::ServerAcknowledgePossession_Implementation(AFortPlayerControllerAthena* _this, APawn* P)
{
	_this->AcknowledgedPawn = P;
}

void FortPlayerControllerAthena::Init()
{
	Hooking::Hook<AFortPlayerControllerAthena>(0x868 / 8, ServerAcknowledgePossession_Implementation);
	Hooking::Hook<AFortPlayerControllerAthena>(0x1590 / 8, OnReadyToStartMatch, OnReadyToStartMatchOG);
	Hooking::Hook<AFortPlayerControllerAthena>(0x1358 / 8, ServerLoadingScreenDropped_Implementation, ServerLoadingScreenDropped_ImplementationOG);
}
