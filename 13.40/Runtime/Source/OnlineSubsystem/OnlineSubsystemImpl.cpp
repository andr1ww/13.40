#include "pch.h"
#include "Core.h"
#include "OnlineSubsystem/OnlineSubsystemImpl.h"

DEFINE_INIT(OnlineSubsystemImpl)

void OnlineSubsystemImpl::Init()
{
	Hooking::RetTrue(ImageBase + 0xB85980);
}
