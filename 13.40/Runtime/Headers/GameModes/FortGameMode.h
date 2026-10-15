#pragma once
#include "pch.h"

namespace FortGameMode
{
	DefineHook(void, FinishWorldInitialization, AFortGameModeAthena* _this, AActor* a2);
	DefineHook(void, InitGameState, AFortGameModeAthena* _this);
	DefineHook(void, HandleStartingNewPlayer, AFortGameModeAthena* _this, AFortPlayerControllerAthena* a2);
	DefineHook(bool, ReadyToStartMatch, AFortGameModeAthena* _this);

	void InitWorldManager(AFortGameModeAthena* _this, AFortWorldManager* WorldManager);

	void Init();
}