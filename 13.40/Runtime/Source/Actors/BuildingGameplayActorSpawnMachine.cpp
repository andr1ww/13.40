#include "pch.h"
#include "Core.h"
#include "Actors/BuildingGameplayActorSpawnMachine.h"
#include "Engine/TimerManager.h"
DEFINE_ENGINE_INIT(BuildingGameplayActorSpawnMachine)

void BuildingGameplayActorSpawnMachine::BeginPlay(ABuildingGameplayActorSpawnMachine* _this)
{
	BeginPlayOG(_this);

	if (AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(GWorld->GameState))
		OnGameStateLoaded(_this, GameState);
}

void BuildingGameplayActorSpawnMachine::OnGameStateLoaded(ABuildingGameplayActorSpawnMachine* _this, AGameStateBase* GameStateBase)
{
	OnGameStateLoadedOG(_this, GameStateBase);

	AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(GameStateBase);

	if (_this == NULL || GameState == NULL || _this->Role != ENetRole::ROLE_Authority)
		return;

	if (_this->ResurrectLocation == NULL)
	{
		TArray<AActor*> ActorArray = GetAll<AActor>(AFortPlayerStart::StaticClass());

		_this->ResurrectLocation = Cast<AFortPlayerStart>(UFortKismetLibrary::GetClosestActorFromArray(_this, ActorArray));

		if (ActorArray.IsValid())
			ActorArray.Free();

		if (_this->ResurrectLocation == NULL)
			return;
	}

	FSpawnMachineRepData RepData{};

	RepData.ReplicationKey = -1;
	RepData.ReplicationID = -1;
	RepData.MostRecentArrayReplicationKey = -1;

	RepData.Location = _this->K2_GetActorLocation();
	RepData.SpawnMachineState = _this->SpawnMachineState;
	RepData.SpawnMachineCooldownStartTime = 0.0f;
	RepData.SpawnMachineCooldownEndTime = 0.0f;

	_this->HandleIntoGameState = GameState->SpawnMachineRepData.SpawnMachineRepDataArray.Num();
	RepData.SpawnMachineRepDataHandle = _this->HandleIntoGameState;

	GameState->SpawnMachineRepData.SpawnMachineRepDataArray.Add(RepData);
	GameState->SpawnMachineRepData.OwningGameState = GameState;
	GameState->SpawnMachineRepData.MarkItemDirty(GameState->SpawnMachineRepData.SpawnMachineRepDataArray[_this->HandleIntoGameState]);
}

void BuildingGameplayActorSpawnMachine::HandleResurrectedPawnSpawned(ABuildingGameplayActorSpawnMachine* _this, AFortPlayerControllerAthena* PlayerController)
{
	AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(PlayerController->PlayerState);

	if (PlayerState != NULL)
	{
		PlayerState->RebootCounter++;
		PlayerState->DeathInfo = FDeathInfo{};

		PlayerController->PlayerDeathReport.Tags.GameplayTags.Free();
		PlayerController->PlayerDeathReport.Tags.ParentTags.Free();
		PlayerController->PlayerDeathReport = FFortPlayerDeathReport{};
		PlayerController->bClientNotifiedOfPawnDied = false;

		PlayerController->ClientClearDeathNotification();
		PlayerController->ClientGotoState(FName(L"Playing"));
	}

	if (AFortGameModeAthena* GameMode = Cast<AFortGameModeAthena>(GWorld->AuthorityGameMode))
		GameMode->AddToAlivePlayers(PlayerController);

	PlayerController->bMarkedAlive = true;
	_this->OnPlayerPawnResurrected(Cast<AFortPlayerPawn>(PlayerController->Pawn));
}

void BuildingGameplayActorSpawnMachine::HandleOnResurrectTimerComplete(ABuildingGameplayActorSpawnMachine* _this)
{
	if (_this == NULL)
		return;

	if (_this->PlayerIdsForResurrection.Num() <= 0)
	{
		_this->GetWorldTimerManager().ClearTimer(_this->ResurrectTimerHandle);
		_this->SetSpawnMachineState(ESpawnMachineState::Complete);
		_this->OnResurrectionCompleted();

		return;
	}

	AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(GWorld->GameState);
	AFortGameModeAthena* GameMode = Cast<AFortGameModeAthena>(GWorld->AuthorityGameMode);

	if (GameState != NULL && GameMode != NULL)
	{
		FUniqueNetIdRepl& PlayerIdToResurrect = _this->PlayerIdsForResurrection[0];

		for (int32 i = 0; i < GameState->PlayerArray.Num(); i++)
		{
			AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(GameState->PlayerArray[i]);

			if (PlayerState == NULL || !UFortKismetLibrary::EqualEqual_UniqueNetIdReplUniqueNetIdRepl(PlayerState->UniqueId, PlayerIdToResurrect))
				continue;

			AFortPlayerControllerAthena* PlayerController = Cast<AFortPlayerControllerAthena>(PlayerState->Owner);

			if (PlayerController == NULL)
				break;

			if (PlayerController->Pawn != NULL)
			{
				PlayerController->UnPossess();
				PlayerController->Pawn->K2_DestroyActor();
			}

			AActor* StartSpot = _this->ResurrectLocation != NULL ? (AActor*)_this->ResurrectLocation : (AActor*)_this;

			GameMode->RestartPlayerAtPlayerStart(PlayerController, StartSpot);

			AFortPlayerPawnAthena* ResurrectedPawn = Cast<AFortPlayerPawnAthena>(PlayerController->Pawn);

			if (ResurrectedPawn == NULL)
				break;

			ResurrectedPawn->SetHealth(ResurrectedPawn->GetMaxHealth());

			HandleResurrectedPawnSpawned(_this, PlayerController);

			break;
		}
	}

	_this->PlayerIdsForResurrection.Remove(0);
}

void BuildingGameplayActorSpawnMachine::Init()
{
	Hooking::Hook<ABuildingGameplayActorSpawnMachine>(0x328 / 8, BeginPlay, BeginPlayOG);
	Hooking::Hook(ImageBase + 0x215C210, OnGameStateLoaded, OnGameStateLoadedOG);
	Hooking::Rel32Hook(ImageBase + 0x2161992 + 3, HandleOnResurrectTimerComplete);

	Hooking::Patch<uint32>(ImageBase + 0x2161A7F, 0x202444C6);
	Hooking::Patch<uint8>(ImageBase + 0x2161A83, 0x01);
}
