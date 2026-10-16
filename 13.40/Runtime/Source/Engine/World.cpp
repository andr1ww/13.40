#include "pch.h"
#include "Core.h"
#include "Engine/World.h"

DEFINE_INIT(World)

bool World::Listen(UWorld* _this, FURL& InURL)
{
	if (_this->NetDriver)
	{
		GEngine->BroadcastNetworkFailure(_this, _this->NetDriver, ENetworkFailure::NetDriverAlreadyExists);
		return false;
	}

	// Create net driver.
	if (GEngine->CreateNamedNetDriver(_this, NAME_GameNetDriver, NAME_GameNetDriver))
	{
		_this->NetDriver = GEngine->FindNamedNetDriver(_this, NAME_GameNetDriver);
		_this->NetDriver->SetWorld(_this);
		FLevelCollection* const SourceCollection = _this->FindCollectionByType(ELevelCollectionType::DynamicSourceLevels);
		if (SourceCollection)
		{
			SourceCollection->SetNetDriver(_this->NetDriver);
		}
		FLevelCollection* const StaticCollection = _this->FindCollectionByType(ELevelCollectionType::StaticLevels);
		if (StaticCollection)
		{
			StaticCollection->SetNetDriver(_this->NetDriver);
		}
	}

	if (_this->NetDriver == nullptr)
	{
		GEngine->BroadcastNetworkFailure(_this, NULL, ENetworkFailure::NetDriverCreateFailure);
		return false;
	}

	FString Error;
	if (!_this->NetDriver->InitListen(_this, InURL, false, Error))
	{
		GEngine->BroadcastNetworkFailure(_this, _this->NetDriver, ENetworkFailure::NetDriverListenFailure, Error);
		UE_LOG(LogWorld, Log, TEXT("Failed to listen: %s"), *Error);
		GEngine->DestroyNamedNetDriver(_this, _this->NetDriver->NetDriverName);
		_this->NetDriver = nullptr;
		FLevelCollection* SourceCollection = _this->FindCollectionByType(ELevelCollectionType::DynamicSourceLevels);
		if (SourceCollection)
		{
			SourceCollection->SetNetDriver(nullptr); 
		}
		FLevelCollection* StaticCollection = _this->FindCollectionByType(ELevelCollectionType::StaticLevels);
		if (StaticCollection)
		{
			StaticCollection->SetNetDriver(nullptr);
		}
		return false;
	}

/*	static const bool bLanPlay = FParse::Param(FCommandLine::Get(), TEXT("lanplay"));
	const bool bLanSpeed = bLanPlay || InURL.HasOption(TEXT("LAN"));*/

	_this->NetDriver->SetWorld(_this);

	if (/*!bLanSpeed && */(_this->NetDriver->MaxInternetClientRate < _this->NetDriver->MaxClientRate) && (_this->NetDriver->MaxInternetClientRate > 2500))
	{
		_this->NetDriver->MaxClientRate = _this->NetDriver->MaxInternetClientRate;
	}

	if (_this->OwningGameInstance)
		_this->OwningGameInstance->SetRunAsDedicated(true);

	//NextSwitchCountdown = NetDriver->ServerTravelPause;
	return true;
}

void World::Init()
{
	Hooking::Rel32Swap(ImageBase + 0x4DC915B, Listen);
}
