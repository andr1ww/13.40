#include "pch.h"
#include "Core.h"
#include "Launch/LaunchEngineLoop.h"

DEFINE_INIT(LaunchEngineLoop)

void LaunchEngineLoop::Init()
{
	Hooking::RetTrue(ImageBase + 0x923AA0);
	Hooking::PatchBytes(ImageBase + 0x92A1BD, { 0xEB, 0x2C });
	Hooking::PatchBytes(ImageBase + 0x92A3ED, { 0xEB, 0x05 });
	Hooking::PatchBytes(ImageBase + 0x92A6E7, { 0xEB, 0x25 });
	Hooking::PatchBytes(ImageBase + 0x92AC00, { 0x0F, 0x1F, 0x44, 0x00, 0x00 });
	Hooking::PatchBytes(ImageBase + 0x92AE73, { 0xEB, 0x3E });
	Hooking::PatchBytes(ImageBase + 0x9287D6, { 0x0F, 0x1F, 0x44, 0x00, 0x00 });
	Hooking::PatchBytes(ImageBase + 0x92A121, { 0xEB, 0x2B });
	Hooking::PatchBytes(ImageBase + 0x92504C, { 0x0F, 0x1F, 0x44, 0x00, 0x00 });
	Hooking::PatchBytes(ImageBase + 0x925215, { 0xEB, 0x28 });
}
