#pragma once
#include "pch.h"

struct FFortLootPackageDataCheck
{
	FName                                         LootPackageID;
	int32                                         LootPackageCategory;
	int32                                         WorldLevel;
	FGameplayTagContainer*                        GameplayTags;
};

class UFortLootPackage
{
public:
	static bool IsValidLootPackage(const FFortLootPackageData* LootPackageData, const FFortLootPackageDataCheck& Check);
	static void CollectLootPackageDatas(TArray<FFortLootPackageData*>& OutLootPackageDatas, float& OutWeight, const TArray<UDataTable*>& LootPackageTables, const FFortLootPackageDataCheck& Check);
	static bool PickLootDropsFromLootPackage(TArray<FFortItemEntry>& OutLootDrops, FName LootPackageID, int32 WorldLevel, int32 LootPackageCategory, IAbilitySystemInterface* AbilitySystemInterface, FReservedRandomValues* ReservedRandomValues, FGameplayTagContainer* RequiredTags, bool bAllowReRoll);
	static bool PickLootDrops(TArray<FFortItemEntry>& OutLootToDrop, int32 WorldLevel, FName LootTierKey, IAbilitySystemInterface* AbilitySystemInterface, FReservedRandomValues* ReservedRandomValues, FGameplayTagContainer* StaticGameplayTags, bool bAllowReRoll, bool bUseLootPreviewPackage);
};
