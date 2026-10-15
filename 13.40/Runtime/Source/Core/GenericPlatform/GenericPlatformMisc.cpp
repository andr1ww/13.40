#include "pch.h"
#include "Core.h"
#include "Core/GenericPlatform/GenericPlatformMisc.h"

DEFINE_INIT(GenericPlatformMisc)

void GenericPlatformMisc::Init()
{
	Hooking::PatchBytes(ImageBase + 0x31F560E, { 0x48, 0x8D, 0x0D, 0xFB, 0xA7, 0x21, 0x03 });
}
