#pragma once
#include "pch.h"

namespace BuildingActor
{
	AFortPickup* GenerateAmmoPickupForWeaponItemDefinitionFromContainer(UWorld* World, UFortWeaponItemDefinition* WeaponItemDefinition, ABuildingContainer* Container, EFortPickupSourceTypeFlag SourceTypeFlag, EFortPickupSpawnSource SpawnSource);
	DefineHook(bool, SpawnLoot, ABuildingContainer* _this, AFortPlayerPawn* Pawn, EFortPickupSourceTypeFlag InSourceTypeFlag, EFortPickupSpawnSource InSpawnSource, bool bFromDestruction);
	bool AttemptSpawnResources(ABuildingSMActor* _this, AFortPlayerPawn* InstigatorPawn, float ActualDamageDealt, bool bJustHitWeakspot);

	void Init();
}
