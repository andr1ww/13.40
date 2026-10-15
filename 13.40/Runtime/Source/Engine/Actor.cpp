#include "pch.h"
#include "Core.h"
#include "Engine/Actor.h"

DEFINE_INIT(Actor)

void Actor::ReceiveTick(AActor* _this, float DeltaSeconds)
{
	if (_this->bAllowReceiveTickEventOnDedicatedServer)
		ReceiveTickOG(_this, DeltaSeconds);
}

void Actor::Init()
{
	Hooking::Rel32Swap(ImageBase + 0x4737E05, ReceiveTick, ReceiveTickOG);
}
