#include "pch.h"
#include "Core.h"
#include "AI/FortAIGoalManager.h"
DEFINE_ENGINE_INIT(FortAIGoalManager)

AFortAIGoalManager* FortAIGoalManager::GetCurrent(UObject* WorldContextObject)
{
	AFortGameModeAthena* GameMode = Cast<AFortGameModeAthena>(GWorld->AuthorityGameMode);
	return GameMode ? GameMode->AIGoalManager : nullptr;
}

void FortAIGoalManager::Init()
{
	for (uintptr_t Offset : { 0x1B7C1CC, 0x1B7C217, 0x1B7C270, 0x1B86E59, 0x1B89EB4, 0x1B89EFF, 0x1B8BE0D, 0x1B8ED2E, 0x1B8F913, 0x1BAFEA6, 0x1BB01F6, 0x1BB64B7, 0x1BCF9D6, 0x1BCFBD6, 0x1BD0471, 0x1BE6751, 0x1BEC0A5, 0x1BEDA40, 0x1BEF06E, 0x1BF0562, 0x1BF2F56, 0x1C053CB, 0x1C05D82, 0x1C0630E, 0x1C07211, 0x1C0748F, 0x1C07D5D, 0x1C0A2EF, 0x1C0ACB4, 0x1C0B0A2, 0x1C0B476, 0x1C0C323, 0x1C0CD5E, 0x1C0FB70, 0x1C220B3, 0x1C33D1D, 0x1C404CD, 0x29199C8, 0x2F84798 })
		Hooking::Rel32Swap(ImageBase + Offset, GetCurrent);
}
