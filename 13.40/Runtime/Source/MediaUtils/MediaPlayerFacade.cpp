#include "pch.h"
#include "Core.h"
#include "MediaUtils/MediaPlayerFacade.h"

DEFINE_INIT(MediaPlayerFacade)

bool MediaPlayerFacade::Open(void* _this)
{
	Set<bool>(_this, 0x68, false);
	return false;
}

void MediaPlayerFacade::Init()
{
	Hooking::Hook(ImageBase + 0x46A4400, Open);
}
