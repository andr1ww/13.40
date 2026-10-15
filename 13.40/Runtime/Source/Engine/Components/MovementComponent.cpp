#include "pch.h"
#include "Core.h"
#include "Engine/Components/MovementComponent.h"

DEFINE_INIT(MovementComponent)

void MovementComponent::Init()
{
	Hooking::PatchBytes(ImageBase + 0x48AB50D, { 0xE9, 0x1F, 0x01, 0x00, 0x00, 0x90, 0x90 });
}
