#pragma once
#include "pch.h"

namespace AbilitySystemComponent
{
	void InternalServerTryActivateAbility(UAbilitySystemComponent* _this, FGameplayAbilitySpecHandle Handle, bool InputPressed, const FPredictionKey& PredictionKey, const FGameplayEventData* TriggerEventData);

	void Init();
}
