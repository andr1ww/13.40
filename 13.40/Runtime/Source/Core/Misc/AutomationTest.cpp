#include "pch.h"
#include "Core.h"
#include "Core/Misc/AutomationTest.h"

DEFINE_INIT(AutomationTest)

void AutomationTest::Init()
{
	Hooking::PatchBytes(ImageBase + 0x32AEC55, { 0x90, 0x90 });
}
