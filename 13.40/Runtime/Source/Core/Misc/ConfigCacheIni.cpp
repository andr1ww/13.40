#include "pch.h"
#include "Core.h"
#include "Core/Misc/ConfigCacheIni.h"

DEFINE_INIT(ConfigCacheIni)

static std::wstring GetPath(const std::wstring& InPath)
{
	const size_t Slash = InPath.find_last_of(L"/\\");
	return Slash == std::wstring::npos ? std::wstring() : InPath.substr(0, Slash);
}

static std::wstring Replace(std::wstring Str, const wchar_t* From, const wchar_t* To)
{
	const size_t FromLen = wcslen(From);
	const size_t ToLen = wcslen(To);

	for (size_t Pos = Str.find(From); Pos != std::wstring::npos; Pos = Str.find(From, Pos + ToLen))
		Str.replace(Pos, FromLen, To);

	return Str;
}

static bool Contains(const std::wstring& Str, const wchar_t* SubStr)
{
	return SubStr && Str.find(SubStr) != std::wstring::npos;
}

void ConfigCacheIni::AddStaticLayersToHierarchy(void* _this, const wchar_t* InBaseIniName, const wchar_t* InPlatformName, const wchar_t* EngineConfigDir, const wchar_t* SourceConfigDir)
{
	AddStaticLayersToHierarchyOG(_this, InBaseIniName, InPlatformName, EngineConfigDir, SourceConfigDir);

	static const auto GConfigLayers = reinterpret_cast<const FConfigLayer*>(ImageBase + 0x86F7470);
	static const auto GConfigExpansions = reinterpret_cast<const FConfigLayerExpansion*>(ImageBase + 0x86F7550);
	static const auto GetPlatformInfo = reinterpret_cast<const FDataDrivenPlatformInfo*(*)(const FString*)>(ImageBase + 0x32D8810);
	static const auto PerformFinalExpansions = reinterpret_cast<FString(*)(FString, const FString&, const wchar_t*, const wchar_t*)>(ImageBase + 0x32B5120);
	static const auto AddStaticLayer = reinterpret_cast<void(*)(void*, const int32*, FIniFilename*)>(ImageBase + 0x329C9D0);

	const FString PlatformName(InPlatformName ? InPlatformName : L"Windows");
	const FString DedicatedServerString(L"DedicatedServer");
	const std::wstring EngineDir = GetPath(GetPath(EngineConfigDir));
	const std::wstring ProjectDir = GetPath(GetPath(SourceConfigDir));
	const int32 DedicatedServerIndex = GetPlatformInfo(&PlatformName)->IniParentChain.Num() + 1;

	for (int32 LayerIndex = 0; LayerIndex < 9; LayerIndex++)
	{
		const FConfigLayer& Layer = GConfigLayers[LayerIndex];
		if (Layer.Flag & uint32(EConfigLayerFlags::NoExpand))
			continue;

		const std::wstring LayerPath = Replace(Layer.Path, L"{TYPE}", InBaseIniName);
		if (!Contains(LayerPath, L"{PLATFORM}"))
			continue;

		for (int32 ExpansionIndex = 0; ExpansionIndex < 6; ExpansionIndex++)
		{
			const FConfigLayerExpansion& Expansion = GConfigExpansions[ExpansionIndex];
			std::wstring ExpandedPath = LayerPath;

			if (Expansion.Before1)
			{
				if (!Contains(LayerPath, Expansion.Before1) && !Contains(LayerPath, Expansion.Before2))
					continue;

				ExpandedPath = Replace(LayerPath, Expansion.Before1, Expansion.After1);
				if (Expansion.Before2)
					ExpandedPath = Replace(ExpandedPath, Expansion.Before2, Expansion.After2);
			}

			if (ExpandedPath.empty())
				continue;

			const FString PlatformPath = PerformFinalExpansions(FString(ExpandedPath.c_str()), DedicatedServerString, EngineDir.c_str(), ProjectDir.c_str());

			const bool bGenerateCacheKey = (Layer.Flag & uint32(EConfigLayerFlags::GenerateCacheKey)) && ExpansionIndex == 5;
			FIniFilename IniFilename{ FString(Replace(PlatformPath.ToWString(), L"Config/DedicatedServer/", L"Config/").c_str()), false, bGenerateCacheKey ? FString(InBaseIniName) : FString() };

			const int32 Key = LayerIndex * 10000 + ExpansionIndex * 100 + DedicatedServerIndex;
			AddStaticLayer(reinterpret_cast<uint8*>(_this) + 0x60, &Key, &IniFilename);
		}
	}
}

void ConfigCacheIni::Init()
{
	Hooking::Hook(ImageBase + 0x32A2530, AddStaticLayersToHierarchy, AddStaticLayersToHierarchyOG);
}
