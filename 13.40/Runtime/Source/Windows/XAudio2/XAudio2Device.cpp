#include "pch.h"
#include "Core.h"
#include "Windows/XAudio2/XAudio2Device.h"

DEFINE_INIT(XAudio2Device)

bool XAudio2Device::InitializeHardware(void* _this)
{
	Get<uint8>(_this, 0x3A2) &= ~0x04;
	Set<bool>(_this, 0x858, false);
	return false;
}

void XAudio2Device::Init()
{
	Hooking::Hook(ImageBase + 0x4FE8880, InitializeHardware);
}
