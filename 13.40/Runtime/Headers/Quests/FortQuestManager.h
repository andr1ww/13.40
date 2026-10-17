#pragma once
#include "pch.h"

namespace FortQuestManager
{
	FString GetStatName(FName BackendName);
	int32 GetChangesInPendingSaveRequests(UFortQuestManager* _this, const FString& StatName);
	FFortQuestObjectiveCompletion* GetPendingChange(UFortQuestManager* _this, const FString& StatName);
	FFortQuestObjectiveCompletion* AddPendingChange(UFortQuestManager* _this, FName BackendName);
	bool CanProgressObjective(UFortQuestManager* _this, AFortPlayerController* PlayerController, FFortMcpQuestObjectiveInfo& Objective, EFortQuestObjectiveStatEvent StatEvent, const UObject* TargetObject, const FGameplayTagContainer& TargetTags, const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& ContextTags, const FString& ContextString);
	bool UpdateObjective(UFortQuestManager* _this, AFortPlayerController* PlayerController, UFortQuestItem* QuestItem, UFortQuestItemDefinition* QuestDefinition, const FFortMcpQuestObjectiveInfo& Objective, int32 Count, bool bAddToPendingChange, bool& bObjectiveCompleted, bool& bQuestCompleted);
	void GrantQuestXP(UFortQuestManager* _this, AFortPlayerController* PlayerController, UFortQuestItem* QuestItem);
	UDataTable* GetObjectiveStatXPTable();
	void GrantObjectiveStatXP(UFortQuestManager* _this, AFortPlayerController* PlayerController, EFortQuestObjectiveStatEvent StatEvent, const UObject* TargetObject, const FGameplayTagContainer& TargetTags, const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& ContextTags, int32 Count);
	int32 GrantAccolade(UFortQuestManager* _this, UFortPlayerControllerAthenaXPComponent* XPComponent, UFortAccoladeItemDefinition* Accolade, const FPrimaryAssetId& AccoladeId);
	void SaveStatEvents(UFortQuestManager* _this);
	void SendStatEventsAndUpdateQuests(UFortQuestManager* _this);
	void EndBattleRoyaleMatchForPlayer(UFortQuestManager* _this, UFortPlaylistAthena* Playlist, const FAthenaMatchStats& MatchStats);
	void SendStatEventInternal(UFortQuestManager* _this, const FDataTableRowHandle& ObjectiveStat, EFortQuestObjectiveStatEvent StatEvent, const UObject* TargetObject, const FGameplayTagContainer& TargetTags, const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& ContextTags, int32 Count, bool bFlushEvents, bool* QuestActive = nullptr, bool* QuestCompleted = nullptr);
	void SendStatEventWithTags(UFortQuestManager* _this, EFortQuestObjectiveStatEvent StatEvent, const UObject* TargetObject, const FGameplayTagContainer& TargetTags, const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& ContextTags, int32 Count, bool bFlushEvents);
	void SendComplexCustomStatEvent(UFortQuestManager* _this, UObject* TargetObject, FGameplayTagContainer AdditionalSourceTags, FGameplayTagContainer TargetTags, bool& QuestActive, bool& QuestCompleted, int32 Count);
	void SendCustomStatEvent(UFortQuestManager* _this, const FDataTableRowHandle& ObjectiveStat, int32 Count, bool bForceFlush);
	void SendCustomStatEventDirect(UFortQuestManager* _this, FName ObjectiveBackendName, UFortQuestItem* QuestItem, int32 Count, bool bForceFlush);
	void InitializeQuestAbilities(UFortQuestManager* _this, APawn* PlayerPawn);
	void execInitializeQuestAbilities(UObject* Context, FFrame& Stack, void* const Result);
	DefineHook(void, InitQuests, UFortQuestManager* _this, bool bFromMCP, bool bFromUpdate);

	void Init();
}
