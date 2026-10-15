#include "pch.h"
#include "Core.h"
#include "Core/Misc/App.h"

DEFINE_INIT(App)

void App::Init()
{
	Hooking::PatchBytes(ImageBase + 0x32BB636, { 0xEB, 0x2F });
	Hooking::RetFalse(ImageBase + 0x1017520);
	Hooking::RetFalse(ImageBase + 0x47E8E60);
}
