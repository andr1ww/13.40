#include "pch.h"
#include "Core.h"
#include "Items/FortLootPackage.h"
#include "Items/FortLootTier.h"

void ChooseLootPackageDatas(UFortGameData* GameData, TArray<UDataTable*>& OutTables)
{
	if (AFortGameMode* GameMode = Cast<AFortGameMode>(GWorld->AuthorityGameMode))
		GameMode->GetLootPackageDataTables(&OutTables);

	if (OutTables.Num() == 0)
	{
		static UDataTable* Table = StaticLoadObject<UDataTable>(L"/Game/Items/Datatables/AthenaLootPackages_Client.AthenaLootPackages_Client");

		if (Table != NULL)
			OutTables.Add(Table);
	}
}

bool UFortLootPackage::IsValidLootPackage(const FFortLootPackageData* LootPackageData, const FFortLootPackageDataCheck& Check)
{
	if (LootPackageData == NULL)
		return false;

	if (LootPackageData->Weight <= 0.0f)
		return false;

	if (Check.LootPackageID != LootPackageData->LootPackageID)
		return false;

	if (Check.LootPackageCategory != -1 && LootPackageData->LootPackageCategory != Check.LootPackageCategory)
		return false;

	if (Check.WorldLevel != -1)
	{
		if (LootPackageData->MaxWorldLevel != -1 && Check.WorldLevel > LootPackageData->MaxWorldLevel)
			return false;

		if (LootPackageData->MinWorldLevel != -1 && Check.WorldLevel < LootPackageData->MinWorldLevel)
			return false;
	}

	return true;
}

void UFortLootPackage::CollectLootPackageDatas(TArray<FFortLootPackageData*>& OutLootPackageDatas, float& OutWeight, const TArray<UDataTable*>& LootPackageTables, const FFortLootPackageDataCheck& Check)
{
	for (int32 t = 0; t < LootPackageTables.Num(); t++)
	{
		ForEachRow<FFortLootPackageData>(LootPackageTables[t], [&](FName RowName, FFortLootPackageData* Row)
			{
				if (UFortLootPackage::IsValidLootPackage(Row, Check))
				{
					OutLootPackageDatas.Add(Row);
					OutWeight += Row->Weight;
				}
			});
	}
}

bool UFortLootPackage::PickLootDropsFromLootPackage(TArray<FFortItemEntry>& OutLootDrops, FName LootPackageID, int32 WorldLevel, int32 LootPackageCategory, IAbilitySystemInterface* AbilitySystemInterface, FReservedRandomValues* ReservedRandomValues, FGameplayTagContainer* RequiredTags, bool bAllowReRoll)
{
	UFortGameData* GameData = UFortGameData::GetGameData();

	if (GameData == NULL)
		return false;

	FFortLootPackageDataCheck Check{};
	Check.LootPackageID = LootPackageID;
	Check.LootPackageCategory = LootPackageCategory;
	Check.WorldLevel = WorldLevel;
	Check.GameplayTags = RequiredTags;

	TArray<UDataTable*> Tables;
	ChooseLootPackageDatas(GameData, Tables);

	TArray<FFortLootPackageData*> LootPackageDatas;
	float TotalWeight = 0.0f;

	UFortLootPackage::CollectLootPackageDatas(LootPackageDatas, TotalWeight, Tables, Check);

	Tables.Free();

	if (LootPackageDatas.Num() == 0 || TotalWeight <= 0.0f)
	{
		LootPackageDatas.Free();
		return false;
	}

	float WeightedRoll = FRand(ReservedRandomValues) * TotalWeight;
	FFortLootPackageData* Selected = NULL;

	for (int32 i = 0; i < LootPackageDatas.Num(); i++)
	{
		if (WeightedRoll <= LootPackageDatas[i]->Weight)
		{
			Selected = LootPackageDatas[i];
			break;
		}

		WeightedRoll -= LootPackageDatas[i]->Weight;
	}

	LootPackageDatas.Free();

	if (Selected == NULL)
		return false;

	if (Selected->LootPackageCall.Num() > 1)
	{
		for (int32 i = 0; i < Selected->Count; i++)
			UFortLootPackage::PickLootDropsFromLootPackage(OutLootDrops, FName(Selected->LootPackageCall.CStr()), WorldLevel, -1, AbilitySystemInterface, ReservedRandomValues, RequiredTags, false);

		return true;
	}

	UFortWorldItemDefinition* ItemDefinition = Cast<UFortWorldItemDefinition>(Selected->ItemDefinition.LoadSynchronous());

	if (ItemDefinition == NULL)
		return false;

	int32 CountMultiplier = 1;

	if (Selected->bAllowBonusDrops && AbilitySystemInterface != NULL)
	{
		UFortPlayerAttrSet* AttrSet = GetPlayerAttrSet(AbilitySystemInterface->GetAbilitySystemComponent());

		if (AttrSet != NULL)
		{
			const float DoubleItemLootChance = AttrSet->DoubleItemLootChance.CurrentValue;

			if (DoubleItemLootChance > 0.0f && DoubleItemLootChance >= FRand(ReservedRandomValues))
				CountMultiplier = 2;
		}
	}

	const int32 ItemLevel = UFortLootLevel::GetItemLevel(&ItemDefinition->LootLevelData, WorldLevel);

	const int32 LoadedAmmo = GetWeaponLoadedAmmo(Cast<UFortWeaponItemDefinition>(ItemDefinition), ItemLevel);

	int32 RemainingCount = CountMultiplier * Selected->Count;

	while (RemainingCount > 0)
	{
		const int32 MaxStackSize = AFortInventory::GetItemMaxStackSize(NULL, ItemDefinition);

		int32 ThisStack = RemainingCount;

		if (MaxStackSize > 0 && ThisStack > MaxStackSize)
			ThisStack = MaxStackSize;

		if (ThisStack <= 0)
			break;

		FFortItemEntry Entry(ItemDefinition, ThisStack, ItemLevel < 0 ? 0 : ItemLevel);

		Entry.Durability = 1.0f;
		Entry.LoadedAmmo = LoadedAmmo;
		Entry.GameplayAbilitySpecHandle.Handle = -1;

		OutLootDrops.Add(Entry);

		RemainingCount -= ThisStack;
	}

	if (bAllowReRoll && AbilitySystemInterface != NULL)
	{
		UFortPlayerAttrSet* AttrSet = GetPlayerAttrSet(AbilitySystemInterface->GetAbilitySystemComponent());

		if (AttrSet != NULL)
		{
			const float DoubleLootRollChance = AttrSet->DoubleLootRollChance.CurrentValue;

			if (DoubleLootRollChance > 0.0f && FRand(ReservedRandomValues) <= DoubleLootRollChance)
				UFortLootPackage::PickLootDropsFromLootPackage(OutLootDrops, LootPackageID, WorldLevel, LootPackageCategory, AbilitySystemInterface, ReservedRandomValues, RequiredTags, false);
		}
	}

	return true;
}

bool UFortLootPackage::PickLootDrops(TArray<FFortItemEntry>& OutLootToDrop, int32 WorldLevel, FName LootTierKey, IAbilitySystemInterface* AbilitySystemInterface, FReservedRandomValues* ReservedRandomValues, FGameplayTagContainer* StaticGameplayTags, bool bAllowReRoll, bool bUseLootPreviewPackage)
{
	UFortGameData* GameData = UFortGameData::GetGameData();

	if (GameData == NULL)
		return false;

	FFortLootTierData* LootTierData = FindLootTierDataRow(GameData, LootTierKey);

	if (LootTierData == NULL)
		return false;

	const int32 EntryCountOnEntry = OutLootToDrop.Num();

	const FName LootPackage = bUseLootPreviewPackage ? LootTierData->LootPreviewPackage : LootTierData->LootPackage;

	if (LootPackage.ComparisonIndex != 0 && LootTierData->NumLootPackageDrops > 0.0f)
	{
		const float NumLootPackageDrops = LootTierData->NumLootPackageDrops;

		int32 NumberOfLootDrops = 1;

		if (!bUseLootPreviewPackage && NumLootPackageDrops >= 1.0f)
		{
			NumberOfLootDrops = (int32)NumLootPackageDrops;

			if ((NumLootPackageDrops - (float)NumberOfLootDrops) >= FRand(ReservedRandomValues))
				NumberOfLootDrops++;
		}

		std::map<int32, int32> LootMap;

		if (bUseLootPreviewPackage)
			LootMap.insert({ 0, 1 });
		else
			UFortLootTier::ProcessLootDrops(LootTierData, NumberOfLootDrops, LootMap, ReservedRandomValues);

		for (auto& Entry : LootMap)
		{
			for (int32 i = 0; i < Entry.second; i++)
			{
				const bool bReRoll = !bUseLootPreviewPackage && bAllowReRoll && LootTierData->bAllowBonusLootDrops;

				UFortLootPackage::PickLootDropsFromLootPackage(OutLootToDrop, LootPackage, WorldLevel, Entry.first, AbilitySystemInterface, ReservedRandomValues, StaticGameplayTags, bReRoll);
			}
		}
	}

	for (int32 i = 0; i < OutLootToDrop.Num(); i++)
	{
		FFortItemEntry& Entry = OutLootToDrop[i];

		if (Entry.ItemDefinition == NULL || Entry.ItemDefinition->bAllowMultipleStacks)
			continue;

		for (int32 j = i + 1; j < OutLootToDrop.Num(); j++)
		{
			if (Entry.ItemDefinition != OutLootToDrop[j].ItemDefinition)
				continue;

			int32 NewCount = Entry.Count + OutLootToDrop[j].Count;

			if (NewCount < 0)
				NewCount = 0;

			Entry.Count = NewCount;
			Entry.SetToDirty();

			OutLootToDrop.Remove(j);
			--j;
		}
	}

	return OutLootToDrop.Num() > EntryCountOnEntry;
}
