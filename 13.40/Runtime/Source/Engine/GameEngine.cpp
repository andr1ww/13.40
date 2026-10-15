#include "pch.h"
#include "Core.h"
#include "Engine/GameEngine.h"

DEFINE_INIT(GameEngine)

void GameEngine::Start(UGameEngine* _this)
{
	Set<bool>(ImageBase + 0x8D265E9, 0);
	Set<bool>(ImageBase + 0x8D265EA, 1);

	for (auto& Init : EngineInitters())
		Init();

	_this->GameInstance->LocalPlayers.Remove(0);

	if (UFortGameInstance* GameInstance = Cast<UFortGameInstance>(_this->GameInstance))
	{
		if (GameInstance->SeasonalEventManager)
			GameInstance->SeasonalEventManager->RefreshCalendar();
		else
			UE_LOG(LogFort, Warning, TEXT("GameEngine::Start: No SeasonalEventManager!!!!"));
	}

	return StartOG(_this);
}

void GameEngine::Init()
{
	Hooking::Hook(ImageBase + 0x499FAF0, Start, StartOG);
} 