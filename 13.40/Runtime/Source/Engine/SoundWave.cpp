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
	Hooking::Hook(ImageBase + 0x4D29960, GetNumChunks);
}
