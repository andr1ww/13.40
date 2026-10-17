#include "pch.h"
#include "Core.h"
#include "Athena/FortGameStateAthena.h"
DEFINE_ENGINE_INIT(FortGameStateAthena)

void AFortGameStateAthena::AddPlayerToTeamLists(AFortPlayerStateAthena* PlayerState)
{
	using FPlayerList = TArray<TWeakObjectPtr<AFortPlayerStateAthena>>;

	auto FindOrAddPlayerList = reinterpret_cast<FPlayerList*(*)(void*, int32)>(ImageBase + 0x1EF7F00);
	auto AddPlayer = reinterpret_cast<void(*)(FPlayerList*, AFortPlayerStateAthena*)>(ImageBase + 0x1EF0060);

	AddPlayer(FindOrAddPlayerList(reinterpret_cast<uint8*>(this) + 0x1548, PlayerState->TeamIndex), PlayerState);
	AddPlayer(FindOrAddPlayerList(reinterpret_cast<uint8*>(this) + 0x1580, PlayerState->SquadId), PlayerState);
}

void FortGameStateAthena::Init()
{
	Hooking::Patch<uint8_t>(ImageBase + 0x1f196c3, 0x85);
}
