#include "pch.h"
#include "Core.h"
#include "Core/Windows/WindowsPlatformCrashContext.h"

DEFINE_INIT(WindowsPlatformCrashContext)

void WindowsPlatformCrashContext::Init()
{
	Hooking::PatchBytes(ImageBase + 0x33236C7, { 0xB3, 0x01, 0x90 });
	Hooking::PatchBytes(ImageBase + 0x3323C7D, { 0x40, 0xB6, 0x01 });
}
