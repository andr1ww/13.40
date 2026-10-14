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

	return StartOG(_this);
}

void GameEngine::Init()
{
	Hooking::Hook(ImageBase + 0x499FAF0, Start, StartOG);
} 