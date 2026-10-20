#pragma once
#include "pch.h"

namespace FortKismetLibrary
{
	bool PickLootDrops(UObject* WorldContextObject, TArray<FFortItemEntry>& OutLootToDrop, FName TierGroupName, int32 WorldLevel, int32 ForcedLootTier);
	void GiveItemToInventoryOwner(TScriptInterface<class IFortInventoryOwnerInterface> InventoryOwner, const class UFortWorldItemDefinition* ItemDefinition, int32 NumberToGive, bool bNotifyPlayer, int32 ItemLevel, int32 PickupInstigatorHandle);
	void K2_GiveItemToAllPlayers(class UObject* WorldContextObject, class UFortWorldItemDefinition* ItemDefinition, int32 NumberToGive, bool bNotifyPlayer);
	void ChangeTeam(AActor* PlayerToSwitch, AActor* Instigator, uint8 NewTeam, FGameplayTagContainer ChangeTeamTags);
	void K2_GiveItemToPlayer(class AFortPlayerController* PlayerController, const class UFortWorldItemDefinition* ItemDefinition, int32 NumberToGive, bool bNotifyPlayer);
	void K2_RemoveItemFromAllPlayers(class UObject* WorldContextObject, class UFortWorldItemDefinition* ItemDefinition, int32 AmountToRemove);
	void K2_GiveBuildingResource(class AFortPlayerController* Controller, const EFortResourceType ResourceType, const int32 ResourceAmount);
	class AFortPickup* K2_SpawnPickupInWorld(class UObject* WorldContextObject, class UFortWorldItemDefinition* ItemDefinition, int32 NumberToSpawn, const struct FVector& Position, const struct FVector& Direction, int32 OverrideMaxStackCount, bool bToss, bool bRandomRotation, bool bBlockedFromAutoPickup, int32 PickupInstigatorHandle, EFortPickupSourceTypeFlag SourceType, EFortPickupSpawnSource Source, class AFortPlayerController* OptionalOwnerPC, bool bPickupOnlyRelevantToOwner);
	class AFortPickup* K2_SpawnPickupInWorldWithClass(class UObject* WorldContextObject, class UFortWorldItemDefinition* ItemDefinition, TSubclassOf<class AFortPickup> PickupClass, int32 NumberToSpawn, const struct FVector& Position, const struct FVector& Direction, int32 OverrideMaxStackCount, bool bToss, bool bRandomRotation, bool bBlockedFromAutoPickup, int32 PickupInstigatorHandle, EFortPickupSourceTypeFlag SourceType, EFortPickupSpawnSource Source, class AFortPlayerController* OptionalOwnerPC, bool bPickupOnlyRelevantToOwner);
	bool SpawnInstancedPickupInWorld(class UObject* WorldContextObject, class UFortWorldItemDefinition* ItemDefinition, int32 NumberToSpawn, const struct FVector& Position, const struct FVector& Direction, int32 OverrideMaxStackCount, bool bToss, bool bRandomRotation, bool bBlockedFromAutoPickup);

	void Init();
}
