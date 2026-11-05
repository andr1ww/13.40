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

	for (FFortBotInventoryInfo& Slot : Controller->SlotItems)
	{
		if (Slot.FortItem && Cast<UFortWeaponItemDefinition>(Slot.ItemDefinition))
		{
			PrimaryWeapon = Slot.FortItem;
			break;
		}
	}

	UFortWorldItem* WeaponToEquip = PrimaryWeapon ? PrimaryWeapon : PickaxeItem;
	AFortPawn* Pawn = Cast<AFortPawn>(Controller->Pawn);

	if (Pawn && WeaponToEquip)
		Pawn->EquipWeaponDefinition(Cast<UFortWeaponItemDefinition>(WeaponToEquip->ItemEntry.ItemDefinition), WeaponToEquip->ItemEntry.ItemGuid);
}

void FortAthenaAIBotController::OnPossesedPawnDied(AFortAthenaAIBotController* _this, AActor* DamagedActor, float Damage, AController* InstigatedBy, AActor* DamageCauser, const FVector& HitLocation, UPrimitiveComponent* FHitComponent, FName BoneName, const FVector& Momentum)
{
	AFortPlayerPawn* KilledPawn = Cast<AFortPlayerPawn>(DamagedActor);
	AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(_this->PlayerState);

	if (KilledPawn && KilledPawn == _this->PlayerBotPawn && PlayerState && _this->bIsAnAthenaGameParticipant)
	{
		AFortPlayerStateAthena* KillerPlayerState = InstigatedBy ? Cast<AFortPlayerStateAthena>(InstigatedBy->PlayerState) : nullptr;
		AFortPawn* KillerPawn = InstigatedBy ? Cast<AFortPawn>(InstigatedBy->Pawn) : nullptr;

		PlayerState->PawnDeathLocation = KilledPawn->K2_GetActorLocation();

		PlayerState->DeathInfo = FDeathInfo{};
		PlayerState->DeathInfo.bDBNO = KilledPawn->IsDBNO();
		PlayerState->DeathInfo.DeathLocation = PlayerState->PawnDeathLocation;
		UBlueprintGameplayTagLibrary::AppendGameplayTagContainers(PlayerState->DeathInfo.DeathTags, KilledPawn->DeathTags);
		PlayerState->DeathInfo.DeathCause = AFortPlayerStateAthena::ToDeathCause(KilledPawn->DeathTags, PlayerState->DeathInfo.bDBNO);
		PlayerState->DeathInfo.FinisherOrDowner = KillerPlayerState ? KillerPlayerState : PlayerState;
		PlayerState->DeathInfo.Distance = PlayerState->DeathInfo.DeathCause != EDeathCause::FallDamage && KillerPawn ? KillerPawn->GetDistanceTo(KilledPawn) : 0.f;
		PlayerState->DeathInfo.bInitialized = true;

		if (!PlayerState->DeathInfo.bDBNO && KillerPlayerState && KillerPlayerState != PlayerState)
		{
			AFortPlayerControllerAthena* KillerController = Cast<AFortPlayerControllerAthena>(InstigatedBy);

			KillerPlayerState->KillScore++;
			KillerPlayerState->TeamKillScore++;

			if (KillerPlayerState->PlayerTeam)
			{
				for (AController* Member : KillerPlayerState->PlayerTeam->TeamMembers)
				{
					AFortPlayerStateAthena* MemberState = Member ? Cast<AFortPlayerStateAthena>(Member->PlayerState) : nullptr;

					if (!MemberState || MemberState == KillerPlayerState)
						continue;

					MemberState->TeamKillScore = KillerPlayerState->TeamKillScore;

					if (Cast<AFortPlayerControllerAthena>(Member))
						MemberState->ClientReportTeamKill(KillerPlayerState->TeamKillScore);
				}
			}

			if (KillerController)
			{
				KillerPlayerState->ClientReportTeamKill(KillerPlayerState->TeamKillScore);
				KillerPlayerState->ClientReportKill(PlayerState);
			}
		}
	}

	OnPossesedPawnDiedOG(_this, DamagedActor, Damage, InstigatedBy, DamageCauser, HitLocation, FHitComponent, BoneName, Momentum);
}

void FortAthenaAIBotController::DropItemsAsPickups(TArray<UFortWorldItem*>& ItemsToDropViaPickup, AFortPawn* DestructionPawn, AFortInventory* Inventory)
{
	if (!DestructionPawn || !Inventory)
		return;

	AFortAthenaAIBotController* Controller = Cast<AFortAthenaAIBotController>(Inventory->GetOwner());
	const bool bDropWeaponAmmo = Controller && Controller->bIsAnAthenaGameParticipant;
	const FVector DropLocation = DestructionPawn->K2_GetActorLocation() + FVector{ 0.f, 0.f, 50.f };

	TArray<FFortItemEntry> Drops{};

	for (UFortWorldItem* Item : ItemsToDropViaPickup)
	{
		FFortItemEntry& Drop = Drops.Add(Item->ItemEntry);
		UFortWeaponItemDefinition* WeaponDefinition = Cast<UFortWeaponItemDefinition>(Item->ItemEntry.ItemDefinition);

		if (!WeaponDefinition || Cast<UFortWeaponMeleeItemDefinition>(WeaponDefinition))
			continue;

		const int32 ClipSize = GetWeaponLoadedAmmo(WeaponDefinition, Item->ItemEntry.Level);

		if (ClipSize > 0 && Drop.LoadedAmmo > ClipSize)
			Drop.LoadedAmmo = ClipSize;

		UFortWorldItemDefinition* AmmoDefinition = bDropWeaponAmmo ? WeaponDefinition->AmmoData.LoadSynchronous() : nullptr;

		if (!AmmoDefinition || AmmoDefinition == WeaponDefinition || AmmoDefinition->DropCount <= 0)
			continue;

		if (ItemsToDropViaPickup.Search([&](UFortWorldItem* Entry) { return Entry->ItemEntry.ItemDefinition == AmmoDefinition; }))
			continue;

		if (Drops.Search([&](FFortItemEntry& Entry) { return Entry.ItemDefinition == AmmoDefinition; }))
			continue;

		Drops.Add(FFortItemEntry(AmmoDefinition, AmmoDefinition->DropCount));
	}

	for (FFortItemEntry& Drop : Drops)
	{
		FFortCreatePickupData CreatePickupData{};
		CreatePickupData.World = GWorld;
		CreatePickupData.ItemEntry = Drop;
		CreatePickupData.ItemEntry.SetItemGuid(NewGuid());
		CreatePickupData.SpawnLocation = DropLocation;
		CreatePickupData.PickupSourceTypeFlags = EFortPickupSourceTypeFlag::AI;

		if (AFortPickup* Pickup = AFortPickup::CreateFromData(CreatePickupData))
		{
			Pickup->PawnWhoDroppedPickup = DestructionPawn;
			Pickup->TossPickup(DropLocation, DestructionPawn, -1, true, true, EFortPickupSourceTypeFlag::AI, EFortPickupSpawnSource::Unset);
		}
	}

	for (UFortWorldItem* Item : ItemsToDropViaPickup)
		Inventory->RemoveInventoryItem(Item->ItemEntry.ItemGuid, -1, true);

	Drops.Free();
}

UFortHero* FortAthenaAIBotController::GetHero(AFortAthenaAIBotController* _this)
{
	UFortHero* Hero = _this->CachedHero.Get();
	UAthenaCharacterItemDefinition* Character = _this->CosmeticLoadoutBC.Character;

	if (Hero && Character && Hero->ItemDefinition != Character->HeroDefinition)
		_this->CachedHero.Reset();

	return GetHeroOG(_this);
}

void FortAthenaAIBotController::RemoveAllInventoryItems(AFortAthenaAIBotController* _this)
{
	if (_this->Inventory == NULL)
		return;

	TArray<FGuid> ItemGuids{};

	for (UFortWorldItem* Item : _this->Inventory->GetItems())
	{
		UFortItemDefinition* ItemDefinition = Item ? Item->ItemEntry.ItemDefinition : nullptr;

		if (!ItemDefinition || ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) || ItemDefinition->IsA(UFortBuildingItemDefinition::StaticClass()) || ItemDefinition->IsA(UFortEditToolItemDefinition::StaticClass()))
			continue;

		ItemGuids.Add(Item->ItemEntry.ItemGuid);
	}

	for (const FGuid& ItemGuid : ItemGuids)
		_this->Inventory->RemoveInventoryItem(ItemGuid, -1, true);

	ItemGuids.Free();
}

void FortAthenaAIBotController::Init()
{
	if (IFortInventoryOwnerInterface* Interface = GetInterfaceAddress<IFortInventoryOwnerInterface>(AFortAthenaAIBotController::GetDefaultObj()))
		Hooking::VirtualSwap(*reinterpret_cast<void***>(Interface), 0x3, CreateInventory);

	Hooking::Hook(ImageBase + 0x1BC52B0, InitializeInventory, InitializeInventoryOG);
	Hooking::Hook(ImageBase + 0x1BC9E60, OnPossesedPawnDied, OnPossesedPawnDiedOG);
	Hooking::Hook(ImageBase + 0x1BBECD0, GetHero, GetHeroOG);
	Hooking::Rel32Swap(ImageBase + 0x1BB702A, DropItemsAsPickups);
}
