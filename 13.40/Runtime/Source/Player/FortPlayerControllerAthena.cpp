#include "pch.h"
#include "Core.h"
#include "Player/FortPlayerControllerAthena.h"
#include "Quests/FortQuestManager.h"
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

void FortPlayerControllerAthena::BindAthenaProfileEvents(AFortPlayerControllerAthena* _this)
{
	if (_this->Role != ENetRole::ROLE_Authority || _this->AthenaProfile == NULL || _this->XPComponent == NULL)
		return;

	_this->XPComponent->BindToProfile(_this->AthenaProfile);

	if (UFortQuestManager* QuestManager = _this->GetQuestManager(ESubGame::Athena))
		QuestManager->RegisterXPComponent(_this->XPComponent);
}

void FortPlayerControllerAthena::UnBindAthenaProfileEvents(AFortPlayerControllerAthena* _this)
{
	if (_this->Role != ENetRole::ROLE_Authority || _this->AthenaProfile == NULL || _this->XPComponent == NULL)
		return;

	_this->XPComponent->UnbindFromProfile(_this->AthenaProfile);

	if (UFortQuestManager* QuestManager = _this->GetQuestManager(ESubGame::Athena))
		QuestManager->UnregisterXPComponent(_this->XPComponent);
}

void FortPlayerControllerAthena::RemoveAllInventoryItems(AFortPlayerControllerAthena* _this)
{
	if (_this->WorldInventory == NULL)
		return;

	TArray<FGuid> ItemGuids{};

	for (UFortWorldItem* Item : _this->WorldInventory->GetItems())
	{
		if (Item && _this->CanRemoveInventoryItem(Item->ItemEntry.ItemGuid))
			ItemGuids.Add(Item->ItemEntry.ItemGuid);
	}

	for (const FGuid& ItemGuid : ItemGuids)
		_this->WorldInventory->RemoveInventoryItem(ItemGuid, -1, true);

	ItemGuids.Free();
}

void FortPlayerControllerAthena::DropItemsAsPickups(AFortPlayerControllerAthena* _this, TArray<UFortWorldItem*>& ItemsToDropViaPickup, AFortPawn* DestructionPawn, AFortPlayerController* InFortPlayerController, EFortPickupSpawnSource SpawnSource)
{
	AFortInventory* Inventory = InFortPlayerController != NULL ? InFortPlayerController->WorldInventory : _this->WorldInventory;

	if (Inventory == NULL || DestructionPawn == NULL)
		return;

	FVector DropLocation = DestructionPawn->K2_GetActorLocation() + FVector{ 0.f, 0.f, 50.f };

	for (UFortWorldItem* Item : ItemsToDropViaPickup)
	{
		FFortCreatePickupData CreatePickupData{};
		CreatePickupData.World = GWorld;
		CreatePickupData.ItemEntry = Item->ItemEntry;
		CreatePickupData.ItemEntry.SetItemGuid(NewGuid());
		CreatePickupData.SpawnLocation = DropLocation;
		CreatePickupData.PickupSourceTypeFlags = EFortPickupSourceTypeFlag::Player;

		AFortPickup* Pickup = AFortPickup::CreateFromData(CreatePickupData);

		if (Pickup != NULL)
		{
			Pickup->PickupSpawnSource = SpawnSource;
			Pickup->PawnWhoDroppedPickup = DestructionPawn;
			Pickup->TossPickup(DropLocation, DestructionPawn, -1, true, true, EFortPickupSourceTypeFlag::Player, SpawnSource);
		}

		Inventory->RemoveInventoryItem(Item->ItemEntry.ItemGuid, -1, true);
	}
}

void FortPlayerControllerAthena::DropItemsAsPickupsAsync(AFortPlayerControllerAthena* _this, TArray<UFortWorldItem*>& ItemsToDropViaPickup, AFortPawn* DestructionPawn)
{
	DropItemsAsPickups(_this, ItemsToDropViaPickup, DestructionPawn, _this, EFortPickupSpawnSource::Unset);
}

void FortPlayerControllerAthena::DropAllItems(AFortPlayerControllerAthena* _this, const UFortItemDefinition* IgnoreItemDef, const UFortItemDefinition* AdditionalIgnoreItemDef, bool bIgnoreBuildingMaterials, bool bSpawnPickups)
{
	AFortInventory* Inventory = _this->WorldInventory;

	if (_this->Role != ENetRole::ROLE_Authority || Inventory == NULL || _this->MyFortPawn == NULL)
		return;

	TArray<UFortWorldItem*> ItemsToDrop{};

	for (UFortWorldItem* Item : Inventory->GetItems())
	{
		if (Item == NULL || Item->ItemEntry.Count <= 0 || !_this->CanRemoveInventoryItem(Item->ItemEntry.ItemGuid))
			continue;

		if (Item->ItemEntry.ItemDefinition == IgnoreItemDef || Item->ItemEntry.ItemDefinition == AdditionalIgnoreItemDef)
			continue;

		UFortWorldItemDefinition* ItemDefinition = Cast<UFortWorldItemDefinition>(Item->ItemEntry.ItemDefinition);

		if (ItemDefinition == NULL)
			continue;

		if (bIgnoreBuildingMaterials && Cast<UFortResourceItemDefinition>(ItemDefinition) != NULL)
			continue;

		bool bShouldDrop = Item->ShouldAlwaysDropItemOnDeathOrLogout()
			&& (ItemDefinition->DropBehavior == EWorldItemDropBehavior::DropAsPickup
				|| (ItemDefinition->DropBehavior == EWorldItemDropBehavior::DropAsPickupDestroyOnEmpty && Item->ItemEntry.Count > 0));
		bool bShouldDestroy = !bShouldDrop;

		_this->ShouldDropOrDestroyByItemType(ItemDefinition, bShouldDrop, bShouldDestroy);

		if (bShouldDrop)
			ItemsToDrop.Add(Item);
	}

	if (bSpawnPickups)
	{
		DropItemsAsPickups(_this, ItemsToDrop, _this->MyFortPawn, _this, EFortPickupSpawnSource::Unset);
	}
	else
	{
		for (UFortWorldItem* Item : ItemsToDrop)
			Inventory->RemoveInventoryItem(Item->ItemEntry.ItemGuid, -1, true);
	}

	ItemsToDrop.Free();
}

void FortPlayerControllerAthena::DropSpecificItem(AFortPlayerControllerAthena* _this, const UFortItemDefinition* DropItemDef)
{
	FFortPickupTossOverrideData TossOverrideData{};

	TossSpecificItem(_this, DropItemDef, TossOverrideData);
}

void FortPlayerControllerAthena::execDropSpecificItem(UObject* Context, FFrame& Stack, void* const Result)
{
	UFortItemDefinition* DropItemDef = Stack.StepCompiledInRef<UFortItemDefinition*>();
	Stack.IncrementCode();

	DropSpecificItem(static_cast<AFortPlayerControllerAthena*>(Context), DropItemDef);
}

void FortPlayerControllerAthena::TossSpecificItem(AFortPlayerControllerAthena* _this, const UFortItemDefinition* DropItemDef, const FFortPickupTossOverrideData& TossOverrideData)
{
	AFortInventory* Inventory = _this->WorldInventory;

	if (_this->Role != ENetRole::ROLE_Authority || Inventory == NULL || DropItemDef == NULL || _this->MyFortPawn == NULL)
		return;

	TArray<UFortWorldItem*> ItemsToDrop{};

	for (UFortWorldItem* Item : Inventory->GetItems())
	{
		if (Item == NULL || Item->ItemEntry.ItemDefinition != DropItemDef || Item->ItemEntry.Count <= 0)
			continue;

		if (!_this->CanRemoveInventoryItem(Item->ItemEntry.ItemGuid) || !Item->ShouldAlwaysDropItemOnDeathOrLogout())
			continue;

		ItemsToDrop.Add(Item);
	}

	DropItemsAsPickups(_this, ItemsToDrop, _this->MyFortPawn, _this, EFortPickupSpawnSource::Unset);

	ItemsToDrop.Free();
}

void FortPlayerControllerAthena::DropItemsOnPawnDestruction(AFortPlayerControllerAthena* _this, EPawnDestructionReason DestructionReason, const FGameplayTagContainer* ContextualTags, AFortPawn* DestructionPawn)
{
	AFortInventory* Inventory = _this->WorldInventory;

	if (_this->Role != ENetRole::ROLE_Authority || Inventory == NULL)
		return;

	AFortPawn* DropPawn = DestructionPawn != NULL ? DestructionPawn : _this->MyFortPawn;

	TArray<UFortWorldItem*> ItemsToDrop{};
	TArray<UFortWorldItem*> ItemsToDestroy{};

	for (UFortWorldItem* Item : Inventory->GetItems())
	{
		if (Item == NULL || Item->ItemEntry.Count <= 0 || !_this->CanRemoveInventoryItem(Item->ItemEntry.ItemGuid))
			continue;

		UFortWorldItemDefinition* ItemDefinition = Cast<UFortWorldItemDefinition>(Item->ItemEntry.ItemDefinition);

		if (ItemDefinition == NULL)
			continue;

		bool bShouldDrop = Item->ShouldAlwaysDropItemOnDeathOrLogout()
			&& (ItemDefinition->DropBehavior == EWorldItemDropBehavior::DropAsPickup
				|| (ItemDefinition->DropBehavior == EWorldItemDropBehavior::DropAsPickupDestroyOnEmpty && Item->ItemEntry.Count > 0));
		bool bShouldDestroy = !bShouldDrop;

		_this->ShouldDropOrDestroyByItemType(ItemDefinition, bShouldDrop, bShouldDestroy);

		if (DestructionReason == Death ? ItemDefinition->bDropOnDeath : ItemDefinition->bDropOnLogout)
			bShouldDrop = true;

		if (bShouldDrop && DropPawn != NULL)
			ItemsToDrop.Add(Item);
		else if (bShouldDestroy)
			ItemsToDestroy.Add(Item);
	}

	DropItemsAsPickups(_this, ItemsToDrop, DropPawn, _this, DestructionReason == Death ? EFortPickupSpawnSource::PlayerElimination : EFortPickupSpawnSource::Unset);

	for (UFortWorldItem* Item : ItemsToDestroy)
		Inventory->RemoveInventoryItem(Item->ItemEntry.ItemGuid, -1, true);

	ItemsToDrop.Free();
	ItemsToDestroy.Free();
}

void FortPlayerControllerAthena::ServerOnMaterialSelection_Implementation(AFortPlayerControllerAthena* _this, EFortResourceType NewResourceType, EFortResourceLevel NewResourceLevel)
{
	_this->CurrentResourceType = NewResourceType;
	_this->CurrentResourceLevel = NewResourceLevel;
}

void FortPlayerControllerAthena::OnPawnDied(AFortPlayerControllerAthena* _this, AFortPlayerPawn* KilledPawn, float Damage, FGameplayTagContainer* InTags, FGameplayEffectContextHandle* EffectContext, AController* EventInstigator, AActor* DamageCauser, AController* DBNOFinisher)
{
	OnPawnDiedOG(_this, KilledPawn, Damage, InTags, EffectContext, EventInstigator, DamageCauser, DBNOFinisher);

	AFortPlayerStateAthena* PlayerState = _this->PlayerState;

	if (PlayerState == NULL || _this->MyFortPawn == NULL || KilledPawn == NULL || DamageCauser == NULL)
		return;

	AFortGameModeAthena* GameMode = Cast<AFortGameModeAthena>(GWorld->AuthorityGameMode);
	AFortGameStateAthena* GameState = GameMode != NULL ? GameMode->GameState : NULL;

	AFortPlayerControllerAthena* KillerController = EventInstigator != NULL ? Cast<AFortPlayerControllerAthena>(EventInstigator) : _this;
	AFortPlayerStateAthena* KillerPlayerState = KillerController != NULL ? KillerController->PlayerState : PlayerState;
	AFortPlayerPawn* KillerPawn = KillerController != NULL ? KillerController->MyFortPawn : NULL;

	PlayerState->PawnDeathLocation = KilledPawn->K2_GetActorLocation();

	PlayerState->DeathInfo = FDeathInfo{};
	PlayerState->DeathInfo.bDBNO = KilledPawn->IsDBNO();
	PlayerState->DeathInfo.DeathLocation = PlayerState->PawnDeathLocation;
	PlayerState->DeathInfo.DeathTags = *InTags;
	PlayerState->DeathInfo.DeathCause = AFortPlayerStateAthena::ToDeathCause(*InTags, PlayerState->DeathInfo.bDBNO);
	PlayerState->DeathInfo.FinisherOrDowner = KillerPlayerState != NULL ? KillerPlayerState : PlayerState;
	PlayerState->DeathInfo.Distance = PlayerState->DeathInfo.DeathCause != EDeathCause::FallDamage && KillerPawn != NULL ? KillerPawn->GetDistanceTo(KilledPawn) : 0.f;
	PlayerState->DeathInfo.bInitialized = true;
	PlayerState->OnRep_DeathInfo();

	const bool bDBNO = PlayerState->DeathInfo.bDBNO;

	if (!bDBNO && KillerPlayerState != NULL && KillerController != NULL && KillerController != _this)
	{
		KillerPlayerState->KillScore++;
		KillerPlayerState->OnRep_Kills();

		KillerPlayerState->TeamKillScore++;

		if (KillerPlayerState->PlayerTeam != NULL)
		{
			for (AController* Member : KillerPlayerState->PlayerTeam->TeamMembers)
			{
				AFortPlayerControllerAthena* MemberController = Cast<AFortPlayerControllerAthena>(Member);
				AFortPlayerStateAthena* MemberState = MemberController != NULL ? MemberController->PlayerState : NULL;

				if (MemberState != NULL)
				{
					MemberState->TeamKillScore = KillerPlayerState->TeamKillScore;
					MemberState->OnRep_TeamKillScore();
					MemberState->ClientReportTeamKill(KillerPlayerState->TeamKillScore);
				}
			}
		}

		KillerPlayerState->OnRep_TeamKillScore();
		KillerPlayerState->ClientReportTeamKill(KillerPlayerState->TeamKillScore);
		KillerPlayerState->ClientReportKill(PlayerState);
	}

	if (bDBNO || GameState == NULL || GameState->IsRespawningAllowed(PlayerState))
		return;

	AFortWeapon* Weapon = Cast<AFortWeapon>(DamageCauser);

	GameMode->RemoveFromAlivePlayers(_this, KillerPlayerState, KillerPawn, Weapon != NULL ? Weapon->WeaponData : NULL, PlayerState->DeathInfo.DeathCause, false);
	_this->bMarkedAlive = false;

	if (GameState->IsResurrectionEnabled(KilledPawn) && !_this->IsSquadWiped())
		return;

	const int32 Placement = GameState->TeamsLeft > 0 ? GameState->TeamsLeft + 1 : 1;

	UFortPlaylistAthena* Playlist = GameState->CurrentPlaylistInfo.OverridePlaylist != NULL ? GameState->CurrentPlaylistInfo.OverridePlaylist : GameState->CurrentPlaylistInfo.BasePlaylist;

	for (TWeakObjectPtr<AFortPlayerStateAthena>& Member : GameState->GetTeamMembers(PlayerState->TeamIndex))
	{
		AFortPlayerStateAthena* MemberState = Member.Get();

		if (MemberState == NULL || MemberState->Place != 0)
			continue;

		AFortPlayerControllerAthena* MemberController = Cast<AFortPlayerControllerAthena>(MemberState->Owner);

		if (MemberController == NULL || MemberController->MatchReport == NULL || MemberController->bMarkedAlive)
			continue;

		MemberState->SetMatchPlacement(Placement);

		UFortQuestManager* QuestManager = MemberController->GetQuestManager(ESubGame::Athena);

		if (QuestManager != NULL && Playlist != NULL)
			FortQuestManager::EndBattleRoyaleMatchForPlayer(QuestManager, Playlist, MemberController->MatchReport->MatchStats);

		if (Placement == 1)
			continue;

		UAthenaPlayerMatchReport* MatchReport = MemberController->MatchReport;

		MatchReport->MatchStats.SetPlacement(Placement);
		MatchReport->MatchStats.SetKills(MemberState->KillScore);
		MatchReport->MatchStats.SetTeamKills(MemberState->TeamKillScore);

		MatchReport->TeamStats.Place = Placement;
		MatchReport->TeamStats.TotalPlayers = GameState->TotalPlayers;

		MemberController->ClientSendMatchStatsForPlayer(MatchReport->MatchStats);
		MemberController->ClientSendTeamStatsForPlayer(MatchReport->TeamStats);
		MemberController->ClientSendEndBattleRoyaleMatchForPlayer(true, MatchReport->EndOfMatchResults);
	}
}

void FortPlayerControllerAthena::ServerPlaySquadQuickChatMessage_Implementation(AFortPlayerControllerAthena* _this, FAthenaQuickChatActiveEntry& ChatEntry, FUniqueNetIdRepl& SenderID)
{
	AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(_this->PlayerState);

	if (PlayerState == NULL)
		return;

	UAthenaQuickChatBank* QuickChatBank = ChatEntry.Bank.Get();

	if (QuickChatBank != NULL && QuickChatBank->ChatOptions.IsValidIndex(ChatEntry.Index))
	{
		FAthenaQuickChatLeafEntry& QuickChatLeafEntry = QuickChatBank->ChatOptions[ChatEntry.Index];

		PlayerState->TeamMemberState = QuickChatLeafEntry.TeamCommType;
		PlayerState->ReplicatedTeamMemberState = QuickChatLeafEntry.TeamCommType;
		PlayerState->OnRep_ReplicatedTeamMemberState();

		_this->ServerPlayEmoteItem(QuickChatLeafEntry.EmojiItemDefinition, UKismetMathLibrary::RandomFloat());
	}
}

void FortPlayerControllerAthena::ServerSetInAircraft_Implementation(AFortPlayerStateAthena* _this, bool bNewInAircraft)
{
	_this->bInAircraft = bNewInAircraft;
}

void FortPlayerControllerAthena::ServerEnableAnonymousMode_Implementation(AFortPlayerControllerAthena* _this)
{
	if (AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(_this->PlayerState))
		PlayerState->bUsingAnonymousMode = true;
}

void FortPlayerControllerAthena::ServerEnableAnonymousCharacterMode_Implementation(AFortPlayerControllerAthena* _this)
{
	if (AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(_this->PlayerState))
		PlayerState->bUsingAnonymousCharacterMode = true;
}

bool FortPlayerControllerAthena::ForceEquipValidWeapon(AFortPlayerControllerAthena* _this)
{
	AFortPlayerPawn* Pawn = _this->MyFortPawn;

	if (Pawn == NULL || _this->WorldInventory == NULL)
		return false;

	if (Pawn->CurrentWeapon != NULL)
		return true;

	for (UFortWorldItem* Item : _this->WorldInventory->GetItems())
	{
		if (Item == NULL)
			continue;

		UFortWeaponItemDefinition* WeaponDefinition = Cast<UFortWeaponItemDefinition>(Item->ItemEntry.ItemDefinition);

		if (WeaponDefinition == NULL)
			continue;

		if (Pawn->EquipWeaponDefinition(WeaponDefinition, Item->ItemEntry.ItemGuid) != NULL)
			return true;
	}

	return false;
}

void FortPlayerControllerAthena::Init()
{
	Hooking::Hook<AFortPlayerControllerAthena>(0x868 / 8, ServerAcknowledgePossession_Implementation);
	Hooking::Hook<AFortPlayerControllerAthena>(0x1590 / 8, OnReadyToStartMatch, OnReadyToStartMatchOG);
	Hooking::Hook<AFortPlayerControllerAthena>(0x1358 / 8, ServerLoadingScreenDropped_Implementation, ServerLoadingScreenDropped_ImplementationOG);
	Hooking::Hook<AFortPlayerControllerAthena>(0x1F80 / 8, OnPawnDied, OnPawnDiedOG);
	Hooking::Hook<AFortPlayerControllerAthena>(0x1E78 / 8, BindAthenaProfileEvents);
	Hooking::Hook<AFortPlayerControllerAthena>(0x1E80 / 8, UnBindAthenaProfileEvents);
	Hooking::Hook<AFortPlayerControllerAthena>(0x1B48 / 8, DropItemsOnPawnDestruction);
	Hooking::Hook<AFortPlayerControllerAthena>(0x1B58 / 8, DropItemsAsPickupsAsync);
	Hooking::Hook<AFortPlayerControllerAthena>(0x1680 / 8, ForceEquipValidWeapon);
	Hooking::Hook<AFortPlayerControllerAthena>(0x2810 / 8, ServerPlaySquadQuickChatMessage_Implementation);
	Hooking::Hook<AFortPlayerControllerAthena>(0x2518 / 8, ServerEnableAnonymousMode_Implementation);
	Hooking::Hook<AFortPlayerControllerAthena>(0x2508 / 8, ServerEnableAnonymousCharacterMode_Implementation);
	Hooking::Hook<AFortPlayerStateAthena>(0x7F8 / 8, ServerSetInAircraft_Implementation);
	Hooking::HookEvery<AFortPlayerController>(0x1130 / 8, ServerOnMaterialSelection_Implementation);
	Hooking::ExecHook(AFortPlayerController::StaticClass()->GetFunction("FortPlayerController", "DropSpecificItem"), execDropSpecificItem);

	Hooking::Rel32Swap(ImageBase + 0x1ED61B6, RemoveAllInventoryItems);
	Hooking::Rel32Swap(ImageBase + 0x3064A81, DropAllItems);
	Hooking::Rel32Swap(ImageBase + 0x25F2527, DropAllItems);
	Hooking::Rel32Swap(ImageBase + 0x306F103, TossSpecificItem);
}
