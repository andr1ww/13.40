#include "pch.h"
#include "Core.h"
#include "Player/FortPlayerPawn.h"
DEFINE_ENGINE_INIT(FortPlayerPawn)

void FortPlayerPawn::InitAbilitySystemComponentFromPlayerState(AFortPlayerPawn* _this)
{
	InitAbilitySystemComponentFromPlayerStateOG(_this);

	AFortPlayerState* PlayerState = Cast<AFortPlayerState>(_this->PlayerState);

	if (!PlayerState)
		return;

	AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(GWorld->GameState);

	if (!GameState)
		return;

	IAbilitySystemInterface* Interface = GetInterfaceAddress<IAbilitySystemInterface>(_this);

	if (!Interface)
		return;

	_this->ResetGameplayAttributes(false);

	UFortGlobals::InitializePlayerGameplayAbilities(Interface);

	AFortPlayerController* Controller = Cast<AFortPlayerController>(_this->Controller);

	if (!Controller)
		Controller = Cast<AFortPlayerController>(PlayerState->GetOwner());

	if (Controller)
	{
		if (UObject* Hero = Controller->GetOrInitializeHero())
			GameState->ApplyHomebaseEffectsOnPlayerSetup(PlayerState->UniqueId, nullptr, Interface, Hero, false, false, false);
	}

	GameState->OnAbilitySystemInterfaceReadyForPersistentGameplayEffects(Interface);

	PlayerState->ApplyCharacterCustomization(_this);
}

void FortPlayerPawn::InitAbilitySystemComponent(AFortPlayerPawn* _this)
{
	InitAbilitySystemComponentOG(_this);

	if (!_this || _this->AbilitySystemComponent)
		return;

	reinterpret_cast<void(*)(AFortPlayerPawn*)>(_this->VTable[0x1260 / 8])(_this);
}

void FortPlayerPawn::SetPawnCosmeticLoadout(AFortPlayerPawn* _this, FFortAthenaLoadout* CosmeticLoadout)
{
	if (_this)
	{
		if (AFortPlayerState* PlayerState = Cast<AFortPlayerState>(_this->PlayerState))
			PlayerState->ApplyCharacterCustomization(_this);
	}

	SetPawnCosmeticLoadoutOG(_this, CosmeticLoadout);
}

void FortPlayerPawn::Init()
{
	Hooking::Hook<AFortPlayerPawnAthena>(0x1260 / 8, InitAbilitySystemComponentFromPlayerState, InitAbilitySystemComponentFromPlayerStateOG);
	Hooking::Hook<AFortPlayerPawnAthena>(0x1258 / 8, InitAbilitySystemComponent, InitAbilitySystemComponentOG);
	Hooking::Hook(ImageBase + 0x2925650, SetPawnCosmeticLoadout, SetPawnCosmeticLoadoutOG);
}
