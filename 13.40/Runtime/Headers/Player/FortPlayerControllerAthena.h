#pragma once
#include "pch.h"

namespace FortPlayerControllerAthena
{
	DefineHook(void, OnReadyToStartMatch, AFortPlayerControllerAthena* _this);
	DefineHook(void, ServerLoadingScreenDropped_Implementation, AFortPlayerControllerAthena* _this);
	void ServerAcknowledgePossession_Implementation(AFortPlayerControllerAthena* _this, APawn* P);

	void Init();
}
