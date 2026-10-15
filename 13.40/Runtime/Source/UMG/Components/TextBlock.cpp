#include "pch.h"
#include "Core.h"
#include "UMG/Components/TextBlock.h"

DEFINE_INIT(TextBlock)

void TextBlock::Init()
{
	Hooking::PatchBytes(ImageBase + 0x44DC40F, { 0xE9, 0xAA, 0x00, 0x00, 0x00 });
}
