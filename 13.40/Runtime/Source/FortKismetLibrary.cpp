#include "pch.h"
#include "Core.h"
#include "FortKismetLibrary.h"
#include "Items/FortLootPackage.h"
#include "Items/FortLootTier.h"
#include "Player/FortPlayerController.h"

DEFINE_ENGINE_INIT(FortKismetLibrary)

bool FortKismetLibrary::PickLootDrops(UObject* WorldContextObject, TArray<FFortItemEntry>& OutLootToDrop, FName TierGroupName, int32 WorldLevel, int32 ForcedLootTier)
{
	FName LootTierKey;
	int32 LootTier = -1;

	if (!UFortLootTier::PickLootTierKeyAndLootTierFromTierGroup(LootTierKey, &LootTier, TierGroupName, WorldLevel, NULL, ForcedLootTier))
		return false;

	return UFortLootPackage::PickLootDrops(OutLootToDrop, WorldLevel, LootTierKey, NULL, NULL, NULL, false, false);
}

void FortKismetLibrary::GiveItemToInventoryOwner(TScriptInterface<class IFortInventoryOwnerInterface> InventoryOwner, const class UFortWorldItemDefinition* ItemDefinition, int32 NumberToGive, bool bNotifyPlayer, int32 ItemLevel, int32 PickupInstigatorHandle)
{
	if (!InventoryOwner.ObjectPointer || !ItemDefinition || NumberToGive <= 0)
		return;

	AFortPlayerController* PlayerController = Cast<AFortPlayerController>(InventoryOwner.ObjectPointer);

	if (PlayerController == NULL)
		return;

	UFortWorldItemDefinition* MutableDefinition = const_cast<UFortWorldItemDefinition*>(ItemDefinition);

	if (PlayerController->WorldInventory == NULL)
		return;

	AFortInventory* Inventory = PlayerController->WorldInventory;

	const int32 ActualMaxStackSize = AFortInventory::GetItemMaxStackSize(PlayerController, MutableDefinition);
	const int32 MaxStackSize = ActualMaxStackSize > 0 ? ActualMaxStackSize : INT32_MAX;

	FFortItemEntry ItemEntry(ItemDefinition, NumberToGive, ItemLevel);

	if (UFortWeaponItemDefinition* WeaponDefinition = Cast<UFortWeaponItemDefinition>(MutableDefinition))
	{
		if (FFortBaseWeaponStats* WeaponStats = FindRow<FFortBaseWeaponStats>(WeaponDefinition->WeaponStatHandle.DataTable, WeaponDefinition->WeaponStatHandle.RowName))
			ItemEntry.LoadedAmmo = WeaponStats->ClipSize;
	}

	int32 CountToDrop = std::clamp(Inventory->GetOverflowFromAddingItem(ItemEntry), 0, NumberToGive);
	int32 CountToGive = NumberToGive - CountToDrop;

	UFortWorldItem* ExistingItem = Inventory->FindExistingItemForDefinition(MutableDefinition, false);

	if (ExistingItem != NULL && CountToGive > 0)
	{
		const int32 SpaceInStack = MaxStackSize - ExistingItem->ItemEntry.Count;

		if (SpaceInStack > 0)
		{
			const int32 AmountToStack = CountToGive < SpaceInStack ? CountToGive : SpaceInStack;

			ExistingItem->ItemEntry.SetCount(ExistingItem->ItemEntry.Count + AmountToStack);
			CountToGive -= AmountToStack;
		}
	}

	while (CountToGive > 0)
	{
		const int32 StackCount = CountToGive < MaxStackSize ? CountToGive : MaxStackSize;

		FFortItemEntry NewEntry = ItemEntry;
		NewEntry.SetCount(StackCount);

		if (Inventory->GiveItemToInventoryOwner(NewEntry) == NULL)
			break;

		CountToGive -= StackCount;
	}

	Inventory->SetRequiresUpdate();
	Inventory->HandleInventoryLocalUpdate();
}

void FortKismetLibrary::K2_GiveItemToAllPlayers(class UObject* WorldContextObject, class UFortWorldItemDefinition* ItemDefinition, int32 NumberToGive, bool bNotifyPlayer)
{
	TArray<AFortPlayerController*> AllControllers = GetAll<AFortPlayerController>();
	for (auto& Controller : AllControllers)
	{
		if (!Controller || !Controller->WorldInventory)
			continue;

		TScriptInterface<IFortInventoryOwnerInterface> InventoryOwner{};
		InventoryOwner.ObjectPointer = Controller;

		GiveItemToInventoryOwner(InventoryOwner, ItemDefinition, NumberToGive, bNotifyPlayer, 1, -1);
	}
}

void FortKismetLibrary::K2_GiveItemToPlayer(AFortPlayerController* PlayerController, const class UFortWorldItemDefinition* ItemDefinition, int32 NumberToGive, bool bNotifyPlayer)
{
	if (PlayerController == NULL || ItemDefinition == NULL)
		return;

	if (!PlayerController->IsA<AFortPlayerController>())
		return K2_GiveItemToAllPlayers(PlayerController, const_cast<UFortWorldItemDefinition*>(ItemDefinition), NumberToGive, bNotifyPlayer);

	TScriptInterface<IFortInventoryOwnerInterface> InventoryOwner{};
	InventoryOwner.ObjectPointer = PlayerController;

	GiveItemToInventoryOwner(InventoryOwner, ItemDefinition, NumberToGive, bNotifyPlayer, 1, -1);
}

void FortKismetLibrary::ChangeTeam(AActor* PlayerToSwitch, AActor* Instigator, uint8 NewTeam, FGameplayTagContainer ChangeTeamTags)
{
	AFortPlayerController* Controller = Cast<AFortPlayerController>(PlayerToSwitch);
	AFortPlayerStateAthena* PlayerState = Controller ? Cast<AFortPlayerStateAthena>(Controller->PlayerState) : Cast<AFortPlayerStateAthena>(PlayerToSwitch);

	if (PlayerState == NULL || PlayerState->TeamIndex == NewTeam)
		return;

	AFortTeamInfo* NewTeamInfo = UFortKismetLibrary::GetTeamInfo(GWorld, NewTeam);

	PlayerState->TeamIndex = NewTeam;
	PlayerState->OnRep_TeamIndex(PlayerState->TeamIndex);

	PlayerState->SquadId = NewTeam - 3;
	PlayerState->OnRep_SquadId();

	if (NewTeamInfo != NULL)
	{
		PlayerState->PlayerTeam = NewTeamInfo;
		PlayerState->OnRep_PlayerTeam();
	}
}

void FortKismetLibrary::K2_RemoveItemFromAllPlayers(class UObject* WorldContextObject, class UFortWorldItemDefinition* ItemDefinition, int32 AmountToRemove)
{
	AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(GWorld->GameState);

	if (GameState == NULL || ItemDefinition == NULL)
		return;

	for (int32 PlayerIndex = 0; PlayerIndex < GameState->PlayerArray.Num(); PlayerIndex++)
	{
		AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(GameState->PlayerArray[PlayerIndex]);

		if (PlayerState == NULL)
			continue;

		if (AFortPlayerController* PlayerController = Cast<AFortPlayerController>(PlayerState->Owner))
			FortPlayerController::RemoveItemFromPlayer(PlayerController, ItemDefinition, AmountToRemove, false);
	}
}

void FortKismetLibrary::K2_GiveBuildingResource(class AFortPlayerController* Controller, const EFortResourceType ResourceType, const int32 ResourceAmount)
{
	UFortGameData* GameData = UFortGameData::GetGameData();

	if (Controller == NULL || GameData == NULL || ResourceAmount <= 0)
		return;

	UFortResourceItemDefinition* ResourceItemDefinition = GameData->GetResourceItemDefinition(ResourceType);

	if (ResourceItemDefinition != NULL)
		K2_GiveItemToPlayer(Controller, ResourceItemDefinition, ResourceAmount, false);
}

AFortPickup* FortKismetLibrary::K2_SpawnPickupInWorld(class UObject* WorldContextObject, class UFortWorldItemDefinition* ItemDefinition, int32 NumberToSpawn, const struct FVector& Position, const struct FVector& Direction, int32 OverrideMaxStackCount, bool bToss, bool bRandomRotation, bool bBlockedFromAutoPickup, int32 PickupInstigatorHandle, EFortPickupSourceTypeFlag SourceType, EFortPickupSpawnSource Source, class AFortPlayerController* OptionalOwnerPC, bool bPickupOnlyRelevantToOwner)
{
	return K2_SpawnPickupInWorldWithClass(WorldContextObject, ItemDefinition, nullptr, NumberToSpawn, Position, Direction, OverrideMaxStackCount, bToss, bRandomRotation, bBlockedFromAutoPickup, PickupInstigatorHandle, SourceType, Source, OptionalOwnerPC, bPickupOnlyRelevantToOwner);
}

AFortPickup* FortKismetLibrary::K2_SpawnPickupInWorldWithClass(class UObject* WorldContextObject, class UFortWorldItemDefinition* ItemDefinition, TSubclassOf<class AFortPickup> PickupClass, int32 NumberToSpawn, const struct FVector& Position, const struct FVector& Direction, int32 OverrideMaxStackCount, bool bToss, bool bRandomRotation, bool bBlockedFromAutoPickup, int32 PickupInstigatorHandle, EFortPickupSourceTypeFlag SourceType, EFortPickupSpawnSource Source, class AFortPlayerController* OptionalOwnerPC, bool bPickupOnlyRelevantToOwner)
{
	if (ItemDefinition == NULL || NumberToSpawn <= 0)
		return NULL;

	FFortItemEntry ItemEntry(ItemDefinition, NumberToSpawn, 1);

	if (UFortWeaponItemDefinition* WeaponDefinition = Cast<UFortWeaponItemDefinition>(ItemDefinition))
	{
		if (FFortBaseWeaponStats* WeaponStats = FindRow<FFortBaseWeaponStats>(WeaponDefinition->WeaponStatHandle.DataTable, WeaponDefinition->WeaponStatHandle.RowName))
			ItemEntry.LoadedAmmo = WeaponStats->ClipSize;
	}

	FFortCreatePickupData CreatePickupData{};
	CreatePickupData.World = GWorld;
	CreatePickupData.ItemEntry = ItemEntry;
	CreatePickupData.SpawnLocation = Position;
	CreatePickupData.SpawnRotation = {};
	CreatePickupData.PlayerController = nullptr;
	CreatePickupData.OverrideClass = PickupClass.Get();
	CreatePickupData.OwnerContainer = nullptr;
	CreatePickupData.bRandomRotation = bRandomRotation;
	CreatePickupData.PickupSourceTypeFlags = SourceType;

	AFortPickup* Pickup = AFortPickup::CreateFromData(CreatePickupData);

	if (Pickup != NULL)
	{
		Pickup->PickupSpawnSource = Source;
		Pickup->TossPickup(Position, nullptr, OverrideMaxStackCount, bToss, true, SourceType, Source);
		Pickup->bCombinePickupsWhenTossCompletes = true;
	}

	return Pickup;
}

bool FortKismetLibrary::SpawnInstancedPickupInWorld(class UObject* WorldContextObject, class UFortWorldItemDefinition* ItemDefinition, int32 NumberToSpawn, const struct FVector& Position, const struct FVector& Direction, int32 OverrideMaxStackCount, bool bToss, bool bRandomRotation, bool bBlockedFromAutoPickup)
{
	return K2_SpawnPickupInWorldWithClass(WorldContextObject, ItemDefinition, nullptr, NumberToSpawn, Position, Direction, OverrideMaxStackCount,
		bToss, bRandomRotation, bBlockedFromAutoPickup, -1, EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::Unset, nullptr, false) != NULL;
}

void FortKismetLibrary::Init()
{
	Hooking::Rel32Swap(ImageBase + 0x2F90300, PickLootDrops);
	Hooking::Rel32Swap(ImageBase + 0x2F8BE9C, GiveItemToInventoryOwner);
	Hooking::Rel32Swap(ImageBase + 0x2F8DA6F, K2_GiveItemToPlayer);
	Hooking::Rel32Swap(ImageBase + 0x2F81CC1, ChangeTeam);
	Hooking::Rel32Swap(ImageBase + 0x2F8DE6D, K2_RemoveItemFromAllPlayers);
	Hooking::Rel32Swap(ImageBase + 0x2F8D93D, K2_GiveBuildingResource);
	Hooking::Rel32Swap(ImageBase + 0x2F8E692, K2_SpawnPickupInWorld);
	Hooking::Rel32Swap(ImageBase + 0x2F8EAA2, K2_SpawnPickupInWorldWithClass);
	Hooking::Rel32Swap(ImageBase + 0x2F94096, SpawnInstancedPickupInWorld);
}
