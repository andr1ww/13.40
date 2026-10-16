#include "pch.h"
#include "Core.h"
#include "OnlineSubsystemUtils/OnlineBeaconClient.h"

DEFINE_INIT(OnlineBeaconClient)

bool OnlineBeaconClient::InitClient(AActor* _this, FURL& URL)
{
	InitializingBeacon = _this;
	const bool bResult = InitClientOG(_this, URL);
	InitializingBeacon = nullptr;

	return bResult;
}

ULocalPlayer* OnlineBeaconClient::GetFirstGamePlayer(UEngine* _this, UWorld* World)
{
	UNetConnection* BeaconConnection = InitializingBeacon ? Get<UNetConnection*>(InitializingBeacon, 0x258) : nullptr;
	if (!BeaconConnection)
		return nullptr;

	TSharedPtr<void> UniquePlayerId;
	reinterpret_cast<void*(*)(void*, TSharedPtr<void>*, UWorld*, int32, FName)>(ImageBase + 0xBAE6F0)(nullptr, &UniquePlayerId, World, 0, FName());

	if (UniquePlayerId.IsValid())
	{
		FUniqueNetIdRepl PlayerId{};
		PlayerId.GetUniqueNetId() = std::move(UniquePlayerId);
		reinterpret_cast<FUniqueNetIdRepl*(*)(FUniqueNetIdRepl*, const FUniqueNetIdRepl*)>(ImageBase + 0xAEAA00)(&BeaconConnection->PlayerID, &PlayerId);
		PlayerId.GetUniqueNetId().Reset();
	}

	return nullptr;
}

void OnlineBeaconClient::Init()
{
	Hooking::Hook(ImageBase + 0xBB1ED0, InitClient, InitClientOG);
	Hooking::Rel32Swap(ImageBase + 0xBB1F8E, GetFirstGamePlayer, GetFirstGamePlayerOG);
}
