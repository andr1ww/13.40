#include "pch.h"
#include "Core.h"
#include "Core/Misc/CoreMisc.h"

DEFINE_INIT(CoreMisc)

void CoreMisc::Init()
{
	Hooking::RetTrue(ImageBase + 0x32DC130);
}
