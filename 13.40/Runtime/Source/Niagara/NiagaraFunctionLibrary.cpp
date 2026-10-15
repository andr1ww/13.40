#include "pch.h"
#include "Core.h"
#include "Niagara/NiagaraFunctionLibrary.h"

DEFINE_INIT(NiagaraFunctionLibrary)

void NiagaraFunctionLibrary::Init()
{
	Hooking::RetNull(ImageBase + 0x10F6140);
	Hooking::RetNull(ImageBase + 0x110B440);
}
