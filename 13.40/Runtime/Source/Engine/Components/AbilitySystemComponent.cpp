#include "pch.h"
#include "Core.h"
#include "Engine/Components/AbilitySystemComponent.h"
DEFINE_ENGINE_INIT(AbilitySystemComponent)

void AbilitySystemComponent::InternalServerTryActivateAbility(UAbilitySystemComponent* _this, FGameplayAbilitySpecHandle Handle, bool InputPressed, const FPredictionKey& PredictionKey, const FGameplayEventData* TriggerEventData)
{
	FGameplayAbilitySpec* Spec = _this->FindAbilitySpecFromHandle(Handle);
	if (!Spec)
	{
		UE_LOG(LogAbilitySystem, Log, TEXT("InternalServerTryActivateAbility. Rejecting ClientActivation of ability with invalid SpecHandle!"));
		_this->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
		return;
	}

	_this->ConsumeAllReplicatedData(Handle, PredictionKey);

	const UGameplayAbility* AbilityToActivate = Spec->Ability;
	if (!AbilityToActivate)
		return;

	UGameplayAbility* InstancedAbility = nullptr;
	Spec->InputPressed = true;

	bool bActivated;
	{
		FScopedNativeActionBlockSuspension NativeActionBlockSuspension(Cast<UFortAbilitySystemComponent>(_this));
		bActivated = _this->InternalTryActivateAbility(Handle, PredictionKey, &InstancedAbility, nullptr, TriggerEventData);
	}

	if (!bActivated)
	{
		std::string AbilityName = AbilityToActivate->GetName();
		UE_LOG(LogAbilitySystem, Log, TEXT("InternalServerTryActivateAbility. Rejecting ClientActivation of %s. InternalTryActivateAbility failed"), std::wstring(AbilityName.begin(), AbilityName.end()).c_str());
		_this->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
		Spec->InputPressed = false;

		_this->MarkAbilitySpecDirty(*Spec);
	}
}

void AbilitySystemComponent::Init()
{
	Hooking::HookEvery<UAbilitySystemComponent>(0x7D0 / 8, InternalServerTryActivateAbility);
}
