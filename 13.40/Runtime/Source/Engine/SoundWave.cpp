#include "pch.h"
#include "Core.h"
#include "Engine/SoundWave.h"

DEFINE_INIT(SoundWave)

uint32 SoundWave::GetNumChunks(USoundWave* _this)
{
	uint8* RunningPlatformData = Get<uint8*>(_this, 0x300);

	return RunningPlatformData ? Get<int32>(RunningPlatformData, 0x18) : 0;
}

void SoundWave::Init()
{
	Hooking::PatchBytes(ImageBase + 0x4D357FD, { 0xE9, 0xE3, 0x01, 0x00, 0x00, 0x90 });
	Hooking::RetFalse(ImageBase + 0x4D2B0C0);
	Hooking::RetNull(ImageBase + 0x4D27420);
	Hooking::Hook(ImageBase + 0x4D29960, GetNumChunks);
	Hooking::PatchBytes(ImageBase + 0x4D2ABC5, { 0xE9, 0xDA, 0x00, 0x00, 0x00, 0x90 });
	Hooking::Null(ImageBase + 0x4D34C10);
}
