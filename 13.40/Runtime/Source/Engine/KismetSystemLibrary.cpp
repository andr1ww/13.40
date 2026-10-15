#include "pch.h"
#include "Core.h"
#include "Engine/KismetSystemLibrary.h"

DEFINE_INIT(KismetSystemLibrary)

void KismetSystemLibrary::Init()
{
	Hooking::RetTrue(ImageBase + 0x4A663C0);
}
