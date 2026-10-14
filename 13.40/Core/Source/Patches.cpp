#include "pch.h"
#include "Core.h"
#include "Memcury.h"

namespace Patches
{
	DefineHook(void, SendRequestNow, UObject* a1, __int64 a2, uint8 a3)
	inline void SendRequestNow(UObject* a1, __int64 a2, uint8 a3)
	{
		return SendRequestNowOG(a1, a2, 0x3);
	}

	DefineHook(void, StartInitialPreload, UUpdateManager* UpdateManager)
	inline void StartInitialPreload(UUpdateManager* UpdateManager)
	{
		InSDKUtils::GetVirtualFunction<void(*)(UUpdateManager*, EUpdateState)>(UpdateManager, 0x5E)(UpdateManager, EUpdateState::WaitingOnInitialLoad);
		InSDKUtils::GetVirtualFunction<void(*)(UUpdateManager*)>(UpdateManager, 0x5C)(UpdateManager);
	}

	void Init()
	{
		void** CRashFixMaybe = reinterpret_cast<void**>(ImageBase + 0x6558CF8);
		Hooking::VirtualSwap(CRashFixMaybe, 0, reinterpret_cast<void*>(ImageBase + 0x91B840));
		Hooking::VirtualSwap(CRashFixMaybe, 3, reinterpret_cast<void*>(ImageBase + 0x91B840));

		Hooking::Hook(ImageBase + 0xB5A990, StartInitialPreload, StartInitialPreloadOG);
		Hooking::Hook(ImageBase + 0x1312430, SendRequestNow, SendRequestNowOG);

		Hooking::RetTrue(ImageBase + 0x49C50F0); // kickplayer
		Hooking::RetTrue(ImageBase + 0x4E1A9A0); // GetNetMode
		Hooking::RetTrue(ImageBase + 0x4AA8530); // SpawnPlayActor
		Hooking::RetTrue(ImageBase + 0x34C9C80); // client/server context

		Hooking::Null(ImageBase + 0x1D43AC0); // gamesession id patch 1
		Hooking::Null(ImageBase + 0x1D44090); // patch 2 ^
	}
}

DEFINE_INIT(Patches)