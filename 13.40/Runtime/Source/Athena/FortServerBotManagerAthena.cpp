#include "pch.h"
#include "Core.h"
#include "Athena/FortServerBotManagerAthena.h"
DEFINE_ENGINE_INIT(FortServerBotManagerAthena)

UFortServerBotManagerAthena* FortServerBotManagerAthena::GetServerBotManager(UObject* WorldContextObject)
{
	AFortGameModeAthena* GameMode = Cast<AFortGameModeAthena>(GWorld->AuthorityGameMode);
	return GameMode ? GameMode->ServerBotManager : nullptr;
}

void FortServerBotManagerAthena::Init()
{
	Hooking::Rel32Swap(ImageBase + 0x1DFA4F5, GetServerBotManager);
	Hooking::Rel32Swap(ImageBase + 0x1DF7F52, GetServerBotManager);
}
