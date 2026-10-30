#include "pch.h"
#include "Core.h"
#include "Athena/FortAthenaLivingWorldManager.h"
#include "Engine/TimerManager.h"
#include "SDK/SDK/LagerRuntime_parameters.hpp"
DEFINE_ENGINE_INIT(FortAthenaLivingWorldManager)

template <typename T>
T& Field(const void* Object, uint32 Offset)
{
	return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(Object) + Offset);
}

int32& PendingLoads(UFortAthenaLivingWorldManager* _this) { return Field<int32>(_this, 0x250); }
TArray<FortAthenaLivingWorldManager::FLivingWorldEventRuntimeData>& EventRuntimeData(UFortAthenaLivingWorldManager* _this) { return Field<TArray<FortAthenaLivingWorldManager::FLivingWorldEventRuntimeData>>(_this, 0x268); }
float& NextGenerationTime(UFortAthenaLivingWorldManager* _this) { return Field<float>(_this, 0x2B8); }
FGameplayTagContainer& CachedPlaylistContextTags(UFortAthenaLivingWorldManager* _this) { return Field<FGameplayTagContainer>(_this, 0x2C0); }
bool& bVerboseLogging(UFortAthenaLivingWorldManager* _this) { return Field<bool>(_this, 0x2E0); }
bool& bGenerateEvents(UFortAthenaLivingWorldManager* _this) { return Field<bool>(_this, 0x2E1); }

bool MatchesTagQuery(const FGameplayTagContainer& Tags, const FGameplayTagQuery& Query)
{
	return Query.QueryTokenStream.Num() == 0 || UBlueprintGameplayTagLibrary::DoesContainerMatchTagQuery(Tags, Query);
}

template <typename T>
void BindToPlaylistLoaded(T* Object)
{
	AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(GWorld->GameState);

	if (Object == NULL || GameState == NULL)
		return;

	if (!GameState->bPlaylistDataIsLoaded)
	{
		FScriptDelegate Delegate;
		Delegate.BindUFunction(Object, FName(L"OnCurrentPlaylistLoaded"));
		GameState->OnCurrentPlaylistLoaded.AddUnique(Delegate);
		return;
	}

	UFortPlaylistAthena* Playlist = GameState->CurrentPlaylistInfo.OverridePlaylist ? GameState->CurrentPlaylistInfo.OverridePlaylist : GameState->CurrentPlaylistInfo.BasePlaylist;

	FGameplayTagContainer PlaylistContextTags{};
	reinterpret_cast<FGameplayTagContainer*(*)(AFortGameStateAthena*, FGameplayTagContainer*)>(ImageBase + 0x1EF97D0)(GameState, &PlaylistContextTags);

	Object->OnCurrentPlaylistLoaded(Playlist ? Playlist->PlaylistName : FName(), PlaylistContextTags);
}

UFortAthenaLivingWorldManager* FortAthenaLivingWorldManager::GetLivingWorldManager()
{
	AFortGameStateAthena* GameState = GWorld ? Cast<AFortGameStateAthena>(GWorld->GameState) : NULL;
	return GameState ? Cast<UFortAthenaLivingWorldManager>(GameState->GetComponentByClass(UFortAthenaLivingWorldManager::StaticClass())) : NULL;
}

void FortAthenaLivingWorldManager::RegisterPointProvider(AActor* PointProvider)
{
	UFortAthenaLivingWorldManager* Manager = GetLivingWorldManager();

	if (Manager && PointProvider && !Manager->PointProviders.Contains(PointProvider))
		Manager->PointProviders.Add(PointProvider);
}

void FortAthenaLivingWorldManager::UnregisterPointProvider(AActor* PointProvider)
{
	if (UFortAthenaLivingWorldManager* Manager = GetLivingWorldManager())
		Manager->PointProviders.Remove(PointProvider);
}

bool FortAthenaLivingWorldManager::GetProviderLocation(AActor* PointProvider, FVector& OutLocation, FRotator& OutRotation)
{
	if (PointProvider->IsA<AFortAthenaLivingWorldVolume>())
		return reinterpret_cast<bool(*)(void*, FVector*, FRotator*)>(ImageBase + 0x1221550)(reinterpret_cast<uint8*>(PointProvider) + 0x258, &OutLocation, &OutRotation);

	UFunction* Func = PointProvider->Class->FindFunctionByName(FName(L"GetValidLocation"));

	if (Func == NULL)
		return false;

	Params::FortAthenaLivingWorldPointProviderInterface_GetValidLocation Parms{};
	PointProvider->ProcessEvent(Func, &Parms);

	OutLocation = Parms.OutPosition;
	OutRotation = Parms.OutRotation;

	return Parms.ReturnValue;
}

AActor* FortAthenaLivingWorldManager::PickPointProvider(UFortAthenaLivingWorldManager* _this, const FGameplayTagQuery& FiltersTagQuery, FVector& OutLocation, FRotator& OutRotation)
{
	TArray<AActor*> Candidates;

	for (int32 i = 0; i < _this->PointProviders.Num(); i++)
	{
		AActor* PointProvider = _this->PointProviders[i];

		if (PointProvider == NULL)
			continue;

		FGameplayTagContainer FiltersTags{};

		if (AFortAthenaLivingWorldVolume* Volume = Cast<AFortAthenaLivingWorldVolume>(PointProvider))
			FiltersTags = Volume->FiltersTags;
		else if (UFunction* Func = PointProvider->Class->FindFunctionByName(FName(L"GetFiltersTags")))
		{
			Params::FortAthenaLivingWorldPointProviderInterface_GetFiltersTags Parms{};
			PointProvider->ProcessEvent(Func, &Parms);
			FiltersTags = Parms.FilterTags;
		}

		if (MatchesTagQuery(FiltersTags, FiltersTagQuery))
			Candidates.Add(PointProvider);
	}

	AActor* Result = NULL;

	while (Result == NULL && Candidates.Num() > 0)
	{
		const int32 Index = min((int32)(FRand(nullptr) * Candidates.Num()), Candidates.Num() - 1);

		if (GetProviderLocation(Candidates[Index], OutLocation, OutRotation))
			Result = Candidates[Index];

		Candidates.Remove(Index);
	}

	return Result;
}

bool FortAthenaLivingWorldManager::IsEventAvailable(UFortAthenaLivingWorldManager* _this, FFortAthenaLivingWorldEvent* Event)
{
	UFortAthenaLivingWorldEventData* EventData = Event->EventData.LoadSynchronous();

	if (EventData == NULL || !UBlueprintGameplayTagLibrary::HasAllTags(CachedPlaylistContextTags(_this), Event->RequirePlaylistTags, false))
		return false;

	if (EventData->TimeOfDayFilters != 0)
	{
		AFortTimeOfDayManager* TimeOfDayManager = AFortTimeOfDayManager::GetTimeOfDayManagerFromContext(GWorld);

		if (TimeOfDayManager && (EventData->TimeOfDayFilters & (1 << (int32)TimeOfDayManager->CurrentDayNightPhase)) == 0)
			return false;
	}

	if (uint8* WaterLevel = reinterpret_cast<uint8*(*)(UWorld*)>(ImageBase + 0xFA78C0)(GWorld))
	{
		const float WaterLevelIndex = *reinterpret_cast<float*>(WaterLevel + 0x7C) / 384.f;

		if ((EventData->WaterLevelIndexMin != -1.f && WaterLevelIndex < EventData->WaterLevelIndexMin) || (EventData->WaterLevelIndexMax != -1.f && WaterLevelIndex > EventData->WaterLevelIndexMax))
			return false;
	}

	const float TimeSeconds = UGameplayStatics::GetTimeSeconds(GWorld);
	TArray<FLivingWorldEventRuntimeData>& RuntimeData = EventRuntimeData(_this);
	int32 Instances = 0;

	for (int32 i = 0; i < RuntimeData.Num(); i++)
	{
		if (RuntimeData[i].Event != Event)
			continue;

		if (RuntimeData[i].SpawnedActors.Num() == 0 && RuntimeData[i].NextSpawnTime <= TimeSeconds)
			return true;

		Instances++;
	}

	return Event->MaxCount <= 0 || Instances < Event->MaxCount;
}

bool FortAthenaLivingWorldManager::IsCategoryAvailable(UFortAthenaLivingWorldManager* _this, FFortAthenaLivingWorldCategory* Category)
{
	if (!UBlueprintGameplayTagLibrary::HasAllTags(CachedPlaylistContextTags(_this), Category->RequirePlaylistTags, false))
		return false;

	TArray<FLivingWorldEventRuntimeData>& RuntimeData = EventRuntimeData(_this);
	int32 ActiveEvents = 0;

	for (int32 i = 0; i < RuntimeData.Num(); i++)
	{
		if (RuntimeData[i].Category == Category && RuntimeData[i].SpawnedActors.Num() > 0)
			ActiveEvents++;
	}

	if (Category->MaxCount > 0 && ActiveEvents >= Category->MaxCount)
		return false;

	bool bHasAvailableEvent = false;

	ForEachRow<FFortAthenaLivingWorldEvent>(Category->EventTable.LoadSynchronous(), [&](FName Key, FFortAthenaLivingWorldEvent* Event)
		{
			if (!bHasAvailableEvent && Event->Weight > 0.f)
				bHasAvailableEvent = IsEventAvailable(_this, Event);
		});

	return bHasAvailableEvent;
}

AActor* FortAthenaLivingWorldManager::SpawnActorDescription(UFortAthenaLivingWorldManager* _this, FFortAthenaLivingWorldEventDataActorSpawnDescription& Description, const FVector& Location, const FRotator& Rotation)
{
	UClass* SpawnerData = Description.SpawnerData.LoadSynchronous();

	if (SpawnerData == NULL && Description.SpawnerDataTagQuery.QueryTokenStream.Num() > 0)
	{
		TArray<UClass*> Matches;

		for (int32 i = 0; i < _this->LoadedSpawnerDataClass.Num(); i++)
		{
			UClass* SpawnerDataClass = _this->LoadedSpawnerDataClass[i];
			UFortAthenaAISpawnerData* SpawnerDataDefault = SpawnerDataClass ? Cast<UFortAthenaAISpawnerData>(SpawnerDataClass->ClassDefaultObject) : NULL;

			if (SpawnerDataDefault && UBlueprintGameplayTagLibrary::DoesContainerMatchTagQuery(SpawnerDataDefault->DescriptorTag, Description.SpawnerDataTagQuery))
				Matches.Add(SpawnerDataClass);
		}

		if (Matches.Num() > 0)
			SpawnerData = Matches[min((int32)(FRand(nullptr) * Matches.Num()), Matches.Num() - 1)];
	}

	if (SpawnerData && SpawnerData->IsSubclassOf(UFortAthenaAISpawnerData::StaticClass()))
	{
		AFortGameModeAthena* GameMode = Cast<AFortGameModeAthena>(GWorld->AuthorityGameMode);
		UFortAthenaAISpawnerDataComponentList* ComponentList = UFortAthenaAISpawnerData::CreateComponentListFromClass(SpawnerData, _this);

		return GameMode && GameMode->ServerBotManager && ComponentList ? GameMode->ServerBotManager->SpawnAI(Location, Rotation, ComponentList) : NULL;
	}

	UClass* ActorClass = Description.ActorClass.LoadSynchronous();

	if (ActorClass == NULL)
		return NULL;

	FActorSpawnParameters SpawnParameters{};
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	return GWorld->SpawnActor(ActorClass, Location, Rotation, SpawnParameters);
}

int32 FortAthenaLivingWorldManager::SpawnEvent(UFortAthenaLivingWorldManager* _this, FFortAthenaLivingWorldCategory* Category, FFortAthenaLivingWorldEvent* Event)
{
	UFortAthenaLivingWorldEventData* EventData = Event->EventData.LoadSynchronous();

	FVector Location;
	FRotator Rotation;

	if (EventData == NULL || !PickPointProvider(_this, EventData->ProviderFiltersTagQuery, Location, Rotation))
		return 0;

	const float TimeSeconds = UGameplayStatics::GetTimeSeconds(GWorld);
	TArray<FLivingWorldEventRuntimeData>& RuntimeData = EventRuntimeData(_this);
	int32 EntryIndex = -1;

	for (int32 i = 0; i < RuntimeData.Num() && EntryIndex == -1; i++)
	{
		if (RuntimeData[i].Event == Event && RuntimeData[i].SpawnedActors.Num() == 0 && RuntimeData[i].NextSpawnTime <= TimeSeconds)
			EntryIndex = i;
	}

	if (EntryIndex == -1)
	{
		FLivingWorldEventRuntimeData NewEntry{};
		NewEntry.Category = Category;
		NewEntry.Event = Event;
		NewEntry.EventData = EventData;

		EntryIndex = RuntimeData.Num();
		RuntimeData.Add(NewEntry);
	}

	FScriptDelegate DestroyDelegate;
	DestroyDelegate.BindUFunction(_this, FName(L"OnSpawnedActorDestroy"));

	for (int32 i = 0; i < EventData->ActorDescriptions.Num(); i++)
	{
		FFortAthenaLivingWorldEventDataActorSpawnDescription& Description = EventData->ActorDescriptions[i];

		FVector SpawnLocation = Location;
		FRotator SpawnRotation = Rotation;

		if (Description.bUseProviderTagOverrides && !PickPointProvider(_this, Description.ProviderFiltersTagQueryOverride, SpawnLocation, SpawnRotation))
			continue;

		AActor* SpawnedActor = SpawnActorDescription(_this, Description, SpawnLocation, SpawnRotation);

		if (SpawnedActor == NULL)
			continue;

		TWeakObjectPtr<AActor> WeakActor;
		WeakActor = SpawnedActor;

		RuntimeData[EntryIndex].SpawnedActors.Add(WeakActor);
		SpawnedActor->OnDestroyed.AddUnique(SpawnedActor, DestroyDelegate);
	}

	return RuntimeData[EntryIndex].SpawnedActors.Num();
}

void FortAthenaLivingWorldManager::GenerateEvents(UFortAthenaLivingWorldManager* _this)
{
	UFortAthenaLivingWorldConfigData* Config = _this->LagerConfig.LoadSynchronous();
	UDataTable* CategoryTable = Config ? Config->CategoryTable.LoadSynchronous() : NULL;

	if (CategoryTable == NULL)
		return;

	TArray<FLivingWorldEventRuntimeData>& RuntimeData = EventRuntimeData(_this);
	int32 Budget = (int32)Config->MaxActorCount.Evaluate(0.f);

	for (int32 i = 0; i < RuntimeData.Num(); i++)
		Budget -= RuntimeData[i].SpawnedActors.Num();

	const int32 MaxEventSpawnPerTick = max((int32)Config->MaxEventSpawnPerTick.Evaluate(0.f), 1);

	for (int32 i = 0; i < MaxEventSpawnPerTick && Budget > 0; i++)
	{
		FFortAthenaLivingWorldCategory* Category = PickWeightedRow<FFortAthenaLivingWorldCategory>(CategoryTable, [&](FFortAthenaLivingWorldCategory* Candidate) { return IsCategoryAvailable(_this, Candidate); });
		FFortAthenaLivingWorldEvent* Event = Category ? PickWeightedRow<FFortAthenaLivingWorldEvent>(Category->EventTable.LoadSynchronous(), [&](FFortAthenaLivingWorldEvent* Candidate) { return IsEventAvailable(_this, Candidate); }) : NULL;

		if (Event == NULL)
			break;

		Budget -= SpawnEvent(_this, Category, Event);
	}
}

void FortAthenaLivingWorldManager::Update(UFortAthenaLivingWorldManager* _this)
{
	if (NextGenerationTime(_this) < 0.f || !bGenerateEvents(_this) || PendingLoads(_this) > 0)
		return;

	const float TimeSeconds = UGameplayStatics::GetTimeSeconds(GWorld);

	if (TimeSeconds < NextGenerationTime(_this))
		return;

	UFortAthenaLivingWorldConfigData* Config = _this->LagerConfig.LoadSynchronous();
	NextGenerationTime(_this) = TimeSeconds + (Config ? Config->MinimumEventGenerationInterval.Evaluate(0.f) : 0.f);

	GenerateEvents(_this);
}

void FortAthenaLivingWorldManager::Flush(UFortAthenaLivingWorldManager* _this)
{
	TArray<FLivingWorldEventRuntimeData>& RuntimeData = EventRuntimeData(_this);
	TArray<AActor*> SpawnedActors;

	for (int32 i = 0; i < RuntimeData.Num(); i++)
	{
		for (int32 j = 0; j < RuntimeData[i].SpawnedActors.Num(); j++)
		{
			if (AActor* SpawnedActor = RuntimeData[i].SpawnedActors[j].Get())
				SpawnedActors.Add(SpawnedActor);
		}
	}

	for (int32 i = 0; i < SpawnedActors.Num(); i++)
		SpawnedActors[i]->K2_DestroyActor();
}

void FortAthenaLivingWorldManager::BeginPlay(UFortAthenaLivingWorldManager* _this)
{
	BeginPlayOG(_this);

	AActor* Owner = _this->GetOwner();

	if (Owner == NULL || Owner->Role != ENetRole::ROLE_Authority)
		return;

	_this->CachedGameState = Cast<AFortGameStateAthena>(GWorld->GameState);
	BindToPlaylistLoaded(_this);

	TWeakObjectPtr<UFortAthenaLivingWorldManager> WeakManager;
	WeakManager = _this;

	FTimerHandle UpdateTimerHandle;
	GWorld->GetTimerManager().SetTimer(UpdateTimerHandle, [WeakManager]()
		{
			if (UFortAthenaLivingWorldManager* Manager = WeakManager.Get())
				Update(Manager);
		}, 0.25f, true);
}

void FortAthenaLivingWorldManager::VolumeBeginPlay(AFortAthenaLivingWorldVolume* _this)
{
	VolumeBeginPlayOG(_this);

	if (_this->Role == ENetRole::ROLE_Authority)
		BindToPlaylistLoaded(_this);
}

void FortAthenaLivingWorldManager::OnEnvQueryFinished(AFortAthenaLivingWorldVolume* _this, void* Result)
{
	OnEnvQueryFinishedOG(_this, Result);

	if (Field<TArray<FVector>>(_this, 0x2C8).Num() > 0)
		RegisterPointProvider(_this);
}

void FortAthenaLivingWorldManager::execLivingWorldManagerRegisterPointProvider(UObject* Context, FFrame& Stack, void* const Result)
{
	AActor* PointProvider = Stack.StepCompiledInRef<AActor*>();
	Stack.IncrementCode();

	RegisterPointProvider(PointProvider);
}

void FortAthenaLivingWorldManager::execLivingWorldManagerUnregisterPointProvider(UObject* Context, FFrame& Stack, void* const Result)
{
	AActor* PointProvider = Stack.StepCompiledInRef<AActor*>();
	Stack.IncrementCode();

	UnregisterPointProvider(PointProvider);
}

void FortAthenaLivingWorldManager::execLivingWorldManagerFlush(UObject* Context, FFrame& Stack, void* const Result)
{
	Stack.IncrementCode();

	Flush(static_cast<UFortAthenaLivingWorldManager*>(Context));
}

void FortAthenaLivingWorldManager::execLivingWorldManagerGenerateEvents(UObject* Context, FFrame& Stack, void* const Result)
{
	Stack.IncrementCode();

	GenerateEvents(static_cast<UFortAthenaLivingWorldManager*>(Context));
}

void FortAthenaLivingWorldManager::execLivingWorldManagerToggleGenerateEvents(UObject* Context, FFrame& Stack, void* const Result)
{
	Stack.IncrementCode();

	UFortAthenaLivingWorldManager* Manager = static_cast<UFortAthenaLivingWorldManager*>(Context);
	bGenerateEvents(Manager) = !bGenerateEvents(Manager);
}

void FortAthenaLivingWorldManager::execLivingWorldManagerToggleVerboseLogging(UObject* Context, FFrame& Stack, void* const Result)
{
	Stack.IncrementCode();

	UFortAthenaLivingWorldManager* Manager = static_cast<UFortAthenaLivingWorldManager*>(Context);
	bVerboseLogging(Manager) = !bVerboseLogging(Manager);
}

void FortAthenaLivingWorldManager::execLivingWorldManagerSpawn(UObject* Context, FFrame& Stack, void* const Result)
{
	FString EventDataName;
	Stack.StepCompiledIn(&EventDataName);
	Stack.IncrementCode();

	UFortAthenaLivingWorldManager* Manager = GetLivingWorldManager();
	UFortAthenaLivingWorldConfigData* Config = Manager ? Manager->LagerConfig.LoadSynchronous() : NULL;

	if (Config == NULL)
		return;

	const std::string Name = EventDataName.ToString();
	FFortAthenaLivingWorldCategory* FoundCategory = NULL;
	FFortAthenaLivingWorldEvent* FoundEvent = NULL;

	ForEachRow<FFortAthenaLivingWorldCategory>(Config->CategoryTable.LoadSynchronous(), [&](FName CategoryKey, FFortAthenaLivingWorldCategory* Category)
		{
			ForEachRow<FFortAthenaLivingWorldEvent>(Category->EventTable.LoadSynchronous(), [&](FName EventKey, FFortAthenaLivingWorldEvent* Event)
				{
					UFortAthenaLivingWorldEventData* EventData = FoundEvent ? NULL : Event->EventData.LoadSynchronous();

					if (EventData && EventData->GetName() == Name)
					{
						FoundCategory = Category;
						FoundEvent = Event;
					}
				});
		});

	if (FoundEvent)
		SpawnEvent(Manager, FoundCategory, FoundEvent);
}

void FortAthenaLivingWorldManager::Init()
{
	UClass* ManagerClass = UFortAthenaLivingWorldManager::StaticClass();

	Hooking::Hook<UFortAthenaLivingWorldManager>(0x320 / 8, BeginPlay, BeginPlayOG);
	Hooking::Hook<AFortAthenaLivingWorldVolume>(0x328 / 8, VolumeBeginPlay, VolumeBeginPlayOG);
	Hooking::Hook(ImageBase + 0x1221D50, OnEnvQueryFinished, OnEnvQueryFinishedOG);

	Hooking::ExecHook(ManagerClass->GetFunction("LivingWorldManagerRegisterPointProvider"), execLivingWorldManagerRegisterPointProvider);
	Hooking::ExecHook(ManagerClass->GetFunction("LivingWorldManagerUnregisterPointProvider"), execLivingWorldManagerUnregisterPointProvider);
	Hooking::ExecHook(ManagerClass->GetFunction("LivingWorldManagerFlush"), execLivingWorldManagerFlush);
	Hooking::ExecHook(ManagerClass->GetFunction("LivingWorldManagerGenerateEvents"), execLivingWorldManagerGenerateEvents);
	Hooking::ExecHook(ManagerClass->GetFunction("LivingWorldManagerToggleGenerateEvents"), execLivingWorldManagerToggleGenerateEvents);
	Hooking::ExecHook(ManagerClass->GetFunction("LivingWorldManagerToggleVerboseLogging"), execLivingWorldManagerToggleVerboseLogging);
	Hooking::ExecHook(UFortCheatManager_LivingWorldManager::StaticClass()->GetFunction("LivingWorldManagerSpawn"), execLivingWorldManagerSpawn);
}
