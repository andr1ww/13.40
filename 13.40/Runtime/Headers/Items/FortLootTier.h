#pragma once
#include "pch.h"

struct FNameLess
{
	bool operator()(const FName& A, const FName& B) const
	{
		if (A.ComparisonIndex != B.ComparisonIndex)
			return A.ComparisonIndex < B.ComparisonIndex;

		return A.Number < B.Number;
	}
};

typedef std::map<FName, FFortLootTierData*, FNameLess> FLootTierDataMap;

struct FFortLootTierDataCheck
{
	FName                                         TierGroup;
	int32                                         LootTier;
	int32                                         WorldLevel;
};

FFortLootTierData* FindLootTierDataRow(UFortGameData* GameData, FName LootTierKey);

class UFortLootTier
{
public:
	static bool IsValidLootTierData(const FFortLootTierDataCheck& Check, FFortLootTierData* LootTierData);
	static void CollectLootTierDatas(FLootTierDataMap& OutLootTierDatas, float& OutWeight, const TArray<UDataTable*>& LootTierDataTables, const FFortLootTierDataCheck& Check);
	static bool ChooseLootTierKeyAndLootTierByWeight(FName& OutLootTierKey, int32* OutLootTier, float TotalWeight, FLootTierDataMap& LootTierDatas, FReservedRandomValues* ReservedRandomValues);
	static bool PickLootTierKeyAndLootTierFromTierGroup(FName& OutLootTierKey, int32* OutLootTier, FName TierGroup, int32 WorldLevel, FReservedRandomValues* ReservedRandomValues, int32 ForcedLootTier);
	static bool ProcessLootDrops(FFortLootTierData* LootTierData, int32 NumberOfLootDrops, std::map<int32, int32>& OutLootMap, FReservedRandomValues* ReservedRandomValues);
};
