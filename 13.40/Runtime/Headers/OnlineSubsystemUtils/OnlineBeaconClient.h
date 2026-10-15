#pragma once
#include "pch.h"

namespace OnlineBeaconClient
{
	inline thread_local AActor* InitializingBeacon = nullptr;

	DefineHook(bool, InitClient, AActor* _this, FURL& URL);
	DefineHook(ULocalPlayer*, GetFirstGamePlayer, UEngine* _this, UWorld* World);

	void Init();
}
