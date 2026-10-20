#include "pch.h"
#include "Core.h"
#include "Player/FortPlayerPawnAthena.h"
#include "Player/FortPlayerPawn.h"
DEFINE_ENGINE_INIT(FortPlayerPawnAthena)

void FortPlayerPawnAthena::EndGameReviveFromDBNO(AFortPlayerPawnAthena* _this)
{
	if (!_this->bIsDBNO)
		return;

	_this->ServerReviveFromDBNO(_this->Controller);
}

void FortPlayerPawnAthena::ServerReviveFromDBNO_Implementation(AFortPlayerPawnAthena* _this, AController* EventInstigator)
{
	FortPlayerPawn::ServerReviveFromDBNO_Implementation(_this, EventInstigator);

	if (!_this->bIsDBNO)
	{
		_this->DBNORevivalStacking = 0;

		UFortAbilitySystemComponent* AbilitySystemComponent = _this->AbilitySystemComponent;

		FFortGlobalGameplayTags* GameplayTags = UFortGlobals::GameplayTags();

		AbilitySystemComponent->RemoveLooseGameplayTag(GameplayTags->StatusDBNOBeingRevived);
		AbilitySystemComponent->RemoveGameplayCue(GameplayTags->GameplayCuePlayerBeingReviviedFromDBNO);

		AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(_this->PlayerState);

		PlayerState->DeathInfo = FDeathInfo{};
		PlayerState->OnRep_DeathInfo();
	}
}

void FortPlayerPawnAthena::Init()
{
	Hooking::Rel32Swap(ImageBase + 0x1EA62F5, EndGameReviveFromDBNO);
	Hooking::Hook<AFortPlayerPawnAthena>(0xEE0 / 8, ServerReviveFromDBNO_Implementation);
}
