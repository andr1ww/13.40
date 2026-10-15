#include "pch.h"
#include "Core.h"
#include "Engine/GameUserSettings.h"

DEFINE_INIT(GameUserSettings)

void GameUserSettings::Init()
{
	Hooking::PatchBytes(ImageBase + 0x49AD4F9, { 0xEB, 0x24, 0x90 });
}
