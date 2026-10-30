#include "pch.h"
#include "Core.h"
#include "Player/FortPlayerPawn.h"
#include "Engine/TimerManager.h"
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

void FortPlayerPawn::ServerHandlePickupInfo_Implementation(AFortPlayerPawn* _this, AFortPickup* Pickup, const FFortPickupRequestInfo& Params)
{
	if (!Pickup || Pickup->bPickedUp)
		return;

	AFortPlayerController* PlayerController = Cast<AFortPlayerController>(_this->Controller);

	if (PlayerController && PlayerController->CanRemoveInventoryItem(Params.SwapWithItem))
	{
		if (UFortWorldItem* ItemToSwap = PlayerController->WorldInventory->GetItem(Params.SwapWithItem))
			PlayerController->ServerAttemptInventoryDrop(Params.SwapWithItem, ItemToSwap->ItemEntry.Count, false);
	}

	Pickup->PickupLocationData.bPlayPickupSound = Params.bPlayPickupSound;
	Pickup->SetPickupTarget(_this, Params.FlyTime / _this->PickupSpeedMultiplier, Params.Direction);
}

void FortPlayerPawn::ServerReviveFromDBNO_Implementation(AFortPlayerPawn* _this, AController* EventInstigator)
{
	UFortAbilitySystemComponent* AbilitySystemComponent = _this->AbilitySystemComponent;

	bool CanRevive = _this->CanReviveFromDBNO(EventInstigator);
	if (CanRevive && AbilitySystemComponent != NULL)
	{
		TSubclassOf<UGameplayEffect> GameplayEffectClass = NULL;
		if (EventInstigator == _this->Controller)
			GameplayEffectClass = _this->SelfReviveGameplayEffect;
		else
			GameplayEffectClass = _this->TeammateReviveGameplayEffect;

		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(GameplayEffectClass, 1.f, EffectContext);
		FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

		if (Spec != NULL)
		{
			_this->bIsDBNO = false;
			_this->bPlayedDying = false;
			_this->bIsDying = false;

			float ValueAtLevel = _this->SetByCallerReviveHealth.Evaluate(0);
			float CurrentHealth = 0.0f;
			if (_this->HealthSet != NULL)
				CurrentHealth = _this->HealthSet->Health.GetCurrentValue();

			if (ValueAtLevel > 0.0)
				Spec->SetSetByCallerMagnitude(FName(L"ReviveHeal"), ValueAtLevel - CurrentHealth);

			_this->UpdateDBNOInteractCollision();

			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec, FPredictionKey());

			if (ValueAtLevel > 0.0f && _this->GetHealth() < ValueAtLevel)
				_this->SetHealth(ValueAtLevel);

			FFortGlobalGameplayTags* GameplayTags = UFortGlobals::GameplayTags();

			_this->RemoveGameplayTag(GameplayTags->ActionPlayerDBNO);
			_this->RemoveGameplayTag(GameplayTags->ActionPlayerDBNOAthena);

			UFortAIPerceptionSystem::ForceForgetEnemy(_this);

			FGameplayTagContainer DBNOTags{};
			DBNOTags.AddTag(GameplayTags->ActionPlayerDBNO);
			DBNOTags.AddTag(GameplayTags->ActionPlayerDBNOAthena);
			AbilitySystemComponent->CancelAbilities(DBNOTags, FGameplayTagContainer(), NULL);

			_this->GetWorldTimerManager().ClearTimer(_this->DBNOTimerHandle);

			FGameplayEventData ReviveEventData{};
			ReviveEventData.Instigator = EventInstigator;
			ReviveEventData.Target = _this;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(_this, _this->EventReviveTag, ReviveEventData);

			AFortPawn* InstigatorPawn = NULL;
			if (EventInstigator != NULL)
				InstigatorPawn = Cast<AFortPawn>(EventInstigator->Pawn);

			_this->TriggerFeedbackEvent(FName(L"Resurrected"), InstigatorPawn, _this, -1.0f);

			AFortPlayerControllerZone* FortPlayerController = Cast<AFortPlayerControllerZone>(_this->Controller);
			if (FortPlayerController != NULL)
				FortPlayerController->OnPawnRevived(EventInstigator);

			if (EventInstigator != _this->Controller && EventInstigator != NULL)
			{
				AFortPlayerControllerZone* ReviverController = Cast<AFortPlayerControllerZone>(EventInstigator);
				if (ReviverController != NULL && ReviverController->StatManager != NULL)
				{
					UStat* TeammateRevivesStat = ReviverController->StatManager->AddOrGetStat(FName(L"TeammateRevives"), EStatRecordingPeriod::Life);
					if (TeammateRevivesStat != NULL)
						TeammateRevivesStat->ModifyStat(1, EStatMod::Delta);
				}
			}
		}
	}
}

void FortPlayerPawn::Init()
{
	Hooking::HookEvery<AFortPlayerPawn>(0xF80 / 8, ServerHandlePickupInfo_Implementation);
	Hooking::Hook<AFortPlayerPawnAthena>(0x1260 / 8, InitAbilitySystemComponentFromPlayerState, InitAbilitySystemComponentFromPlayerStateOG);
	Hooking::Hook<AFortPlayerPawnAthena>(0x1258 / 8, InitAbilitySystemComponent, InitAbilitySystemComponentOG);
	Hooking::Hook(ImageBase + 0x2925650, SetPawnCosmeticLoadout, SetPawnCosmeticLoadoutOG);
	Hooking::Hook<AFortPlayerPawn>(0xEE0 / 8, ServerReviveFromDBNO_Implementation);
}
