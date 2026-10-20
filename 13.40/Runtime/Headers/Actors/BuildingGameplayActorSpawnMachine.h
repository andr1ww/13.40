#pragma once
#include "pch.h"

namespace BuildingGameplayActorSpawnMachine
{
	DefineHook(void, BeginPlay, ABuildingGameplayActorSpawnMachine* _this);
	DefineHook(void, OnGameStateLoaded, ABuildingGameplayActorSpawnMachine* _this, AGameStateBase* GameStateBase);
	void HandleResurrectedPawnSpawned(ABuildingGameplayActorSpawnMachine* _this, AFortPlayerControllerAthena* PlayerController);
	void HandleOnResurrectTimerComplete(ABuildingGameplayActorSpawnMachine* _this);

	void Init();
}
