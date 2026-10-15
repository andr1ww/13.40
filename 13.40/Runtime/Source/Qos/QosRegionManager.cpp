#include "pch.h"
#include "Core.h"
#include "Qos/QosRegionManager.h"

DEFINE_INIT(QosRegionManager)

void QosRegionManager::Init()
{
	Hooking::Null(ImageBase + 0xADC460);
}
