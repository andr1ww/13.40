#pragma once
#include "pch.h"

namespace FortAthenaAIBotCustomizationData
{
	void GetOverrideCosmeticLoadout(const UFortAthenaAIBotCustomizationData* _this, AFortPlayerPawnAthena* Pawn, FFortAthenaLoadout& OutLoadout, const FGameplayTag& PredefinedCosmeticSetTag);

	void Init();
}
