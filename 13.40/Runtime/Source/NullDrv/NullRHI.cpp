#include "pch.h"
#include "Core.h"
#include "NullDrv/NullRHI.h"

DEFINE_INIT(NullRHI)

void NullRHI::Init()
{
	Hooking::PatchBytes(ImageBase + 0x3799B87, { 0xEB, 0x25 });
}
