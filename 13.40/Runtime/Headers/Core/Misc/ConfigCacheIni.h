#pragma once
#include "pch.h"

enum class EConfigLayerFlags : uint32
{
	GenerateCacheKey = 1 << 3,
	NoExpand = 1 << 4,
};

struct FConfigLayer
{
	const wchar_t* EditorName;
	const wchar_t* Path;
	uint32 Flag;
};

struct FConfigLayerExpansion
{
	const wchar_t* Before1;
	const wchar_t* After1;
	const wchar_t* Before2;
	const wchar_t* After2;
};

struct FIniFilename
{
	FString Filename;
	bool bRequired;
	FString CacheKey;
};

struct FDataDrivenPlatformInfo
{
	TArray<FString> IniParentChain;
};

namespace ConfigCacheIni
{
	DefineHook(void, AddStaticLayersToHierarchy, void* _this, const wchar_t* InBaseIniName, const wchar_t* InPlatformName, const wchar_t* EngineConfigDir, const wchar_t* SourceConfigDir);

	void Init();
}
