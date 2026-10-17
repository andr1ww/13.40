#include "pch.h"
#include "Core.h"
#include "GameModes/FortGameMode.h"
#include "AI/FortAthenaAIBotController.h"
DEFINE_ENGINE_INIT(FortGameMode)

void FortGameMode::InitWorldManager(AFortGameModeAthena* _this, AFortWorldManager* WorldManager)
{
	AFortGameStateAthena* GameState = _this->GameState;

	if (!GameState)
		return;

	if (!WorldManager)
		WorldManager = static_cast<AFortWorldManager*>(UGameplayStatics::GetActorOfClass(GWorld, AFortWorldManager::StaticClass()));

	if (!WorldManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("FortGameMode::InitWorldManager: Failed to find a world manager"));
		return;
	}

	WorldManager->bStreamInBuildings = true;

	if (GameState->WorldManager != WorldManager)
		GameState->SetWorldManager(WorldManager);
}

void FortGameMode::SetupPlaylist(AFortGameModeAthena* _this)
{
	if (_this->CurrentPlaylistName.ComparisonIndex == 0)
		_this->SetCurrentPlaylistName(FName("Playlist_DefaultSolo"));

	UFortPlaylistAthena* Playlist = _this->GetPlaylistManager()->GetPlaylistByName(_this->CurrentPlaylistName);

	if (!Playlist)
		return;

	_this->WarmupRequiredPlayerCount = 1;

	if (Playlist->AISettings)
		_this->AISettings = Playlist->AISettings;

	_this->GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;

	_this->GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
	if (_this->GameState->CurrentPlaylistInfo.PlaylistReplicationKey == -1)
		_this->GameState->CurrentPlaylistInfo.PlaylistReplicationKey = 0;
	_this->GameState->CurrentPlaylistInfo.MarkArrayDirty();
	_this->GameState->OnRep_CurrentPlaylistInfo();

	_this->GameState->CurrentPlaylistId = Playlist->PlaylistId;

	_this->GameSession->MaxPlayers = Playlist->MaxPlayers;
	_this->GameState->AirCraftBehavior = Playlist->AirCraftBehavior;

	_this->GameState->OnRep_AdditionalPlaylistLevelsStreamed();
	_this->GameContextTags.AppendTags(_this->GameState->GetAthenaPlaylistContextTags());

	UClass* PhoebeManagerClass = StaticLoadObject<UClass>(L"/Game/Athena/AI/Phoebe/BP_PhoebeManager.BP_PhoebeManager_C");

	_this->ServerBotManagerClass = PhoebeManagerClass ? PhoebeManagerClass : UFortServerBotManagerAthena::StaticClass();
	_this->ServerBotManager = (UFortServerBotManagerAthena*)UGameplayStatics::SpawnObject(_this->ServerBotManagerClass, _this);

	if (_this->ServerBotManager)
	{
		_this->ServerBotManager->UseCustomSupportedItemList.Value = 0.f;
		_this->ServerBotManager->UseCustomSupportedItemList.Curve.CurveTable = nullptr;
		_this->ServerBotManager->UseCustomSupportedItemList.Curve.RowName = FName();

		_this->ServerBotManager->CachedGameMode = _this;
		_this->ServerBotManager->CachedGameState = _this->GameState;

		Set<bool>(_this->ServerBotManager, 0x458, true);
	}

	if (!_this->SpawningPolicyManager)
	{
		FActorSpawnParameters SpawnParameters{};
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParameters.Owner = _this;

		_this->SpawningPolicyManager = GWorld->SpawnActor<AFortAthenaSpawningPolicyManager>({}, SpawnParameters);
		_this->SpawningPolicyManager->GameStateAthena = _this->GameState;
		_this->SpawningPolicyManager->GameModeAthena = _this;
	}
}

void FortGameMode::FinishWorldInitialization(AFortGameModeAthena* _this, AActor* WorldManager)
{
	_this->bWorldIsReady = true;
	FinishWorldInitializationOG(_this, WorldManager);

	InitWorldManager(_this, Cast<AFortWorldManager>(WorldManager));
	SetupPlaylist(_this);

	std::wstring Title = L"13.40 | Ready | Built: ";
	Title += std::wstring(L"" __DATE__ L" " __TIME__);

	SetConsoleTitleW(Title.c_str());
	UE_LOG(LogFort, Log, TEXT("FortGameMode::FinishWorldInitialization: %s"), *FString(Title.c_str()));
}

int32 Index;
void FortGameMode::HandleStartingNewPlayer(AFortGameModeAthena* _this, AFortPlayerControllerAthena* NewPlayer)
{
	if (!NewPlayer || !NewPlayer->PlayerState)
		return HandleStartingNewPlayerOG(_this, NewPlayer);

	NewPlayer->PlayerState->WorldPlayerId = ++Index;

	if (UFortGlobals::IsInAthena(GWorld))
	{
		FGameMemberInfo Member{ -1, -1, -1 };
		Member.TeamIndex = NewPlayer->PlayerState->TeamIndex;
		Member.SquadId = NewPlayer->PlayerState->SquadId;
		Member.MemberUniqueId = NewPlayer->PlayerState->UniqueId;

		_this->GameState->GameMemberInfoArray.OwningGameState = _this->GameState;

		FGameMemberInfo& AddedMember = _this->GameState->GameMemberInfoArray.Members.Add(Member);
		_this->GameState->GameMemberInfoArray.MarkItemDirty(AddedMember);
		_this->GameState->GameMemberInfoArray.MarkArrayDirty();
		AddedMember.PostReplicatedAdd(_this->GameState->GameMemberInfoArray);

		NewPlayer->MatchReport = NewObject<UAthenaPlayerMatchReport>(NewPlayer);
	}

	if (!NewPlayer->StatManager)
		NewPlayer->StatManager = NewObject<UStatManager>(NewPlayer);

	return HandleStartingNewPlayerOG(_this, NewPlayer);
}

void FortGameMode::PlaceBotOnTeam(AFortGameModeAthena* _this, AFortPlayerStateAthena* PlayerState, AFortTeamInfo* Team, uint8 SquadId)
{
	PlaceBotOnTeamOG(_this, PlayerState, Team, SquadId);

	if (!Cast<AFortAthenaAIBotController>(PlayerState->Owner))
		return;

	PlayerState->WorldPlayerId = ++Index;
	_this->GameState->AddPlayerToTeamLists(PlayerState);
}

bool FortGameMode::StartAircraftPhase(AFortGameModeAthena* _this, bool bGoStraightToSafeZone)
{
	for (AFortAthenaAIBotController* Bot : _this->AliveBots)
	{
		if (Bot && Bot->Inventory && Bot->bIsAnAthenaGameParticipant)
			FortAthenaAIBotController::RemoveAllInventoryItems(Bot);
	}

	bool bStarted = StartAircraftPhaseOG(_this, bGoStraightToSafeZone);

	UFortServerBotManagerAthena* BotManager = _this->ServerBotManager;
	AFortAthenaAircraft* Aircraft = _this->GameState->GetAircraft(0);

	if (!bStarted || !BotManager || !Aircraft)
		return bStarted;

	static FName IsInBusKey = FName(L"AIEvaluator_Global_IsInBus");
	static FName HasEverJumpedFromBusKey = FName(L"AIEvaluator_Global_HasEverJumpedFromBusKey");
	static FName HasEverJumpedFromBusAndLandedKey = FName(L"AIEvaluator_Global_HasEverJumpedFromBusAndLandedKey");

	for (AFortAthenaAIBotController* Bot : _this->AliveBots)
	{
		if (!Bot || !Bot->Blackboard)
			continue;

		if (UFortControllerComponent_Aircraft* AircraftComponent = Cast<UFortControllerComponent_Aircraft>(Bot->GetComponentByClass(UFortControllerComponent_Aircraft::StaticClass())))
			AircraftComponent->ClientEnterAircraft(Aircraft);

		Bot->Blackboard->SetValueAsBool(IsInBusKey, true);
		Bot->Blackboard->SetValueAsBool(HasEverJumpedFromBusKey, false);
		Bot->Blackboard->SetValueAsBool(HasEverJumpedFromBusAndLandedKey, false);

		if (AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(Bot->PlayerState))
		{
			PlayerState->bHasEverSkydivedFromBus = false;
			PlayerState->bHasEverSkydivedFromBusAndLanded = false;
		}

		if (FRand(nullptr) > BotManager->ThankBusDriverProbability.Evaluate(0.f))
			continue;

		float MinTime = BotManager->ThankBusDriverMinTime.Evaluate(0.f);
		float Delay = MinTime + FRand(nullptr) * (BotManager->ThankBusDriverMaxTime.Evaluate(0.f) - MinTime);

		UKismetSystemLibrary::K2_SetTimer(Bot, FString(L"ThankBusDriver"), Delay, false, 0.f, 0.f);
	}

	return bStarted;
}

bool FortGameMode::ReadyToStartMatch(AFortGameModeAthena* _this)
{
	const FName& WaitingToStart = *reinterpret_cast<FName*>(ImageBase + 0x8E75F78);

	if (!_this->bWorldIsReady || _this->MatchState != WaitingToStart)
		return false;

	AFortGameStateAthena* GameState = _this->GameState;
	if (!GameState || !GameState->IsA(AFortGameStateAthena::StaticClass()) || !GameState->bPlaylistDataIsLoaded)
		return false;

	AFortVolumeManager* VolumeManager = GameState->VolumeManager;

	if (!GameState->ArePlaylistLevelsLoaded() || !GameState->AreAllPlaylistLevelsVisible())
		return false;

	if (VolumeManager && VolumeManager->bInSpawningStartup)
		return false;

	return _this->CountReadyPlayers() >= _this->WarmupRequiredPlayerCount;
}

void FortGameMode::Init()
{
	Hooking::Hook<AFortGameModeAthena>(0x808 / 8, ReadyToStartMatch, ReadyToStartMatchOG);

	Hooking::Hook<AFortGameModeAthena>(0xA68 / 8, FinishWorldInitialization, FinishWorldInitializationOG);
	Hooking::Hook<AFortGameModeAthena>(0x670 / 8, HandleStartingNewPlayer, HandleStartingNewPlayerOG);

	Hooking::HookEvery<AFortGameMode>(0x640 / 8, AGameMode::GetDefaultObj()->VTable[0x640 / 8]); // SpawnDefaultPawnFor

	Hooking::Hook(ImageBase + 0x1EC1470, PlaceBotOnTeam, PlaceBotOnTeamOG);
	Hooking::Hook(ImageBase + 0x1ED5710, StartAircraftPhase, StartAircraftPhaseOG);

	Hooking::PatchBytes(ImageBase + 0x1EE37DF, { 0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00 });
}