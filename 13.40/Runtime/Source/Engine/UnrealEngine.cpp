#include "pch.h"
#include "Core.h"
#include "Engine/UnrealEngine.h"

DEFINE_INIT(UnrealEngine)

float UnrealEngine::GetTimeBetweenGarbageCollectionPasses(UEngine* _this)
{
	float TimeBetweenGC = GetTimeBetweenGarbageCollectionPassesOG(_this);

	if (!GWorld || !GWorld->NetDriver || GWorld->NetDriver->ClientConnections.Num() == 0)
		TimeBetweenGC *= GTimeBetweenPurgingPendingKillObjectsOnIdleServerMultiplier;

	return TimeBetweenGC;
}

void UnrealEngine::Init()
{
	Hooking::Hook(ImageBase + 0x4DBFE80, GetTimeBetweenGarbageCollectionPasses, GetTimeBetweenGarbageCollectionPassesOG);
	Hooking::PatchBytes(ImageBase + 0x4DC2E1A, { 0xE9, 0xFA, 0x00, 0x00, 0x00, 0x90 });
	Hooking::PatchBytes(ImageBase + 0x4DC363D, { 0xEB, 0x0C });
	Hooking::PatchBytes(ImageBase + 0x4DD2D0A, { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
	Hooking::PatchBytes(ImageBase + 0x4DD2D18, { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
	Hooking::PatchBytes(ImageBase + 0x4DD2D25, { 0xEB, 0x58, 0x90, 0x90 });
	Hooking::PatchBytes(ImageBase + 0x4DC6C49, { 0xE9, 0xA6, 0x03, 0x00, 0x00 });
	Hooking::PatchBytes(ImageBase + 0x4DC510F, { 0xE9, 0xD8, 0x06, 0x00, 0x00, 0x90 });
	Hooking::PatchBytes(ImageBase + 0x4DC4C18, { 0xE9, 0xEF, 0x03, 0x00, 0x00, 0x90 });
	Hooking::RetFalse(ImageBase + 0x4DC75D0);
	Hooking::Null(ImageBase + 0x4DBD170);
	Hooking::PatchBytes(ImageBase + 0x4DC93A0, { 0x0F, 0x1F, 0x44, 0x00, 0x00 });
}
