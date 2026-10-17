#include "pch.h"
#include "Core.h"
#include "Athena/Modifiers/FortAthenaMutator_Bots.h"
DEFINE_ENGINE_INIT(FortAthenaMutator_Bots)

void FortAthenaMutator_Bots::BeginPlay(AFortAthenaMutator_Bots* _this)
{
	_this->HumanReservationCount = 1;

	BeginPlayOG(_this);

	AFortGameModeAthena* GameMode = Cast<AFortGameModeAthena>(GWorld->AuthorityGameMode);
	UFortServerBotManagerAthena* BotManager = GameMode ? GameMode->ServerBotManager : nullptr;

	if (BotManager && !BotManager->CachedBotMutator)
		BotManager->CachedBotMutator = _this;
}

void FortAthenaMutator_Bots::Init()
{
	GPhoebeEnable = true;
	GPhoebeMinimap = true;
	GPhoebeOverrideMMR = true;
	GPhoebeOverrideMMRValue = 2200;
	GPhoebeEnableWarmupBehavior = true;

	Hooking::Hook<AFortAthenaMutator_Bots>(0x328 / 8, BeginPlay, BeginPlayOG);
}
