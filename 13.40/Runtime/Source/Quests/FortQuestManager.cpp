#include "pch.h"
#include "Core.h"
#include "Quests/FortQuestManager.h"
DEFINE_ENGINE_INIT(FortQuestManager)

FString FortQuestManager::GetStatName(FName BackendName)
{
	FString StatName = BackendName.ToString();

	for (int32 i = 0; i < StatName.Num(); i++)
		StatName[i] = towlower(StatName[i]);

	return StatName;
}

int32 FortQuestManager::GetChangesInPendingSaveRequests(UFortQuestManager* _this, const FString& StatName)
{
	FFortQuestObjectiveCompletion* PendingChange = GetPendingChange(_this, StatName);

	return PendingChange ? PendingChange->Count : 0;
}

FFortQuestObjectiveCompletion* FortQuestManager::GetPendingChange(UFortQuestManager* _this, const FString& StatName)
{
	for (FFortQuestObjectiveCompletion& PendingChange : _this->PendingChanges)
	{
		if (PendingChange.StatName == StatName)
			return &PendingChange;
	}

	return nullptr;
}

FFortQuestObjectiveCompletion* FortQuestManager::AddPendingChange(UFortQuestManager* _this, FName BackendName)
{
	FFortQuestObjectiveCompletion PendingChange{};
	PendingChange.StatName = GetStatName(BackendName);

	return &_this->PendingChanges.Add(std::move(PendingChange));
}

bool FortQuestManager::CanProgressObjective(UFortQuestManager* _this, AFortPlayerController* PlayerController, FFortMcpQuestObjectiveInfo& Objective, EFortQuestObjectiveStatEvent StatEvent, const UObject* TargetObject, const FGameplayTagContainer& TargetTags, const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& ContextTags, const FString& ContextString)
{
	FFortQuestFilterContext Context(TargetObject, _this->BoundProfile.Get(), PlayerController, nullptr, &SourceTags, &TargetTags, &ContextTags, false);

	if (Objective.InlineObjectiveStats.Num() > 0)
	{
		for (FFortQuestObjectiveStat& Stat : Objective.InlineObjectiveStats)
		{
			Stat.CacheData();

			if (Stat.CanProgress(StatEvent, TargetTags, SourceTags, ContextTags) && Context.EvaluateCondition(Stat.GetCompiledCondition()))
				return true;
		}

		return false;
	}

	if (!UFortQuestManager::CanProgressQuestObjective(Objective, StatEvent, TargetTags, SourceTags, ContextTags, ContextString))
		return false;

	FFortQuestObjectiveStatTableRow* ObjectiveRow = FindRow<FFortQuestObjectiveStatTableRow>(Objective.ObjectiveStatHandle.DataTable, Objective.ObjectiveStatHandle.RowName);

	if (!ObjectiveRow)
		return true;

	ObjectiveRow->CacheData();

	return Context.EvaluateCondition(ObjectiveRow->GetCompiledCondition());
}

bool FortQuestManager::UpdateObjective(UFortQuestManager* _this, AFortPlayerController* PlayerController, UFortQuestItem* QuestItem, UFortQuestItemDefinition* QuestDefinition, const FFortMcpQuestObjectiveInfo& Objective, int32 Count, bool bAddToPendingChange, bool& bObjectiveCompleted, bool& bQuestCompleted)
{
	UFortQuestObjectiveInfo* ObjectiveInfo = QuestItem->GetObjectiveInfoWithName(Objective.BackendName);
	FString StatName = GetStatName(Objective.BackendName);

	int32 CurrentCount = ObjectiveInfo ? ObjectiveInfo->AchievedCount : 0;
	bool bOncePerMatch = QuestDefinition->GetAthenaUpdateObjectiveOncePerMatch() || QuestDefinition->GetUpdateObjectiveOncePerMatch();

	if (CurrentCount >= Objective.Count || (bOncePerMatch && GetChangesInPendingSaveRequests(_this, StatName) > 0))
		return false;

	FFortQuestObjectiveCompletion* PendingChange = GetPendingChange(_this, StatName);

	if (bAddToPendingChange)
	{
		if (!PendingChange)
			PendingChange = AddPendingChange(_this, Objective.BackendName);

		PendingChange->Count += Count;
	}

	if (!PendingChange)
		return false;

	int32 NewCount = min(CurrentCount + Count, Objective.Count);

	bObjectiveCompleted = NewCount >= Objective.Count;

	if (ObjectiveInfo)
		ObjectiveInfo->AchievedCount = NewCount;

	bQuestCompleted = bObjectiveCompleted && QuestItem->GetNumObjectivesComplete() >= QuestItem->Objectives.Num();

	_this->HandleQuestUpdated(PlayerController, QuestDefinition, Objective.BackendName, NewCount, Count, nullptr, bObjectiveCompleted, bQuestCompleted);

	return true;
}

void FortQuestManager::SaveStatEvents(UFortQuestManager* _this)
{
	UFortMcpProfileSubgame* Profile = _this->BoundProfile.Get();

	if (!Profile)
		return;

	FString DebugName = _this->GetDebugName();

	for (const FFortQuestObjectiveCompletion& PendingChange : _this->PendingChanges)
		UE_LOG(LogFortQuest, Log, TEXT("SaveStatEvents for %s: %s:%d"), *DebugName, *PendingChange.StatName, PendingChange.Count);

	TArray<FFortQuestObjectiveCompletion> Advance = std::move(_this->PendingChanges);
	FDedicatedServerUrlContext Context{};

	Profile->UpdateQuests(Advance, &Context);
}

void FortQuestManager::SendStatEventsAndUpdateQuests(UFortQuestManager* _this)
{
	if (_this->PendingChanges.Num() != 0)
		SaveStatEvents(_this);
}

void FortQuestManager::EndBattleRoyaleMatchForPlayer(UFortQuestManager* _this, UFortPlaylistAthena* Playlist, const FAthenaMatchStats& MatchStats)
{
	UFortMcpProfileAthena* Profile = Cast<UFortMcpProfileAthena>(_this->BoundProfile.Get());

	if (!Profile)
	{
		UE_LOG(LogFortQuest, Warning, TEXT("EndBattleRoyaleMatchForPlayer: no Athena profile found %s"), *_this->GetDebugName());
		return;
	}

	if (!Playlist)
		return;

	UFortRegisteredPlayerInfo* PlayerInfo = Cast<UFortRegisteredPlayerInfo>(_this->Outer);
	AFortPlayerControllerAthena* PlayerController = PlayerInfo ? Cast<AFortPlayerControllerAthena>(PlayerInfo->GetPlayerController()) : nullptr;
	UFortPlayerControllerAthenaXPComponent* XPComponent = PlayerController ? PlayerController->XPComponent : nullptr;

	int32 TotalXPAccum = XPComponent ? XPComponent->TotalXpEarned : 0;
	float AntiAddictionPlayTimeMultiplier = PlayerController ? PlayerController->AntiAddictionPlayTimeMultiplier : 1.0f;

	FString PlaylistId = Playlist->PlaylistName.ToString();
	TArray<FFortQuestObjectiveCompletion> Advance = std::move(_this->PendingChanges);
	TArray<FAthenaAccolades> NoAccolades{};
	TArray<FString> ShuffledLoadoutUsed{};
	TArray<FString> GrantedQuestDefs{};
	TArray<FSecondaryXpGained> SecondaryXp{};
	FDedicatedServerUrlContext Context{};

	Profile->EndBattleRoyaleGame(Advance, PlaylistId, MatchStats, TotalXPAccum, 0, Profile->GetSeasonFriendMatchXpBoost(), Profile->GetSeasonMatchXpBoost(), AntiAddictionPlayTimeMultiplier, Playlist->bAccumulateToProfileStats, Playlist->bSaveToRecentGameList, XPComponent ? XPComponent->PlayerAccolades : NoAccolades, ShuffledLoadoutUsed, 0, GrantedQuestDefs, SecondaryXp, &Context);
}

void FortQuestManager::SendStatEventInternal(UFortQuestManager* _this, const FDataTableRowHandle& ObjectiveStat, EFortQuestObjectiveStatEvent StatEvent, const UObject* TargetObject, const FGameplayTagContainer& TargetTags, const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& ContextTags, int32 Count, bool bFlushEvents, bool* QuestActive, bool* QuestCompleted)
{
	UFortRegisteredPlayerInfo* PlayerInfo = Cast<UFortRegisteredPlayerInfo>(_this->Outer);
	AFortPlayerController* PlayerController = PlayerInfo ? PlayerInfo->GetPlayerController() : nullptr;

	if (!PlayerController)
	{
		UE_LOG(LogFortQuest, Warning, TEXT("SendStatEventWithTags: Cannot be called, %s is no longer registered!"), *_this->GetDebugName());
		return;
	}

	if (PlayerController->Role != ENetRole::ROLE_Authority)
	{
		UE_LOG(LogFortQuest, Warning, TEXT("SendStatEventWithTags: Cannot be called on the client!"));
		return;
	}

	bool bObjectiveStatIsNull = !ObjectiveStat.DataTable && ObjectiveStat.RowName.IsNone();

	if (bObjectiveStatIsNull && StatEvent >= EFortQuestObjectiveStatEvent::NumGameplayEvents)
	{
		UE_LOG(LogFortQuest, Warning, TEXT("SendStatEventWithTags: Cannot be called with stat type %d!"), static_cast<int32>(StatEvent));
		return;
	}

	UFortItemDefinition* TargetItemDefinition = Cast<UFortItemDefinition>(const_cast<UObject*>(TargetObject));

	std::vector<FName> ObjectivesUpdated;
	bool bFlush = bFlushEvents;
	bool bAnyQuestCompleted = false;

	for (int32 QuestIndex = 0; QuestIndex < _this->CurrentQuests.Num(); QuestIndex++)
	{
		UFortQuestItem* QuestItem = _this->CurrentQuests[QuestIndex];

		if (!QuestItem || QuestItem->quest_state == EFortQuestState::Completed || QuestItem->quest_state == EFortQuestState::Claimed)
			continue;

		UFortQuestItemDefinition* QuestDefinition = Cast<UFortQuestItemDefinition>(QuestItem->ItemDefinition);

		if (!QuestDefinition)
			continue;

		FString ContextString = QuestDefinition->Name.ToString();

		for (FFortMcpQuestObjectiveInfo& Objective : QuestDefinition->Objectives)
		{
			if (Objective.Stage != -1 && Objective.Stage != QuestItem->CurrentStage)
				continue;

			bool bAlreadyUpdated = std::find(ObjectivesUpdated.begin(), ObjectivesUpdated.end(), Objective.BackendName) != ObjectivesUpdated.end();

			if (!bAlreadyUpdated)
			{
				bool bMatches = false;

				if (bObjectiveStatIsNull)
				{
					if (Objective.ItemEvent == EFortQuestObjectiveItemEvent::Max_None)
					{
						bMatches = CanProgressObjective(_this, PlayerController, Objective, StatEvent, TargetObject, TargetTags, SourceTags, ContextTags, ContextString);
					}
					else if (TargetItemDefinition)
					{
						UFortQuestObjectiveInfo* ObjectiveInfo = QuestItem->GetObjectiveInfoWithName(Objective.BackendName);

						bMatches = ObjectiveInfo && ObjectiveInfo->MatchesStatEvent(StatEvent) && TargetItemDefinition == Objective.ItemReference.LoadSynchronous();
					}
				}
				else if (Objective.InlineObjectiveStats.Num() > 0 || (Objective.ObjectiveStatHandle.DataTable == ObjectiveStat.DataTable && Objective.ObjectiveStatHandle.RowName == ObjectiveStat.RowName))
				{
					bMatches = CanProgressObjective(_this, PlayerController, Objective, StatEvent, TargetObject, TargetTags, SourceTags, ContextTags, ContextString);
				}

				if (!bMatches)
					continue;
			}

			bool bObjectiveCompleted = false;
			bool bQuestCompleted = false;

			if (!UpdateObjective(_this, PlayerController, QuestItem, QuestDefinition, Objective, Count, !bAlreadyUpdated, bObjectiveCompleted, bQuestCompleted))
				continue;

			if (!bAlreadyUpdated)
				ObjectivesUpdated.push_back(Objective.BackendName);

			bFlush |= bObjectiveCompleted;
			bAnyQuestCompleted |= bQuestCompleted;
		}
	}

	if (QuestActive)
		*QuestActive = !ObjectivesUpdated.empty();

	if (QuestCompleted)
		*QuestCompleted = bAnyQuestCompleted;

	if (ObjectivesUpdated.empty())
		return;

	_this->UpdateQuests();

	if (!bFlush)
		return;

	UE_LOG(LogFortQuest, Verbose, TEXT("SendStatEvent for %s: %d quests listening to flushed event %s %d %s %s %s"), *_this->GetDebugName(), static_cast<int32>(ObjectivesUpdated.size()), *FString(ObjectiveStat.RowName.ToString()), static_cast<int32>(StatEvent), *TargetTags.ToStringSimple(false), *SourceTags.ToStringSimple(false), *ContextTags.ToStringSimple(false));

	SendStatEventsAndUpdateQuests(_this);
	PlayerController->ClientForceProfileQuery();
}

void FortQuestManager::SendStatEventWithTags(UFortQuestManager* _this, EFortQuestObjectiveStatEvent StatEvent, const UObject* TargetObject, const FGameplayTagContainer& TargetTags, const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& ContextTags, int32 Count, bool bFlushEvents)
{
	SendStatEventInternal(_this, FDataTableRowHandle{}, StatEvent, TargetObject, TargetTags, SourceTags, ContextTags, Count, bFlushEvents);
}

void FortQuestManager::SendComplexCustomStatEvent(UFortQuestManager* _this, UObject* TargetObject, FGameplayTagContainer AdditionalSourceTags, FGameplayTagContainer TargetTags, bool& QuestActive, bool& QuestCompleted, int32 Count)
{
	QuestActive = false;
	QuestCompleted = false;

	FGameplayTagContainer SourceTags{};
	FGameplayTagContainer ContextTags{};

	_this->GetSourceAndContextTags(&SourceTags, &ContextTags);
	SourceTags.AppendTags(AdditionalSourceTags);

	SendStatEventInternal(_this, FDataTableRowHandle{}, EFortQuestObjectiveStatEvent::ComplexCustom, TargetObject, TargetTags, SourceTags, ContextTags, Count, false, &QuestActive, &QuestCompleted);
}

void FortQuestManager::SendCustomStatEvent(UFortQuestManager* _this, const FDataTableRowHandle& ObjectiveStat, int32 Count, bool bForceFlush)
{
	FFortQuestObjectiveStatTableRow* ObjectiveRow = FindRow<FFortQuestObjectiveStatTableRow>(ObjectiveStat.DataTable, ObjectiveStat.RowName);

	if (!ObjectiveRow || ObjectiveRow->Type != EFortQuestObjectiveStatEvent::Custom)
	{
		UE_LOG(LogFortQuest, Warning, TEXT("SendCustomStatEvent: Called with invalid or non custom stat %s!"), *FString(ObjectiveStat.RowName.ToString()));
		return;
	}

	FGameplayTagContainer EmptyTags{};

	SendStatEventInternal(_this, ObjectiveStat, EFortQuestObjectiveStatEvent::Custom, nullptr, EmptyTags, EmptyTags, EmptyTags, Count, bForceFlush);
}

void FortQuestManager::SendCustomStatEventDirect(UFortQuestManager* _this, FName ObjectiveBackendName, UFortQuestItem* QuestItem, int32 Count, bool bForceFlush)
{
	UFortRegisteredPlayerInfo* PlayerInfo = Cast<UFortRegisteredPlayerInfo>(_this->Outer);
	AFortPlayerController* PlayerController = PlayerInfo ? PlayerInfo->GetPlayerController() : nullptr;

	if (!PlayerController || PlayerController->Role != ENetRole::ROLE_Authority || !QuestItem || QuestItem->quest_state == EFortQuestState::Completed || QuestItem->quest_state == EFortQuestState::Claimed)
		return;

	UFortQuestItemDefinition* QuestDefinition = Cast<UFortQuestItemDefinition>(QuestItem->ItemDefinition);

	if (!QuestDefinition)
		return;

	for (const FFortMcpQuestObjectiveInfo& Objective : QuestDefinition->Objectives)
	{
		if (Objective.BackendName != ObjectiveBackendName || (Objective.Stage != -1 && Objective.Stage != QuestItem->CurrentStage))
			continue;

		bool bObjectiveCompleted = false;
		bool bQuestCompleted = false;

		if (!UpdateObjective(_this, PlayerController, QuestItem, QuestDefinition, Objective, Count, true, bObjectiveCompleted, bQuestCompleted))
			return;

		_this->UpdateQuests();

		if (bForceFlush || bObjectiveCompleted)
		{
			SendStatEventsAndUpdateQuests(_this);
			PlayerController->ClientForceProfileQuery();
		}

		return;
	}
}

void FortQuestManager::InitializeQuestAbilities(UFortQuestManager* _this, APawn* PlayerPawn)
{
	for (FFortAbilitySetHandle& Handle : _this->EquippedQuestAbilities)
		UFortKismetLibrary::UnequipFortAbilitySet(Handle);

	_this->EquippedQuestAbilities.Free();

	if (!PlayerPawn)
		return;

	TScriptInterface<IAbilitySystemInterface> AbilitySystemInterface{};
	AbilitySystemInterface.ObjectPointer = PlayerPawn;
	AbilitySystemInterface.InterfacePointer = GetInterfaceAddress<IAbilitySystemInterface>(PlayerPawn);

	if (!AbilitySystemInterface.InterfacePointer)
		return;

	for (UFortQuestItem* QuestItem : _this->CurrentQuests)
	{
		if (!QuestItem || QuestItem->quest_state == EFortQuestState::Completed || QuestItem->quest_state == EFortQuestState::Claimed)
			continue;

		UFortQuestItemDefinition* QuestDefinition = Cast<UFortQuestItemDefinition>(QuestItem->ItemDefinition);
		UFortAbilitySet* AbilitySet = QuestDefinition ? QuestDefinition->QuestAbilitySet.LoadSynchronous() : nullptr;

		if (AbilitySet)
			_this->EquippedQuestAbilities.Add(UFortKismetLibrary::EquipFortAbilitySet(AbilitySystemInterface, AbilitySet, QuestItem));
	}
}

void FortQuestManager::execInitializeQuestAbilities(UObject* Context, FFrame& Stack, void* const Result)
{
	APawn* PlayerPawn = Stack.StepCompiledInRef<APawn*>();
	Stack.IncrementCode();

	InitializeQuestAbilities(static_cast<UFortQuestManager*>(Context), PlayerPawn);
}

void FortQuestManager::InitQuests(UFortQuestManager* _this, bool bFromMCP, bool bFromUpdate)
{
	InitQuestsOG(_this, bFromMCP, bFromUpdate);

	UFortRegisteredPlayerInfo* PlayerInfo = Cast<UFortRegisteredPlayerInfo>(_this->Outer);
	AFortPlayerController* PlayerController = PlayerInfo ? PlayerInfo->GetPlayerController() : nullptr;

	if (PlayerController && PlayerController->MyFortPawn)
		InitializeQuestAbilities(_this, PlayerController->MyFortPawn);
}

void FortQuestManager::Init()
{
	std::vector<uintptr_t> SendStatEventWithTagsCallers = {
		ImageBase + 0x1EBC6D0,
		ImageBase + 0x1ECC8B1,
		ImageBase + 0x1F071F7,
		ImageBase + 0x1F7FE87,
		ImageBase + 0x1F89E1C,
		ImageBase + 0x1F89E9F,
		ImageBase + 0x1FC1679,
		ImageBase + 0x1FC17A6,
		ImageBase + 0x2182D90,
		ImageBase + 0x21BFE91,
		ImageBase + 0x225A980,
		ImageBase + 0x2288DFB,
		ImageBase + 0x2477281,
		ImageBase + 0x247782D,
		ImageBase + 0x247805F,
		ImageBase + 0x26B7D85,
		ImageBase + 0x28E8767,
		ImageBase + 0x28E87F2,
		ImageBase + 0x2934AB6,
		ImageBase + 0x29C20A3,
		ImageBase + 0x29C20D3,
	};

	for (uintptr_t Caller : SendStatEventWithTagsCallers)
		Hooking::Rel32Swap(Caller, SendStatEventWithTags);

	Hooking::Hook(ImageBase + 0x2A1D7F0, InitQuests, InitQuestsOG);
	Hooking::ExecHook(UFortQuestManager::StaticClass()->GetFunction("FortQuestManager", "InitializeQuestAbilities"), execInitializeQuestAbilities);
	Hooking::Hook(ImageBase + 0x2A286D0, SendComplexCustomStatEvent);
	Hooking::Hook(ImageBase + 0x2A287A0, SendCustomStatEvent);
	Hooking::Hook(ImageBase + 0x2A28870, SendCustomStatEventDirect);
}
