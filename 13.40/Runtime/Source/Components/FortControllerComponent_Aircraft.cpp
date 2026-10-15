#include "pch.h"
#include "Core.h"
#include "Components/FortControllerComponent_Aircraft.h"
DEFINE_ENGINE_INIT(FortControllerComponent_Aircraft)

void FortControllerComponent_Aircraft::ServerAttemptAircraftJump_Implementation(UFortControllerComponent_Aircraft* _this, const FRotator& ClientRotation)
{
	if (!_this->bIsInAircraft || _this->bIsAttemptingAircraftJump)
		return;

	AFortPlayerControllerAthena* PlayerController = Cast<AFortPlayerControllerAthena>(_this->GetOwner());
	AFortGameModeAthena* GameMode = Cast<AFortGameModeAthena>(GWorld->AuthorityGameMode);
	AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(GWorld->GameState);

	if (!PlayerController || !GameMode || !GameState || GameState->bAircraftIsLocked || !_this->CurrentAircraft)
		return;

	_this->bIsAttemptingAircraftJump = true;
	GameMode->RestartPlayer(PlayerController);
	_this->bIsAttemptingAircraftJump = false;

	if (APawn* Pawn = PlayerController->Pawn)
		Pawn->K2_SetActorRotation(FRotator{ 0.f, ClientRotation.Yaw, 0.f }, true);

	PlayerController->SetControlRotation(ClientRotation);
	PlayerController->ClientSetRotation(ClientRotation, false);

	AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(PlayerController->PlayerState);

	if (!PlayerState || PlayerController->SkydiveLeader != PlayerState)
		return;

	for (int32 i = 0; i < GameState->PlayerArray.Num(); i++)
	{
		AFortPlayerStateAthena* SquadMember = Cast<AFortPlayerStateAthena>(GameState->PlayerArray[i]);

		if (!SquadMember || SquadMember == PlayerState || SquadMember->SquadId != PlayerState->SquadId)
			continue;

		AFortPlayerControllerAthena* SquadController = Cast<AFortPlayerControllerAthena>(SquadMember->Owner);

		if (!SquadController)
			continue;

		if (UFortControllerComponent_Aircraft* SquadAircraftComponent = SquadController->GetAircraftComponent())
			ServerAttemptAircraftJump_Implementation(SquadAircraftComponent, ClientRotation);
	}
}

void FortControllerComponent_Aircraft::Init()
{
	Hooking::HookEvery<UFortControllerComponent_Aircraft>(0x448 / 8, ServerAttemptAircraftJump_Implementation);
}
