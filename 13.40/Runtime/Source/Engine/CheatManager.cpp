#include "pch.h"
#include "Core.h"
#include "Engine/CheatManager.h"

DEFINE_INIT(CheatManager)

void CheatManager::SpawnServerStatReplicator(UCheatManager* _this)
{
	SpawnServerStatReplicatorOG(_this);

	AGameModeBase* GameMode = GWorld ? GWorld->AuthorityGameMode : nullptr;
	if (!GameMode || !GWorld->NetDriver)
		return;

	FActorSpawnParameters SpawnParameters{};
	SpawnParameters.Owner = Cast<APlayerController>(_this->Outer);
	SpawnParameters.Name = FName(L"ServerStatReplicatorInst");

	GameMode->ServerStatReplicator = static_cast<AServerStatReplicator*>(GWorld->SpawnActor(AServerStatReplicator::StaticClass(), FVector(), FRotator(), SpawnParameters));
	Set<bool>(GWorld->NetDriver, 0x2B4, true);
}

void CheatManager::Init()
{
	Hooking::Hook(ImageBase + 0x4838BC0, SpawnServerStatReplicator, SpawnServerStatReplicatorOG);
}
