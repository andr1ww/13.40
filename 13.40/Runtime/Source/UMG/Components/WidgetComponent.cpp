#include "pch.h"
#include "Core.h"
#include "UMG/Components/WidgetComponent.h"

DEFINE_INIT(WidgetComponent)

void WidgetComponent::Init()
{
	Hooking::PatchBytes(ImageBase + 0x4512BFA, { 0x48, 0x8B, 0x6C, 0x24, 0x70, 0xE9, 0xEB, 0x00, 0x00, 0x00 });
	Hooking::PatchBytes(ImageBase + 0x45427F0, { 0xE9, 0x0B, 0xCE, 0x33, 0x00 });
	Hooking::Null(ImageBase + 0x4528950);
}
