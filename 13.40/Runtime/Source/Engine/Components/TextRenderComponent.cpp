#include "pch.h"
#include "Core.h"
#include "Engine/Components/TextRenderComponent.h"

DEFINE_INIT(TextRenderComponent)

void TextRenderComponent::Init()
{
	Hooking::PatchBytes(ImageBase + 0x49040E5, { 0x4C, 0x8D, 0x5C, 0x24, 0x60, 0xE9, 0x40, 0x03, 0x00, 0x00 });
}
