#include "pch.h"
#include "Core.h"
#include "UMG/Components/ComboBoxString.h"

DEFINE_INIT(ComboBoxString)

void ComboBoxString::Init()
{
	Hooking::PatchBytes(ImageBase + 0x44D8088, { 0xE9, 0x77, 0x00, 0x00, 0x00 });
}
