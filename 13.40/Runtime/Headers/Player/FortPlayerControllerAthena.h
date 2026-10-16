#pragma once
#include "pch.h"

enum EPawnDestructionReason : int32
{
	Death = 0x0,
	Logout = 0x1,
};

namespace FortPlayerControllerAthena
{
	DefineHook(void, OnReadyToStartMatch, AFortPlayerControllerAthena* _this);
	DefineHook(void, ServerLoadingScreenDropped_Implementation, AFortPlayerControllerAthena* _this);
	void ServerAcknowledgePossession_Implementation(AFortPlayerControllerAthena* _this, APawn* P);
	void RemoveAllInventoryItems(AFortPlayerControllerAthena* _this);
	void DropItemsAsPickups(AFortPlayerControllerAthena* _this, TArray<UFortWorldItem*>& ItemsToDropViaPickup, AFortPawn* DestructionPawn, AFortPlayerController* InFortPlayerController, EFortPickupSpawnSource SpawnSource = EFortPickupSpawnSource::Unset);
	void DropItemsAsPickupsAsync(AFortPlayerControllerAthena* _this, TArray<UFortWorldItem*>& ItemsToDropViaPickup, AFortPawn* DestructionPawn);
	void DropAllItems(AFortPlayerControllerAthena* _this, const UFortItemDefinition* IgnoreItemDef, const UFortItemDefinition* AdditionalIgnoreItemDef, bool bIgnoreBuildingMaterials, bool bSpawnPickups);
	void DropSpecificItem(AFortPlayerControllerAthena* _this, const UFortItemDefinition* DropItemDef);
	void execDropSpecificItem(UObject* Context, FFrame& Stack, void* const Result);
	void TossSpecificItem(AFortPlayerControllerAthena* _this, const UFortItemDefinition* DropItemDef, const FFortPickupTossOverrideData& TossOverrideData);
	void DropItemsOnPawnDestruction(AFortPlayerControllerAthena* _this, EPawnDestructionReason DestructionReason, const FGameplayTagContainer* ContextualTags, AFortPawn* DestructionPawn);
	void ServerOnMaterialSelection_Implementation(AFortPlayerControllerAthena* _this, EFortResourceType NewResourceType, EFortResourceLevel NewResourceLevel);
	void ServerPlaySquadQuickChatMessage_Implementation(AFortPlayerControllerAthena* _this, FAthenaQuickChatActiveEntry& ChatEntry, FUniqueNetIdRepl& SenderID);
	void ServerSetInAircraft_Implementation(AFortPlayerStateAthena* _this, bool bNewInAircraft);
	void ServerEnableAnonymousMode_Implementation(AFortPlayerControllerAthena* _this);
	void ServerEnableAnonymousCharacterMode_Implementation(AFortPlayerControllerAthena* _this);
	bool ForceEquipValidWeapon(AFortPlayerControllerAthena* _this);

	void Init();
}
