#include "pch.h"
#include "Core.h"
#include "ResonanceAudio/ResonanceAudioModule.h"

DEFINE_INIT(ResonanceAudioModule)

void ResonanceAudioModule::Init()
{
	Hooking::PatchBytes(ImageBase + 0x1926E12, { 0xE9, 0xAE, 0x01, 0x00, 0x00, 0x90 });
}
