#include "pch.h"
#include "Core.h"
#include "GameplayAbilities/GameplayAbilitiesModule.h"

DEFINE_INIT(GameplayAbilitiesModule)

void GameplayAbilitiesModule::Init()
{
	Hooking::PatchBytes(ImageBase + 0x975BB0, { 0x48, 0xC7, 0x41, 0x20, 0x00, 0x00, 0x00, 0x00, 0xC3 });
}
