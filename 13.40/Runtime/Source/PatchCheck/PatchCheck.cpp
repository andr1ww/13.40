#include "pch.h"
#include "Core.h"
#include "PatchCheck/PatchCheck.h"

DEFINE_INIT(PatchCheck)

void PatchCheck::Init()
{
	Hooking::PatchBytes(ImageBase + 0xB48F86, { 0xEB, 0x36 });
}
