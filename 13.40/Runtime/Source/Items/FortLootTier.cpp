#include "pch.h"
#include "Core.h"
#include "Items/FortLootTier.h"

void ChooseLootTierDatas(UFortGameData* GameData, TArray<UDataTable*>& OutTables)
{
	if (AFortGameMode* GameMode = Cast<AFortGameMode>(GWorld->AuthorityGameMode))
		GameMode->GetLootTierDataTables(&OutTables);

	if (OutTables.Num() == 0)
	{
		static UDataTable* Table = StaticLoadObject<UDataTable>(L"/Game/Items/Datatables/AthenaLootTierData_Client.AthenaLootTierData_Client");

		if (Table != NULL)
			OutTables.Add(Table);
	}
}

FFortLootTierData* FindLootTierDataRow(UFortGameData* GameData, FName LootTierKey)
{
	TArray<UDataTable*> Tables;
	ChooseLootTierDatas(GameData, Tables);

	FFortLootTierData* Result = NULL;

	for (int32 t = 0; t < Tables.Num() && Result == NULL; t++)
	{
		ForEachRow<FFortLootTierData>(Tables[t], [&](FName RowName, FFortLootTierData* Row)
			{
				if (Result == NULL && RowName == LootTierKey)
					Result = Row;
			});
	}

	Tables.Free();

	return Result;
}

bool UFortLootTier::IsValidLootTierData(const FFortLootTierDataCheck& Check, FFortLootTierData* LootTierData)
{
	if (LootTierData == NULL)
		return false;

	if (Check.TierGroup != LootTierData->TierGroup)
		return false;

	if (Check.LootTier != -1 && Check.LootTier != LootTierData->LootTier)
		return false;

	if (Check.WorldLevel != -1)
	{
		if (LootTierData->MaxWorldLevel != -1 && Check.WorldLevel > LootTierData->MaxWorldLevel)
			return false;

		if (LootTierData->MinWorldLevel != -1 && Check.WorldLevel < LootTierData->MinWorldLevel)
			return false;
	}

	return true;
}

void UFortLootTier::CollectLootTierDatas(FLootTierDataMap& OutLootTierDatas, float& OutWeight, const TArray<UDataTable*>& LootTierDataTables, const FFortLootTierDataCheck& Check)
{
	for (int32 t = 0; t < LootTierDataTables.Num(); t++)
	{
		ForEachRow<FFortLootTierData>(LootTierDataTables[t], [&](FName RowName, FFortLootTierData* Row)
			{
				if (UFortLootTier::IsValidLootTierData(Check, Row))
				{
					OutLootTierDatas.insert({ RowName, Row });
					OutWeight += Row->Weight;
				}
			});
	}
}

bool UFortLootTier::ChooseLootTierKeyAndLootTierByWeight(FName& OutLootTierKey, int32* OutLootTier, float TotalWeight, FLootTierDataMap& LootTierDatas, FReservedRandomValues* ReservedRandomValues)
{
	if (TotalWeight <= 0.0f)
		return false;

	float WeightedValue = FRand(ReservedRandomValues) * TotalWeight;

	for (auto& Entry : LootTierDatas)
	{
		FFortLootTierData* Row = Entry.second;

		if (Row == NULL)
			continue;

		if (WeightedValue <= Row->Weight)
		{
			if (OutLootTier != NULL)
				*OutLootTier = Row->LootTier;

			OutLootTierKey = Entry.first;
			return true;
		}

		WeightedValue -= Row->Weight;
	}

	return false;
}

bool UFortLootTier::PickLootTierKeyAndLootTierFromTierGroup(FName& OutLootTierKey, int32* OutLootTier, FName TierGroup, int32 WorldLevel, FReservedRandomValues* ReservedRandomValues, int32 ForcedLootTier)
{
	UFortGameData* GameData = UFortGameData::GetGameData();

	if (GameData == NULL)
		return false;

	FFortLootTierDataCheck Check{};
	Check.TierGroup = TierGroup;
	Check.WorldLevel = WorldLevel;
	Check.LootTier = -1;

	TArray<UDataTable*> Tables;
	ChooseLootTierDatas(GameData, Tables);

	if (Tables.Num() == 0)
	{
		Tables.Free();
		return false;
	}

	FLootTierDataMap LootTierDatas;
	float TotalWeight = 0.0f;

	UFortLootTier::CollectLootTierDatas(LootTierDatas, TotalWeight, Tables, Check);

	Tables.Free();

	if (ForcedLootTier != -1)
	{
		int32 MatchedTier = -1;

		for (int32 Tier = ForcedLootTier; Tier >= 0 && MatchedTier == -1; Tier--)
		{
			for (auto& Entry : LootTierDatas)
			{
				if (Entry.second != NULL && Entry.second->LootTier == Tier)
				{
					MatchedTier = Tier;
					break;
				}
			}
		}

		if (MatchedTier == -1)
			return false;

		TotalWeight = 0.0f;

		for (auto It = LootTierDatas.begin(); It != LootTierDatas.end(); )
		{
			if (It->second == NULL || It->second->LootTier != MatchedTier)
			{
				It = LootTierDatas.erase(It);
				continue;
			}

			TotalWeight += It->second->Weight;
			++It;
		}
	}

	return UFortLootTier::ChooseLootTierKeyAndLootTierByWeight(OutLootTierKey, OutLootTier, TotalWeight, LootTierDatas, ReservedRandomValues);
}

bool UFortLootTier::ProcessLootDrops(FFortLootTierData* LootTierData, int32 NumberOfLootDrops, std::map<int32, int32>& OutLootMap, FReservedRandomValues* ReservedRandomValues)
{
	if (LootTierData == NULL)
		return false;

	OutLootMap.clear();

	const int32 NumCategories = LootTierData->LootPackageCategoryMinArray.Num();

	if (NumCategories != LootTierData->LootPackageCategoryWeightArray.Num() || NumCategories != LootTierData->LootPackageCategoryMaxArray.Num())
		return false;

	int32 AllocatedDrops = 0;

	for (int32 i = 0; i < NumCategories; i++)
	{
		const int32 CategoryMin = LootTierData->LootPackageCategoryMinArray[i];

		if (CategoryMin > 0)
		{
			OutLootMap.emplace(i, CategoryMin);
			AllocatedDrops += CategoryMin;
		}
	}

	if (AllocatedDrops > NumberOfLootDrops)
	{
		OutLootMap.clear();
		return false;
	}

	int32 TotalCategoryWeight = 0;
	std::map<int32, int32> CategoryWeightMap;

	for (int32 i = 0; i < NumCategories; i++)
	{
		const int32 CategoryWeight = LootTierData->LootPackageCategoryWeightArray[i];
		const int32 CategoryMax = LootTierData->LootPackageCategoryMaxArray[i];

		if (CategoryWeight <= 0)
			continue;

		const int32 Current = OutLootMap.count(i) ? OutLootMap.at(i) : 0;

		if (CategoryMax < 0 || Current < CategoryMax)
		{
			TotalCategoryWeight += CategoryWeight;
			CategoryWeightMap.emplace(i, CategoryWeight);
		}
	}

	while (AllocatedDrops < NumberOfLootDrops && TotalCategoryWeight > 0)
	{
		int32 Selection = (int32)(FRand(ReservedRandomValues) * (float)TotalCategoryWeight);
		int32 SelectedCategory = -1;

		for (auto& Entry : CategoryWeightMap)
		{
			if (Selection <= Entry.second)
			{
				SelectedCategory = Entry.first;
				break;
			}

			Selection -= Entry.second;
		}

		if (SelectedCategory == -1)
			break;

		int32& NumDropsForCategory = OutLootMap[SelectedCategory];
		NumDropsForCategory++;
		AllocatedDrops++;

		const int32 CategoryMax = LootTierData->LootPackageCategoryMaxArray[SelectedCategory];

		if (CategoryMax >= 0 && NumDropsForCategory >= CategoryMax)
		{
			TotalCategoryWeight -= LootTierData->LootPackageCategoryWeightArray[SelectedCategory];
			CategoryWeightMap.erase(SelectedCategory);
		}
	}

	if (AllocatedDrops < NumberOfLootDrops)
	{
		OutLootMap.clear();
		return false;
	}

	return true;
}
