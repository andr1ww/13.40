#pragma once
#include "pch.h"

namespace FortGameMode
{
	DefineHook(void, FinishWorldInitialization, AFortGameModeAthena* _this, AActor* a2);
	DefineHook(void, HandleStartingNewPlayer, AFortGameModeAthena* _this, AFortPlayerControllerAthena* a2);
	DefineHook(bool, ReadyToStartMatch, AFortGameModeAthena* _this);
	DefineHook(void, PlaceBotOnTeam, AFortGameModeAthena* _this, AFortPlayerStateAthena* PlayerState, AFortTeamInfo* Team, uint8 SquadId);
	DefineHook(bool, StartAircraftPhase, AFortGameModeAthena* _this, bool bGoStraightToSafeZone);

	void InitWorldManager(AFortGameModeAthena* _this, AFortWorldManager* WorldManager);
	void SetupPlaylist(AFortGameModeAthena* _this);

	void Init();
}