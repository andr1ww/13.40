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

	if (_this->GameInstance->LocalPlayers.Num() > 0)
		_this->GameInstance->RemoveLocalPlayer(_this->GameInstance->LocalPlayers[0]);

	if (UFortGameInstance* GameInstance = Cast<UFortGameInstance>(_this->GameInstance))
	{
		if (GameInstance->SeasonalEventManager)
			GameInstance->SeasonalEventManager->RefreshCalendar();
		else
			UE_LOG(LogFort, Warning, TEXT("GameEngine::Start: No SeasonalEventManager!!!!"));
	}

	StartOG(_this);

	UKismetSystemLibrary::CollectGarbage();
}

void GameEngine::Tick(UGameEngine* _this, float DeltaSeconds, bool bIdleMode)
{
	double& LastTimeLogsFlushed = Get<double>(_this, 0xE20);

	if (FPlatformTime::Seconds() - LastTimeLogsFlushed > static_cast<double>(_this->ServerFlushLogInterval))
	{
		GLog->Flush();
		LastTimeLogsFlushed = FPlatformTime::Seconds();
	}

	TickOG(_this, DeltaSeconds, bIdleMode);
}

void GameEngine::Init()
{
	Hooking::Hook(ImageBase + 0x499FAF0, Start, StartOG);
	Hooking::Hook(ImageBase + 0x49A0410, Tick, TickOG);
} 