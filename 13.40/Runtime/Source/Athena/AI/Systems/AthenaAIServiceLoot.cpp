#include "pch.h"
#include "Core.h"
#include "Athena/AI/Systems/AthenaAIServiceLoot.h"
DEFINE_ENGINE_INIT(AthenaAIServiceLoot)

void AthenaAIServiceLoot::Init()
{
	for (uintptr_t Offset : { 0x1E03E6C, 0x1E03E76, 0x1E03E81, 0x1E03FE8, 0x1E03FF2, 0x1E03FFD })
		Hooking::PatchBytes(ImageBase + Offset, { 0x00, 0x00, 0x80, 0x48 });
}
