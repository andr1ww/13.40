#include "pch.h"
#include "Core.h"
#include "Core/Windows/WindowsPlatformMisc.h"

DEFINE_INIT(WindowsPlatformMisc)

void WindowsPlatformMisc::Init()
{
	Hooking::PatchBytes(ImageBase + 0x332DC4A, { 0xEB, 0x37 });
	Hooking::Patch<int32>(ImageBase + 0x8700900, 4);
}
