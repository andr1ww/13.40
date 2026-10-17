#pragma once
#include "pch.h"

namespace FortAthenaAIBotController
{
	bool CreateInventory(IFortInventoryOwnerInterface* _this);

	DefineHook(UFortHero*, GetHero, AFortAthenaAIBotController* _this);
	DefineHook(void, InitializeInventory, IFortInventoryOwnerInterface* _this, const TArray<FItemAndCount>& ItemList);
	DefineHook(void, OnPossesedPawnDied, AFortAthenaAIBotController* _this, AActor* DamagedActor, float Damage, AController* InstigatedBy, AActor* DamageCauser, const FVector& HitLocation, UPrimitiveComponent* FHitComponent, FName BoneName, const FVector& Momentum);
	void DropItemsAsPickups(TArray<UFortWorldItem*>& ItemsToDropViaPickup, AFortPawn* DestructionPawn, AFortInventory* Inventory);
	void RemoveAllInventoryItems(AFortAthenaAIBotController* _this);

	void Init();
}
