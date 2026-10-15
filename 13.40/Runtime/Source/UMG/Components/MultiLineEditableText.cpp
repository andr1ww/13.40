#include "pch.h"
#include "Core.h"
#include "UMG/Components/MultiLineEditableText.h"

DEFINE_INIT(MultiLineEditableText)

void MultiLineEditableText::Init()
{
	Hooking::PatchBytes(ImageBase + 0x44DA63C, { 0x4C, 0x8B, 0xA4, 0x24, 0xA8, 0x00, 0x00, 0x00, 0xE9, 0xAC, 0x00, 0x00, 0x00 });
}
