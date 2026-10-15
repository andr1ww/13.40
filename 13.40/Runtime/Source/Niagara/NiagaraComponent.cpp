#include "pch.h"
#include "Core.h"
#include "Niagara/NiagaraComponent.h"

DEFINE_INIT(NiagaraComponent)

void NiagaraComponent::Init()
{
	Hooking::PatchBytes(ImageBase + 0x1014709, { 0xE9, 0x3D, 0x05, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90 });
}
