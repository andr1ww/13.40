#include "pch.h"
#include "Core.h"
#include "Engine/WorldComposition.h"

DEFINE_INIT(WorldComposition)

void WorldComposition::Init()
{
	Hooking::PatchBytes(ImageBase + 0x4E2F869, { 0x90, 0xE9, 0xBB, 0x01, 0x00, 0x00 });
}
