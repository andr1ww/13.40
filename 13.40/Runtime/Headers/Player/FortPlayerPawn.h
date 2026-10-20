#pragma once
#include "pch.h"

namespace FortPlayerPawn
{
	DefineHook(void, InitAbilitySystemComponentFromPlayerState, AFortPlayerPawn*);
	DefineHook(void, InitAbilitySystemComponent, AFortPlayerPawn*);
	DefineHook(void, SetPawnCosmeticLoadout, AFortPlayerPawn*, FFortAthenaLoadout*);
	void ServerHandlePickupInfo_Implementation(AFortPlayerPawn* _this, AFortPickup* Pickup, const FFortPickupRequestInfo& Params);
	void ServerReviveFromDBNO_Implementation(AFortPlayerPawn* _this, AController* EventInstigator);

	void Init();
}
