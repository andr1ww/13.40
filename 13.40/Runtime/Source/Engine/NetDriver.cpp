#include "pch.h"
#include "Core.h"
#include "Engine/NetDriver.h"

DEFINE_ENGINE_INIT(NetDriver)

void NetDriver::TickFlush(UNetDriver* _this, float DeltaSeconds)
{
	if (_this->ReplicationDriver)
		_this->ReplicationDriver->ServerReplicateActors(DeltaSeconds);

	return TickFlushOG(_this, DeltaSeconds);
}

void NetDriver::Init()
{
	Hooking::Hook(ImageBase + 0x4B2AC40, TickFlush, TickFlushOG);
}