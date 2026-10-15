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

FString* Settings::GetGlobalDefaultGameMode(FString* ret)
{
	UGameMapsSettings* DefaultSettings = UGameMapsSettings::GetDefaultObj();

	*ret = DefaultSettings->GlobalDefaultServerGameMode.IsValid() ? DefaultSettings->GlobalDefaultServerGameMode.ToString() : DefaultSettings->GlobalDefaultGameMode.ToString();
	return ret;
}

void Settings::Init()
{
	Hooking::Hook(ImageBase + 0x3CF17F0, GetGameDefaultMap, GetGameDefaultMapOG);
	Hooking::Hook(ImageBase + 0x3CF1E80, GetGlobalDefaultGameMode);
}