#include "pch.h"
#include "Core.h"
#include "Media/MediaModule.h"

DEFINE_INIT(MediaModule)

void MediaModule::Init()
{
	Hooking::Null(ImageBase + 0x469CB40);
}
