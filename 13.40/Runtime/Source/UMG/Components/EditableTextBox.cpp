#include "pch.h"
#include "Core.h"
#include "UMG/Components/EditableTextBox.h"

DEFINE_INIT(EditableTextBox)

void EditableTextBox::Init()
{
	Hooking::PatchBytes(ImageBase + 0x44D8893, { 0xE9, 0x8F, 0x00, 0x00, 0x00, 0x90 });
}
