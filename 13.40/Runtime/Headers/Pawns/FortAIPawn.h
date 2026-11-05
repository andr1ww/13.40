#pragma once
#include "pch.h"

namespace FortAIPawn
{
	DefineHook(void, HandleDeath, AFortAIPawn* _this, float KillingDamage, const FGameplayTagContainer& GameplayTags, const FGameplayEffectContextHandle& EffectContext, AController* EventInstigator, AActor* DamageCauser);
	void DropAllItems(AFortAIPawn* _this, bool bSpawnPickups);
	void execDropAllItems(UObject* Context, FFrame& Stack, void* const Result);

	void Init();
}
