#include "pch.h"
#include "Core.h"
#include "Engine/GameplayStatics.h"

DEFINE_INIT(GameplayStatics)

void GameplayStatics::Init()
{
	Hooking::RetNull(ImageBase + 0x49B30C0);
	Hooking::RetNull(ImageBase + 0x49D2660);
	Hooking::Null(ImageBase + 0x49CF970);
	Hooking::Null(ImageBase + 0x49D0760);
	Hooking::Null(ImageBase + 0x49CF820);
	Hooking::Null(ImageBase + 0x49C8D60);
	Hooking::RetNull(ImageBase + 0x49B33B0);
	Hooking::Null(ImageBase + 0x49C8F20);
	Hooking::RetNull(ImageBase + 0x49D3360);
	Hooking::RetNull(ImageBase + 0x49D3570);
}
