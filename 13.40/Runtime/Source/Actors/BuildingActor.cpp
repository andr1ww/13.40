#include "pch.h"
#include "Core.h"
#include "Actors/BuildingActor.h"
#include "Items/FortLootPackage.h"
#include "Items/FortLootTier.h"
DEFINE_ENGINE_INIT(BuildingActor)

AFortPickup* BuildingActor::GenerateAmmoPickupForWeaponItemDefinitionFromContainer(UWorld* World, UFortWeaponItemDefinition* WeaponItemDefinition, ABuildingContainer* Container, EFortPickupSourceTypeFlag SourceTypeFlag, EFortPickupSpawnSource SpawnSource)
{
	if (!World || !WeaponItemDefinition || !Container)
		return NULL;

	UFortWeaponPickupSpawnAmmoData* SpawnAmmoData = UFortWeaponPickupSpawnAmmoData::Get();

	if (!SpawnAmmoData)
		return NULL;

	UFortAmmoItemDefinition* AmmoItemDefinition = Cast<UFortAmmoItemDefinition>(WeaponItemDefinition->GetAmmoWorldItemDefinition_BP());

	if (!AmmoItemDefinition)
		return NULL;

	FGameplayTagContainer WeaponTags = GetOwnedGameplayTags(WeaponItemDefinition);
	FGameplayTagContainer SourceTags = GetOwnedGameplayTags(Container);

	FVector Location = Container->K2_GetActorLocation() + Container->GetActorForwardVector() * Container->LootSpawnLocation_Athena.X
		+ Container->GetActorRightVector() * Container->LootSpawnLocation_Athena.Y
		+ Container->GetActorUpVector() * Container->LootSpawnLocation_Athena.Z;

	AFortPickup* Pickup = SpawnAmmoData->CreateAmmoPickup(World, AmmoItemDefinition, WeaponTags, SourceTags, Location, SourceTypeFlag, SpawnSource, Container);

	FreeGameplayTags(WeaponTags);
	FreeGameplayTags(SourceTags);

	if (!Pickup)
		return NULL;

	const int32 Step = min((int32)((float)rand() * (11.0f / 32767.0f)), 10);

	UFortKismetLibrary::TossPickupFromContainer(World, Container, Pickup, 10, Step, Container->LootTossConeHalfAngle_Athena, Container->LootTossDirection_Athena, Container->LootTossSpeed_Athena, Container->bForceHidePickupMinimapIndicator);

	return Pickup;
}

bool BuildingActor::SpawnLoot(ABuildingContainer* _this, AFortPlayerPawn* Pawn, EFortPickupSourceTypeFlag InSourceTypeFlag, EFortPickupSpawnSource InSpawnSource, bool bFromDestruction)
{
	if (_this->bAlreadySearched || _this->Role < ENetRole::ROLE_Authority || !GWorld)
		return false;

	AFortGameState* GameState = Cast<AFortGameState>(GWorld->GameState);

	if (!GameState)
		return false;

	static FName Loot_Treasure = FName(L"Loot_Treasure");
	static FName Loot_Ammo = FName(L"Loot_Ammo");
	static FName Loot_AthenaTreasure = FName(L"Loot_AthenaTreasure");
	static FName Loot_AthenaAmmoLarge = FName(L"Loot_AthenaAmmoLarge");

	if (_this->SearchLootTierGroup == Loot_Treasure)
		_this->SearchLootTierGroup = Loot_AthenaTreasure;
	else if (_this->SearchLootTierGroup == Loot_Ammo)
		_this->SearchLootTierGroup = Loot_AthenaAmmoLarge;

	FName LootTierKey;
	int32 LootTier = -1;

	if (!UFortLootTier::PickLootTierKeyAndLootTierFromTierGroup(LootTierKey, &LootTier, _this->SearchLootTierGroup, GameState->WorldLevel, NULL, -1))
		return false;

	IAbilitySystemInterface* AbilitySystemInterface = GetInterfaceAddress<IAbilitySystemInterface>(Pawn);

	TArray<FFortItemEntry> LootDrops{};

	if (!UFortLootPackage::PickLootDrops(LootDrops, GameState->WorldLevel, LootTierKey, AbilitySystemInterface, NULL, NULL, true, false))
	{
		LootDrops.Free();
		return false;
	}

	FVector Origin = _this->K2_GetActorLocation() + _this->GetActorForwardVector() * _this->LootSpawnLocation_Athena.X
		+ _this->GetActorRightVector() * _this->LootSpawnLocation_Athena.Y
		+ _this->GetActorUpVector() * _this->LootSpawnLocation_Athena.Z;

	for (int32 i = 0; i < LootDrops.Num(); i++)
	{
		FFortItemEntry& LootDrop = LootDrops[i];

		if (LootDrop.Count <= 0)
			continue;

		FFortCreatePickupData CreatePickupData{};
		CreatePickupData.World = GWorld;
		CreatePickupData.ItemEntry = LootDrop;
		CreatePickupData.SpawnLocation = Origin;
		CreatePickupData.OwnerContainer = _this;
		CreatePickupData.PickupSourceTypeFlags = InSourceTypeFlag;
		CreatePickupData.PickupSpawnSource = InSpawnSource;
		CreatePickupData.bRandomRotation = true;

		AFortPickup* Pickup = AFortPickup::CreateFromData(CreatePickupData);

		if (Pickup == NULL)
			continue;

		Pickup->PickupSpawnSource = InSpawnSource;

		UFortKismetLibrary::TossPickupFromContainer(GWorld, _this, Pickup, LootDrops.Num(), i, _this->LootTossConeHalfAngle_Athena, _this->LootTossDirection_Athena, _this->LootTossSpeed_Athena, _this->bForceHidePickupMinimapIndicator);

		if (UFortWeaponItemDefinition* WeaponItemDefinition = Cast<UFortWeaponItemDefinition>(LootDrop.ItemDefinition))
		{
			UFortWorldItemDefinition* AmmoItemDefinition = WeaponItemDefinition->GetAmmoWorldItemDefinition_BP();

			bool bLootHasAmmo = false;

			for (int32 j = 0; j < LootDrops.Num() && !bLootHasAmmo; j++)
				bLootHasAmmo = AmmoItemDefinition && LootDrops[j].ItemDefinition == AmmoItemDefinition;

			if (!bLootHasAmmo)
				GenerateAmmoPickupForWeaponItemDefinitionFromContainer(GWorld, WeaponItemDefinition, _this, InSourceTypeFlag, InSpawnSource);
		}
	}

	LootDrops.Free();

	_this->bAlreadySearched = true;
	_this->OnRep_bAlreadySearched();
	_this->OnSetSearched();

	_this->SearchBounceData.SearchAnimationCount++;
	_this->ForceNetUpdate();
	_this->BounceContainer();

	return true;
}

bool BuildingActor::AttemptSpawnResources(ABuildingSMActor* _this, AFortPlayerPawn* InstigatorPawn, float ActualDamageDealt, bool bJustHitWeakspot)
{
	if (!InstigatorPawn || ActualDamageDealt <= 0.f || _this->Role != ENetRole::ROLE_Authority)
		return false;

	AFortPlayerController* PlayerController = Cast<AFortPlayerController>(InstigatorPawn->Controller);

	if (!PlayerController)
		return false;

	AFortGameState* GameState = Cast<AFortGameState>(GWorld->GameState);
	UFortResourceItemDefinition* ResourceItemDefinition = UFortGameData::GetGameData()->GetResourceItemDefinition(_this->ResourceType);

	int32 ResourceCount = 0;
	float ResourcesPerHealth = 0.f;

	if (ResourceItemDefinition)
	{
		if (_this->MaxResourcesToSpawn < 0)
			_this->MaxResourcesToSpawn = (int32)_this->DetermineMaxResourcesToSpawn(true);

		ResourcesPerHealth = _this->MaxResourcesToSpawn / _this->GetMaxHealth();

		float ResourcesToSpawn = ResourcesPerHealth * ActualDamageDealt;

		if (UFortPlayerAttrSet* AttrSet = GetPlayerAttrSet(InstigatorPawn->AbilitySystemComponent))
		{
			if (AttrSet->ResourceHarvesting.CurrentValue > 0.f)
				ResourcesToSpawn *= AttrSet->ResourceHarvesting.CurrentValue;
		}

		ResourcesToSpawn += _this->UndistributedResources;

		ResourceCount = (int32)ResourcesToSpawn;
		_this->UndistributedResources = ResourcesToSpawn - (float)ResourceCount;
	}

	const bool bDestroyed = _this->GetHealth() <= 0.f;

	if (bDestroyed)
	{
		if (ResourceItemDefinition && ResourcesPerHealth != 0.f && ResourceCount < 1)
			ResourceCount = 1;

		if (_this->DestructionLootTierKey.ComparisonIndex != 0 && GameState)
		{
			IAbilitySystemInterface* AbilitySystemInterface = GetInterfaceAddress<IAbilitySystemInterface>(InstigatorPawn);

			TArray<FFortItemEntry> LootDrops{};

			if (UFortLootPackage::PickLootDrops(LootDrops, GameState->WorldLevel, _this->DestructionLootTierKey, AbilitySystemInterface, NULL, &_this->StaticGameplayTags, true, false))
			{
				for (int32 i = 0; i < LootDrops.Num(); i++)
				{
					FFortItemEntry& LootDrop = LootDrops[i];

					if (!Cast<UFortWorldItemDefinition>(LootDrop.ItemDefinition))
						continue;

					FFortCreatePickupData CreateLootPickupData{};
					CreateLootPickupData.World = GWorld;
					CreateLootPickupData.ItemEntry = LootDrop;
					CreateLootPickupData.SpawnLocation = _this->K2_GetActorLocation() + UKismetMathLibrary::RandomUnitVector() * 200.f;
					CreateLootPickupData.PickupSourceTypeFlags = EFortPickupSourceTypeFlag::Destruction;
					CreateLootPickupData.bRandomRotation = true;

					if (AFortPickup* LootPickup = AFortPickup::CreateFromData(CreateLootPickupData))
						LootPickup->SetPickupTarget(InstigatorPawn, 0.3f * (float)(i + 1), FVector{ 0.f, 0.f, 1.f });
				}
			}

			LootDrops.Free();
		}
	}

	if (ResourceCount <= 0)
		return false;

	AFortInventory* Inventory = PlayerController->WorldInventory;
	UFortWorldItem* ExistingItem = Inventory ? Inventory->FindExistingItemForDefinition(ResourceItemDefinition, false) : NULL;

	int32 MaxStackSize = max(1, AFortInventory::GetItemMaxStackSize(PlayerController, ResourceItemDefinition));
	int32 CurrentCount = ExistingItem ? ExistingItem->ItemEntry.Count : 0;
	int32 CountToGive = Inventory ? std::clamp(MaxStackSize - CurrentCount, 0, ResourceCount) : 0;

	if (CountToGive > 0)
	{
		if (ExistingItem)
		{
			ExistingItem->ItemEntry.SetCount(CurrentCount + CountToGive);
			Inventory->SetItemRequiresUpdate(&ExistingItem->ItemEntry);
		}
		else if (!Inventory->GiveItemToInventoryOwner(FFortItemEntry(ResourceItemDefinition, CountToGive)))
		{
			CountToGive = 0;
		}
	}

	if (CountToGive > 0)
		PlayerController->ClientReportDamagedResourceBuilding(_this, _this->ResourceType, CountToGive, bDestroyed, bJustHitWeakspot);

	if (ResourceCount > CountToGive)
	{
		FVector SpawnLocation = InstigatorPawn->K2_GetActorLocation() + UKismetMathLibrary::RandomUnitVector() * 200.f;

		FFortCreatePickupData CreatePickupData{};
		CreatePickupData.World = GWorld;
		CreatePickupData.ItemEntry = FFortItemEntry(ResourceItemDefinition, ResourceCount - CountToGive);
		CreatePickupData.SpawnLocation = SpawnLocation;
		CreatePickupData.PickupSourceTypeFlags = EFortPickupSourceTypeFlag::Destruction;
		CreatePickupData.bRandomRotation = true;

		if (AFortPickup* OverflowPickup = AFortPickup::CreateFromData(CreatePickupData))
			OverflowPickup->TossPickup(SpawnLocation, InstigatorPawn, -1, true, true, EFortPickupSourceTypeFlag::Destruction, EFortPickupSpawnSource::Unset);
	}

	return CountToGive > 0;
}

void BuildingActor::Init()
{
	Hooking::Hook(ImageBase + 0x213E180, SpawnLoot, SpawnLootOG);
	Hooking::HookEvery<ABuildingSMActor>(0xB20 / 8, AttemptSpawnResources);
}
