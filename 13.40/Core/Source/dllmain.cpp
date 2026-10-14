#include "pch.h"
#include "Paradise/Public/Core.h"
#include "Paradise/Globals.h"
#include "Paradise/Public/Unreal.h"

void Init()
{
	Paradise::Unreal::FMemory::IRealloc = decltype(Paradise::Unreal::FMemory::IRealloc)(ImageBase + 0x3230950);

	while (!Paradise::Core::Initialize())
		Sleep(250);

	std::string CommandLine = GetCommandLineA();
	if (CommandLine.find("-nullrhi") == std::string::npos)
		return;

	AllocConsole();

	FILE* fptr;
	freopen_s(&fptr, "CONOUT$", "w+", stdout);
	freopen_s(&fptr, "CONOUT$", "w", stderr);
	freopen_s(&fptr, "CONIN$", "r", stdin);

	std::wstring Title = L"13.40 | https://github.com/andr1ww | Loading | Built: ";
	Title += std::wstring(L"" __DATE__ L" " __TIME__);

	SetConsoleTitleW(Title.c_str());

	for (auto& Init : Initters())
		Init();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
		std::thread(Init).detach(); // we don't want to be on the game thread at init for obvious reasons.. - andr1ww
	}

    return TRUE;
}

