#include "pch.h"
#include "Core.h"
#include "Components/FortControllerComponent_IndicatedActorManagement.h"
#include "Engine/TimerManager.h"
DEFINE_ENGINE_INIT(FortControllerComponent_IndicatedActorManagement)

UFortControllerComponent_IndicatedActorManagement* GetIndicatedActorComponent(AController* Controller)
{
	if (!Controller)
		return nullptr;

	return Cast<UFortControllerComponent_IndicatedActorManagement>(Controller->GetComponentByClass(UFortControllerComponent_IndicatedActorManagement::StaticClass()));
}

TArray<UFortControllerComponent_IndicatedActorManagement*> GetSquadComponents(UFortControllerComponent_IndicatedActorManagement* _this)
{
	TArray<UFortControllerComponent_IndicatedActorManagement*> Components;

	AFortPlayerControllerAthena* Controller = Cast<AFortPlayerControllerAthena>(_this->GetOwner());
	AFortPlayerStateAthena* PlayerState = Controller ? Cast<AFortPlayerStateAthena>(Controller->PlayerState) : nullptr;

	if (PlayerState && PlayerState->PlayerTeam)
	{
		for (AController* Member : PlayerState->PlayerTeam->TeamMembers)
		{
			if (UFortControllerComponent_IndicatedActorManagement* Component = GetIndicatedActorComponent(Cast<AFortPlayerControllerAthena>(Member)))
				Components.Add(Component);
		}
	}

	if (!Components.Contains(_this))
		Components.Add(_this);

	return Components;
}

template <typename ListType>
bool ContainsActor(const ListType& List, const AActor* Actor)
{
	for (int32 i = 0; i < List.Entries.Num(); i++)
	{
		if (List.Entries[i].Actor == Actor)
			return true;
	}

	return false;
}

template <typename ListType, typename PredicateType>
void RemoveEntries(ListType& List, PredicateType Predicate)
{
	bool bRemoved = false;

	for (int32 i = List.Entries.Num() - 1; i >= 0; i--)
	{
		if (Predicate(List.Entries[i]))
		{
			List.Entries.RemoveAtSwap(i);
			bRemoved = true;
		}
	}

	if (bRemoved)
		List.MarkArrayDirty();
}

template <typename EntryType, typename ListType, typename DataType>
void AddActorsToList(UFortControllerComponent_IndicatedActorManagement* _this, ListType UFortControllerComponent_IndicatedActorManagement::* ListMember, const TArray<AActor*>& Actors, const DataType& Data)
{
	const float TimeSeconds = UGameplayStatics::GetTimeSeconds(GWorld);

	for (AActor* Actor : Actors)
	{
		ListType& List = _this->*ListMember;

		if (!Actor || ContainsActor(List, Actor))
			continue;

		EntryType Entry{};
		Entry.ReplicationID = -1;
		Entry.ReplicationKey = -1;
		Entry.MostRecentArrayReplicationKey = -1;
		Entry.Actor = Actor;
		Entry.StartTime = TimeSeconds;
		Entry.EndTime = Data.Duration > 0.f ? TimeSeconds + Data.Duration : 0.f;
		Entry.Data = Data;

		List.MarkItemDirty(List.Entries.Add(Entry));

		if (Data.Duration <= 0.f)
			continue;

		TWeakObjectPtr<UFortControllerComponent_IndicatedActorManagement> WeakComponent;
		WeakComponent = _this;
		const float EndTime = Entry.EndTime;

		FTimerHandle TimerHandle;
		GWorld->GetTimerManager().SetTimer(TimerHandle, [WeakComponent, ListMember, Actor, EndTime]()
			{
				if (UFortControllerComponent_IndicatedActorManagement* Component = WeakComponent.Get())
					RemoveEntries(Component->*ListMember, [&](const EntryType& Existing) { return Existing.Actor == Actor && Existing.EndTime == EndTime; });
			}, Data.Duration, false);
	}
}

bool GroupIdentifierMatches(const FString& A, const FString& B)
{
	return wcscmp(*A, *B) == 0;
}

void FortControllerComponent_IndicatedActorManagement::AddActorsToIndicatedList(UFortControllerComponent_IndicatedActorManagement* _this, const TArray<AActor*>& IndicatedActors, const FIndicatedActorData& Data, bool bAddAsUnique)
{
	AddActorsToList<FIndicatedActorInfoEntry>(_this, &UFortControllerComponent_IndicatedActorManagement::IndicatedActorList, IndicatedActors, Data);
}

void FortControllerComponent_IndicatedActorManagement::AddActorsToStenciledList(UFortControllerComponent_IndicatedActorManagement* _this, const TArray<AActor*>& StenciledActors, const FStenciledActorData& Data, bool bAddAsUnique)
{
	AddActorsToList<FStenciledActorInfoEntry>(_this, &UFortControllerComponent_IndicatedActorManagement::StenciledActorList, StenciledActors, Data);
}

void FortControllerComponent_IndicatedActorManagement::RemoveActorFromIndicatedList(UFortControllerComponent_IndicatedActorManagement* _this, AActor* IndicatedActor, const bool bIncludeSquad)
{
	TArray<UFortControllerComponent_IndicatedActorManagement*> Components = bIncludeSquad ? GetSquadComponents(_this) : TArray<UFortControllerComponent_IndicatedActorManagement*>();
	if (!bIncludeSquad)
		Components.Add(_this);

	for (UFortControllerComponent_IndicatedActorManagement* Component : Components)
		RemoveEntries(Component->IndicatedActorList, [&](const FIndicatedActorInfoEntry& Entry) { return Entry.Actor == IndicatedActor; });
}

void FortControllerComponent_IndicatedActorManagement::RemoveActorFromStenciledList(UFortControllerComponent_IndicatedActorManagement* _this, AActor* StenciledActor, const bool bIncludeSquad)
{
	TArray<UFortControllerComponent_IndicatedActorManagement*> Components = bIncludeSquad ? GetSquadComponents(_this) : TArray<UFortControllerComponent_IndicatedActorManagement*>();
	if (!bIncludeSquad)
		Components.Add(_this);

	for (UFortControllerComponent_IndicatedActorManagement* Component : Components)
		RemoveEntries(Component->StenciledActorList, [&](const FStenciledActorInfoEntry& Entry) { return Entry.Actor == StenciledActor; });
}

void FortControllerComponent_IndicatedActorManagement::RemoveGroupFromIndicatedList(UFortControllerComponent_IndicatedActorManagement* _this, const FString& GroupIdentifier, const bool bIncludeSquad)
{
	TArray<UFortControllerComponent_IndicatedActorManagement*> Components = bIncludeSquad ? GetSquadComponents(_this) : TArray<UFortControllerComponent_IndicatedActorManagement*>();
	if (!bIncludeSquad)
		Components.Add(_this);

	for (UFortControllerComponent_IndicatedActorManagement* Component : Components)
		RemoveEntries(Component->IndicatedActorList, [&](const FIndicatedActorInfoEntry& Entry) { return GroupIdentifierMatches(Entry.Data.GroupIdentifier, GroupIdentifier); });
}

void FortControllerComponent_IndicatedActorManagement::RemoveGroupFromStenciledList(UFortControllerComponent_IndicatedActorManagement* _this, const FString& GroupIdentifier, const bool bIncludeSquad)
{
	TArray<UFortControllerComponent_IndicatedActorManagement*> Components = bIncludeSquad ? GetSquadComponents(_this) : TArray<UFortControllerComponent_IndicatedActorManagement*>();
	if (!bIncludeSquad)
		Components.Add(_this);

	for (UFortControllerComponent_IndicatedActorManagement* Component : Components)
		RemoveEntries(Component->StenciledActorList, [&](const FStenciledActorInfoEntry& Entry) { return GroupIdentifierMatches(Entry.Data.GroupIdentifier, GroupIdentifier); });
}

void FortControllerComponent_IndicatedActorManagement::execAddActorsToIndicatedList(UObject* Context, FFrame& Stack, void* const Result)
{
	TArray<AActor*> IndicatedActors = Stack.StepCompiledInRef<TArray<AActor*>>();
	FIndicatedActorData Data{};
	Stack.StepCompiledIn(&Data);
	bool bAddAsUnique = Stack.StepCompiledInRef<bool>();
	bool bAllowOwningPlayer = Stack.StepCompiledInRef<bool>();
	Stack.IncrementCode();

	AddActorsToIndicatedList(static_cast<UFortControllerComponent_IndicatedActorManagement*>(Context), IndicatedActors, Data, bAddAsUnique);
}

void FortControllerComponent_IndicatedActorManagement::execAddActorsToStenciledList(UObject* Context, FFrame& Stack, void* const Result)
{
	TArray<AActor*> StenciledActors = Stack.StepCompiledInRef<TArray<AActor*>>();
	FStenciledActorData Data{};
	Stack.StepCompiledIn(&Data);
	bool bAddAsUnique = Stack.StepCompiledInRef<bool>();
	Stack.IncrementCode();

	AddActorsToStenciledList(static_cast<UFortControllerComponent_IndicatedActorManagement*>(Context), StenciledActors, Data, bAddAsUnique);
}

void FortControllerComponent_IndicatedActorManagement::execRemoveActorFromIndicatedList(UObject* Context, FFrame& Stack, void* const Result)
{
	AActor* IndicatedActor = Stack.StepCompiledInRef<AActor*>();
	bool bIncludeSquad = Stack.StepCompiledInRef<bool>();
	Stack.IncrementCode();

	RemoveActorFromIndicatedList(static_cast<UFortControllerComponent_IndicatedActorManagement*>(Context), IndicatedActor, bIncludeSquad);
}

void FortControllerComponent_IndicatedActorManagement::execRemoveActorFromStenciledList(UObject* Context, FFrame& Stack, void* const Result)
{
	AActor* StenciledActor = Stack.StepCompiledInRef<AActor*>();
	bool bIncludeSquad = Stack.StepCompiledInRef<bool>();
	Stack.IncrementCode();

	RemoveActorFromStenciledList(static_cast<UFortControllerComponent_IndicatedActorManagement*>(Context), StenciledActor, bIncludeSquad);
}

void FortControllerComponent_IndicatedActorManagement::execRemoveGroupFromIndicatedList(UObject* Context, FFrame& Stack, void* const Result)
{
	FString GroupIdentifier;
	Stack.StepCompiledIn(&GroupIdentifier);
	bool bIncludeSquad = Stack.StepCompiledInRef<bool>();
	Stack.IncrementCode();

	RemoveGroupFromIndicatedList(static_cast<UFortControllerComponent_IndicatedActorManagement*>(Context), GroupIdentifier, bIncludeSquad);
}

void FortControllerComponent_IndicatedActorManagement::execRemoveGroupFromStenciledList(UObject* Context, FFrame& Stack, void* const Result)
{
	FString GroupIdentifier;
	Stack.StepCompiledIn(&GroupIdentifier);
	bool bIncludeSquad = Stack.StepCompiledInRef<bool>();
	Stack.IncrementCode();

	RemoveGroupFromStenciledList(static_cast<UFortControllerComponent_IndicatedActorManagement*>(Context), GroupIdentifier, bIncludeSquad);
}

void AddActorsInRadiusToList(FFrame& Stack, bool bStenciled)
{
	AFortPlayerControllerAthena* InstigatingController = Stack.StepCompiledInRef<AFortPlayerControllerAthena*>();
	TArray<FIndicatedActorDataWithFilter> FilterDatas;
	Stack.StepCompiledIn(&FilterDatas);
	bool bAddAsUnique = Stack.StepCompiledInRef<bool>();
	Stack.IncrementCode();

	if (!InstigatingController || !InstigatingController->MyFortPawn || InstigatingController->IsA(AFortAthenaAIBotController::StaticClass()) || FilterDatas.Num() == 0)
		return;

	UFortControllerComponent_IndicatedActorManagement* Component = GetIndicatedActorComponent(InstigatingController);
	if (!Component)
		return;

	TArray<UFortControllerComponent_IndicatedActorManagement*> SquadComponents = GetSquadComponents(Component);

	TArray<AActor*> BuildingActors;
	UGameplayStatics::GetAllActorsOfClass(GWorld, ABuildingActor::StaticClass(), &BuildingActors);

	TArray<AActor*> PawnActors;
	UGameplayStatics::GetAllActorsOfClass(GWorld, AFortPawn::StaticClass(), &PawnActors);

	const FVector Origin = InstigatingController->MyFortPawn->K2_GetActorLocation();

	TArray<AActor*> MatchedActors;

	for (const FIndicatedActorDataWithFilter& Data : FilterDatas)
	{
		MatchedActors.Reset();

		const float RadiusSquared = Data.OverlapRadius * Data.OverlapRadius;
		UClass* ActorClassFilter = Data.ActorClassFilter;

		auto TryMatch = [&](AActor* Actor, const FGameplayTagContainer& ActorTags)
			{
				if (!ActorTags.HasAny(Data.IndicatedActorTags))
					return;

				if (ActorClassFilter && !Actor->IsA(ActorClassFilter))
					return;

				const FVector Location = Actor->K2_GetActorLocation();
				const float DeltaX = Location.X - Origin.X;
				const float DeltaY = Location.Y - Origin.Y;
				const float DeltaZ = Location.Z - Origin.Z;

				if (DeltaX * DeltaX + DeltaY * DeltaY + DeltaZ * DeltaZ > RadiusSquared)
					return;

				MatchedActors.Add(Actor);
			};

		for (AActor* Actor : BuildingActors)
		{
			if (Actor)
				TryMatch(Actor, static_cast<ABuildingActor*>(Actor)->StaticGameplayTags);
		}

		for (AActor* Actor : PawnActors)
		{
			if (Actor)
				TryMatch(Actor, static_cast<AFortPawn*>(Actor)->GameplayTags);
		}

		if (MatchedActors.Num() == 0)
			continue;

		for (UFortControllerComponent_IndicatedActorManagement* SquadComponent : SquadComponents)
		{
			if (bStenciled)
				FortControllerComponent_IndicatedActorManagement::AddActorsToStenciledList(SquadComponent, MatchedActors, Data.StenciledData, bAddAsUnique);
			else
				FortControllerComponent_IndicatedActorManagement::AddActorsToIndicatedList(SquadComponent, MatchedActors, Data.IndicatedData, bAddAsUnique);
		}
	}
}

void FortControllerComponent_IndicatedActorManagement::execLibraryAddActorsInRadiusToIndicatedList(UObject* Context, FFrame& Stack, void* const Result)
{
	AddActorsInRadiusToList(Stack, false);
}

void FortControllerComponent_IndicatedActorManagement::execLibraryAddActorsInRadiusToStenciledList(UObject* Context, FFrame& Stack, void* const Result)
{
	AddActorsInRadiusToList(Stack, true);
}

void FortControllerComponent_IndicatedActorManagement::execLibraryAddActorsToIndicatedList(UObject* Context, FFrame& Stack, void* const Result)
{
	AFortPlayerControllerAthena* InstigatingController = Stack.StepCompiledInRef<AFortPlayerControllerAthena*>();
	TArray<AActor*> IndicatedActors;
	Stack.StepCompiledIn(&IndicatedActors);
	FIndicatedActorData Data{};
	Stack.StepCompiledIn(&Data);
	bool bAddAsUnique = Stack.StepCompiledInRef<bool>();
	bool bAllowOwningPlayer = Stack.StepCompiledInRef<bool>();
	Stack.IncrementCode();

	if (UFortControllerComponent_IndicatedActorManagement* Component = GetIndicatedActorComponent(InstigatingController))
		AddActorsToIndicatedList(Component, IndicatedActors, Data, bAddAsUnique);
}

void FortControllerComponent_IndicatedActorManagement::execLibraryAddActorsToStenciledList(UObject* Context, FFrame& Stack, void* const Result)
{
	AFortPlayerControllerAthena* InstigatingController = Stack.StepCompiledInRef<AFortPlayerControllerAthena*>();
	TArray<AActor*> StenciledActors;
	Stack.StepCompiledIn(&StenciledActors);
	FStenciledActorData Data{};
	Stack.StepCompiledIn(&Data);
	bool bAddAsUnique = Stack.StepCompiledInRef<bool>();
	Stack.IncrementCode();

	if (UFortControllerComponent_IndicatedActorManagement* Component = GetIndicatedActorComponent(InstigatingController))
		AddActorsToStenciledList(Component, StenciledActors, Data, bAddAsUnique);
}

void FortControllerComponent_IndicatedActorManagement::execLibraryRemoveActorFromIndicatedList(UObject* Context, FFrame& Stack, void* const Result)
{
	AFortPlayerControllerAthena* InstigatingController = Stack.StepCompiledInRef<AFortPlayerControllerAthena*>();
	AActor* IndicatedActor = Stack.StepCompiledInRef<AActor*>();
	bool bIncludeSquad = Stack.StepCompiledInRef<bool>();
	Stack.IncrementCode();

	if (UFortControllerComponent_IndicatedActorManagement* Component = GetIndicatedActorComponent(InstigatingController))
		RemoveActorFromIndicatedList(Component, IndicatedActor, bIncludeSquad);
}

void FortControllerComponent_IndicatedActorManagement::execLibraryRemoveActorFromStenciledList(UObject* Context, FFrame& Stack, void* const Result)
{
	AFortPlayerControllerAthena* InstigatingController = Stack.StepCompiledInRef<AFortPlayerControllerAthena*>();
	AActor* StenciledActor = Stack.StepCompiledInRef<AActor*>();
	bool bIncludeSquad = Stack.StepCompiledInRef<bool>();
	Stack.IncrementCode();

	if (UFortControllerComponent_IndicatedActorManagement* Component = GetIndicatedActorComponent(InstigatingController))
		RemoveActorFromStenciledList(Component, StenciledActor, bIncludeSquad);
}

void FortControllerComponent_IndicatedActorManagement::execLibraryRemoveGroupFromIndicatedList(UObject* Context, FFrame& Stack, void* const Result)
{
	AFortPlayerControllerAthena* InstigatingController = Stack.StepCompiledInRef<AFortPlayerControllerAthena*>();
	FString GroupIdentifier;
	Stack.StepCompiledIn(&GroupIdentifier);
	bool bIncludeSquad = Stack.StepCompiledInRef<bool>();
	Stack.IncrementCode();

	if (UFortControllerComponent_IndicatedActorManagement* Component = GetIndicatedActorComponent(InstigatingController))
		RemoveGroupFromIndicatedList(Component, GroupIdentifier, bIncludeSquad);
}

void FortControllerComponent_IndicatedActorManagement::execLibraryRemoveGroupFromStenciledList(UObject* Context, FFrame& Stack, void* const Result)
{
	AFortPlayerControllerAthena* InstigatingController = Stack.StepCompiledInRef<AFortPlayerControllerAthena*>();
	FString GroupIdentifier;
	Stack.StepCompiledIn(&GroupIdentifier);
	bool bIncludeSquad = Stack.StepCompiledInRef<bool>();
	Stack.IncrementCode();

	if (UFortControllerComponent_IndicatedActorManagement* Component = GetIndicatedActorComponent(InstigatingController))
		RemoveGroupFromStenciledList(Component, GroupIdentifier, bIncludeSquad);
}

void FortControllerComponent_IndicatedActorManagement::Init()
{
	UClass* ComponentClass = UFortControllerComponent_IndicatedActorManagement::StaticClass();

	Hooking::ExecHook(ComponentClass->GetFunction("AddActorsToIndicatedList"), execAddActorsToIndicatedList);
	Hooking::ExecHook(ComponentClass->GetFunction("AddActorsToStenciledList"), execAddActorsToStenciledList);
	Hooking::ExecHook(ComponentClass->GetFunction("RemoveActorFromIndicatedList"), execRemoveActorFromIndicatedList);
	Hooking::ExecHook(ComponentClass->GetFunction("RemoveActorFromStenciledList"), execRemoveActorFromStenciledList);
	Hooking::ExecHook(ComponentClass->GetFunction("RemoveGroupFromIndicatedList"), execRemoveGroupFromIndicatedList);
	Hooking::ExecHook(ComponentClass->GetFunction("RemoveGroupFromStenciledList"), execRemoveGroupFromStenciledList);

	UClass* LibraryClass = UFortIndicatedActorManagementLibrary::StaticClass();

	Hooking::ExecHook(LibraryClass->GetFunction("AddActorsInRadiusToIndicatedList"), execLibraryAddActorsInRadiusToIndicatedList);
	Hooking::ExecHook(LibraryClass->GetFunction("AddActorsInRadiusToStenciledList"), execLibraryAddActorsInRadiusToStenciledList);
	Hooking::ExecHook(LibraryClass->GetFunction("AddActorsToIndicatedList"), execLibraryAddActorsToIndicatedList);
	Hooking::ExecHook(LibraryClass->GetFunction("AddActorsToStenciledList"), execLibraryAddActorsToStenciledList);
	Hooking::ExecHook(LibraryClass->GetFunction("RemoveActorFromIndicatedList"), execLibraryRemoveActorFromIndicatedList);
	Hooking::ExecHook(LibraryClass->GetFunction("RemoveActorFromStenciledList"), execLibraryRemoveActorFromStenciledList);
	Hooking::ExecHook(LibraryClass->GetFunction("RemoveGroupFromIndicatedList"), execLibraryRemoveGroupFromIndicatedList);
	Hooking::ExecHook(LibraryClass->GetFunction("RemoveGroupFromStenciledList"), execLibraryRemoveGroupFromStenciledList);
}
