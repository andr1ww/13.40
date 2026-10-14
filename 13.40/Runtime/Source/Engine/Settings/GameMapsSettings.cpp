#include "pch.h"
#include "Core.h"
#include "Engine/Settings/GameMapsSettings.h"

DEFINE_INIT(Settings)

void Settings::GetGameDefaultMap(FString* ret, char a2)
{
	UGameMapsSettings* DefaultSettings = UGameMapsSettings::GetDefaultObj();
	DefaultSettings->GameDefaultMap.AssetPathName = FName(L"/Game/Athena/Apollo/Maps/Apollo_Terrain.Apollo_Terrain");

	GetGameDefaultMapOG(ret, a2);
}

void Settings::Init()
{
	Hooking::Hook(ImageBase + 0x3CF17F0, GetGameDefaultMap, GetGameDefaultMapOG);
}