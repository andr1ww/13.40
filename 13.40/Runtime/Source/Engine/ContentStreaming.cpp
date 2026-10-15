#include "pch.h"
#include "Core.h"
#include "Engine/ContentStreaming.h"

DEFINE_INIT(ContentStreaming)

void ContentStreaming::Init()
{
	Hooking::PatchBytes(ImageBase + 0x49084E2, { 0x90, 0x90 });
}
