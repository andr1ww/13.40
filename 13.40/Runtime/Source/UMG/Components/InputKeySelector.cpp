#include "pch.h"
#include "Core.h"
#include "UMG/Components/InputKeySelector.h"

DEFINE_INIT(InputKeySelector)

void InputKeySelector::Init()
{
	Hooking::PatchBytes(ImageBase + 0x44D95C4, { 0xE9, 0xA4, 0x00, 0x00, 0x00 });
}
