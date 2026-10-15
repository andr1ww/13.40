#include "pch.h"
#include "Core.h"
#include "GameModes/FortGameMode.h"
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

void FortGameMode::InitGameState(AFortGameModeAthena* _this)
{
	InitGameStateOG(_this);

	if (_this->CurrentPlaylistName.ComparisonIndex == 0)
		_this->SetCurrentPlaylistName(FName("Playlist_DefaultSolo"));

	UFortPlaylistAthena* Playlist = _this->GetPlaylistManager()->GetPlaylistByName(_this->CurrentPlaylistName);

	if (!Playlist)
		return;

	_this->WarmupRequiredPlayerCount = 1;

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
}

void FortGameMode::FinishWorldInitialization(AFortGameModeAthena* _this, AActor* WorldManager)
{
	_this->bWorldIsReady = true;
	FinishWorldInitializationOG(_this, WorldManager);

	InitWorldManager(_this, Cast<AFortWorldManager>(WorldManager));

	std::wstring Title = L"13.40 | Ready | Built: ";
	Title += std::wstring(L"" __DATE__ L" " __TIME__);

	SetConsoleTitleW(Title.c_str());
	UE_LOG(LogFort, Log, TEXT("FortGameMode::FinishWorldInitialization: %s"), *FString(Title.c_str()));

	_this->GameState->OnRep_CurrentPlaylistInfo();
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

	Hooking::Hook<AFortGameModeAthena>(0x690 / 8, InitGameState, InitGameStateOG);
	Hooking::Hook<AFortGameModeAthena>(0xA68 / 8, FinishWorldInitialization, FinishWorldInitializationOG);
	Hooking::Hook<AFortGameModeAthena>(0x670 / 8, HandleStartingNewPlayer, HandleStartingNewPlayerOG);

	Hooking::HookEvery<AFortGameMode>(0x640 / 8, AGameMode::GetDefaultObj()->VTable[0x640 / 8]); // SpawnDefaultPawnFor
}