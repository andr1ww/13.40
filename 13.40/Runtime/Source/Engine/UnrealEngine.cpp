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
}
