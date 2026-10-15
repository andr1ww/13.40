#include "pch.h"
#include "Core.h"
#include "Engine/UserInterfaceSettings.h"

DEFINE_INIT(UserInterfaceSettings)

void UserInterfaceSettings::Init()
{
	Hooking::PatchBytes(ImageBase + 0x4DEB813, { 0x0A, 0x91, 0x31, 0x02, 0x00, 0x00, 0x90 });
	Hooking::PatchBytes(ImageBase + 0x4DEB81D, { 0x75, 0x08 });
}
