#include "pch.h"
#include "Core.h"
#include "Hotfix/OnlineHotfixManager.h"

DEFINE_INIT(OnlineHotfixManager)

bool OnlineHotfixManager::StartsWithDefaultPrefix(const FString& _this, const FString& DefaultPrefix, int32 SearchCase)
{
	const FString& ServerPrefix = (&DefaultPrefix)[-1];

	return StartsWithDefaultPrefixOG(_this, DefaultPrefix, SearchCase) || StartsWithDefaultPrefixOG(_this, ServerPrefix, SearchCase);
}

void OnlineHotfixManager::Init()
{
	Hooking::Rel32Swap(ImageBase + 0xB5B8EF, StartsWithDefaultPrefix, StartsWithDefaultPrefixOG);
}
