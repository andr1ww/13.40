#include "pch.h"
#include "Core.h"
#include "SignificanceManager/SignificanceManager.h"

DEFINE_INIT(SignificanceManager)

void SignificanceManager::Init()
{
	Hooking::PatchBytes(ImageBase + 0x1371487, { 0x0F, 0x1F, 0x44, 0x00, 0x00 });
}
