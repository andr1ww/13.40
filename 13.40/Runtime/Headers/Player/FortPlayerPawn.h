#pragma once
#include "pch.h"

namespace FortPlayerPawn
{
	DefineHook(void, InitAbilitySystemComponentFromPlayerState, AFortPlayerPawn*);
	DefineHook(void, InitAbilitySystemComponent, AFortPlayerPawn*);
	DefineHook(void, SetPawnCosmeticLoadout, AFortPlayerPawn*, FFortAthenaLoadout*);

	void Init();
}
