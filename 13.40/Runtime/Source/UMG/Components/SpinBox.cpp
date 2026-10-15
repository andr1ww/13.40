#include "pch.h"
#include "Core.h"
#include "UMG/Components/SpinBox.h"

DEFINE_INIT(SpinBox)

void SpinBox::Init()
{
	Hooking::PatchBytes(ImageBase + 0x44DBDE7, { 0xE9, 0x99, 0x00, 0x00, 0x00 });
}
