#include "pch.h"
#include "Core.h"
#include "Slate/Framework/Application/SlateApplication.h"

DEFINE_INIT(SlateApplication)

void SlateApplication::PrepassWindowAndChildren(TSharedRef<SWindow> WindowToPrepass)
{
}

void SlateApplication::Init()
{
	Hooking::Hook(ImageBase + 0x3679720, PrepassWindowAndChildren);
}
