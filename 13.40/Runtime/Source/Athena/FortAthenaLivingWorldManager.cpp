#include "pch.h"
#include "Core.h"
#include "Athena/FortAthenaLivingWorldManager.h"
#include "Engine/TimerManager.h"
DEFINE_ENGINE_INIT(FortAthenaLivingWorldManager)

template <typename T>
void FortAthenaLivingWorldManager::BindToPlaylistLoaded(T* Object)
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

bool FortAthenaLivingWorldManager::MatchesTagQuery(const FGameplayTagContainer& Tags, const FGameplayTagQuery& Query)
{
	return Query.QueryTokenStream.Num() == 0 || UBlueprintGameplayTagLibrary::DoesContainerMatchTagQuery(Tags, Query);
}

bool FortAthenaLivingWorldManager::MatchesPlaylistTags(UFortAthenaLivingWorldManager* _this, const FGameplayTagContainer& RequirePlaylistTags)
{
	return RequirePlaylistTags.GameplayTags.Num() == 0 || UBlueprintGameplayTagLibrary::HasAnyTags(Get<FGameplayTagContainer>(_this, 0x2C0), RequirePlaylistTags, false);
}

bool FortAthenaLivingWorldManager::GetProviderLocation(AActor* PointProvider, FVector& OutLocation, FRotator& OutRotation)
{
	IFortAthenaLivingWorldPointProviderInterface* Interface = GetInterfaceAddress<IFortAthenaLivingWorldPointProviderInterface>(PointProvider);
	return Interface && (*reinterpret_cast<bool(***)(IFortAthenaLivingWorldPointProviderInterface*, FVector*, FRotator*)>(Interface))[2](Interface, &OutLocation, &OutRotation);
}

AActor* FortAthenaLivingWorldManager::PickPointProvider(UFortAthenaLivingWorldManager* _this, const FGameplayTagQuery& FiltersTagQuery)
{
	TArray<AActor*> Candidates;

	for (int32 i = 0; i < _this->PointProviders.Num(); i++)
	{
		AFortAthenaLivingWorldVolume* Volume = Cast<AFortAthenaLivingWorldVolume>(_this->PointProviders[i]);

		if (Volume && MatchesTagQuery(Volume->FiltersTags, FiltersTagQuery))
			Candidates.Add(Volume);
	}

	return Candidates.Num() > 0 ? Candidates[min((int32)(FRand(nullptr) * Candidates.Num()), Candidates.Num() - 1)] : NULL;
}

bool FortAthenaLivingWorldManager::HasPointProvider(UFortAthenaLivingWorldManager* _this, UFortAthenaLivingWorldEventData* EventData)
{
	if (!PointProviderCache.contains(EventData))
		PointProviderCache[EventData] = PickPointProvider(_this, EventData->ProviderFiltersTagQuery) != NULL;

	return PointProviderCache[EventData];
}

int32 FortAthenaLivingWorldManager::GetSpawnedCount(UFortAthenaLivingWorldManager* _this, FFortAthenaLivingWorldCategory* Category, FFortAthenaLivingWorldEvent* Event)
{
	const float TimeSeconds = UGameplayStatics::GetTimeSeconds(GWorld);
	TArray<FLivingWorldEventRuntimeData>& RuntimeData = Get<TArray<FLivingWorldEventRuntimeData>>(_this, 0x268);
	TArray<FLivingWorldEventRequest>& PendingEventRequests = Get<TArray<FLivingWorldEventRequest>>(_this, 0x258);
	int32 Count = 0;

	for (int32 i = 0; i < RuntimeData.Num(); i++)
	{
		if ((Category == NULL || RuntimeData[i].Category == Category) && (Event == NULL || RuntimeData[i].Event == Event))
			Count += RuntimeData[i].SpawnedActors.Num() > 0 ? RuntimeData[i].SpawnedActors.Num() : RuntimeData[i].NextSpawnTime > TimeSeconds;
	}

	for (int32 i = 0; i < PendingEventRequests.Num(); i++)
	{
		if ((Category == NULL || PendingEventRequests[i].Category == Category) && (Event == NULL || PendingEventRequests[i].Event == Event))
			Count++;
	}

	return Count;
}

bool FortAthenaLivingWorldManager::IsEventAvailable(UFortAthenaLivingWorldManager* _this, FFortAthenaLivingWorldEvent* Event)
{
	UFortAthenaLivingWorldEventData* EventData = Event->EventData.LoadSynchronous();

	if (EventData == NULL || !MatchesPlaylistTags(_this, Event->RequirePlaylistTags) || GetSpawnedCount(_this, NULL, Event) >= Event->MaxCount)
		return false;

	AFortTimeOfDayManager* TimeOfDayManager = AFortTimeOfDayManager::GetTimeOfDayManagerFromContext(GWorld);

	if (TimeOfDayManager == NULL || (EventData->TimeOfDayFilters & (1 << (int32)TimeOfDayManager->CurrentDayNightPhase)) == 0)
		return false;

	if (void* WaterSubsystem = reinterpret_cast<void*(*)(UWorld*)>(ImageBase + 0xFA78C0)(GWorld))
	{
		const float WaterLevelIndex = Get<float>(WaterSubsystem, 0x7C) * (1.f / 384.f);

		if (WaterLevelIndex != -1.f && ((EventData->WaterLevelIndexMin != -1.f && WaterLevelIndex < EventData->WaterLevelIndexMin) || (EventData->WaterLevelIndexMax != -1.f && WaterLevelIndex > EventData->WaterLevelIndexMax)))
			return false;
	}

	return HasPointProvider(_this, EventData);
}

bool FortAthenaLivingWorldManager::IsCategoryAvailable(UFortAthenaLivingWorldManager* _this, FFortAthenaLivingWorldCategory* Category)
{
	if (!MatchesPlaylistTags(_this, Category->RequirePlaylistTags) || GetSpawnedCount(_this, Category, NULL) >= Category->MaxCount)
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
	if (UClass* ActorClass = Description.ActorClass.LoadSynchronous())
	{
		FActorSpawnParameters SpawnParameters{};
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		return GWorld->SpawnActor(ActorClass, Location, Rotation, SpawnParameters);
	}

	TArray<UClass*> SpawnerDataClasses;
	UClass* SpawnerData = Description.SpawnerData.LoadSynchronous();

	if (SpawnerData && SpawnerData->IsSubclassOf(UFortAthenaAISpawnerData::StaticClass()))
		SpawnerDataClasses.Add(SpawnerData);

	for (int32 i = 0; i < _this->LoadedSpawnerDataClass.Num() && Description.SpawnerDataTagQuery.QueryTokenStream.Num() > 0; i++)
	{
		UClass* SpawnerDataClass = _this->LoadedSpawnerDataClass[i];
		UFortAthenaAISpawnerData* SpawnerDataDefault = SpawnerDataClass ? Cast<UFortAthenaAISpawnerData>(SpawnerDataClass->ClassDefaultObject) : NULL;

		if (SpawnerDataDefault && !SpawnerDataClasses.Contains(SpawnerDataClass) && UBlueprintGameplayTagLibrary::DoesContainerMatchTagQuery(SpawnerDataDefault->DescriptorTag, Description.SpawnerDataTagQuery))
			SpawnerDataClasses.Add(SpawnerDataClass);
	}

	if (SpawnerDataClasses.Num() == 0)
		return NULL;

	AFortGameModeAthena* GameMode = Cast<AFortGameModeAthena>(GWorld->AuthorityGameMode);
	UFortAthenaAISpawnerDataComponentList* ComponentList = UFortAthenaAISpawnerData::CreateComponentListFromClass(SpawnerDataClasses[min((int32)(FRand(nullptr) * SpawnerDataClasses.Num()), SpawnerDataClasses.Num() - 1)], _this);

	return GameMode && GameMode->ServerBotManager && ComponentList ? GameMode->ServerBotManager->SpawnAI(Location, Rotation, ComponentList) : NULL;
}

void FortAthenaLivingWorldManager::SpawnEvent(UFortAthenaLivingWorldManager* _this, FFortAthenaLivingWorldCategory* Category, FFortAthenaLivingWorldEvent* Event)
{
	UFortAthenaLivingWorldEventData* EventData = Event->EventData.LoadSynchronous();
	AActor* PointProvider = EventData ? PickPointProvider(_this, EventData->ProviderFiltersTagQuery) : NULL;

	if (PointProvider == NULL)
		return;

	const float TimeSeconds = UGameplayStatics::GetTimeSeconds(GWorld);
	TArray<FLivingWorldEventRuntimeData>& RuntimeData = Get<TArray<FLivingWorldEventRuntimeData>>(_this, 0x268);
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
		AActor* DescriptionPointProvider = Description.bUseProviderTagOverrides ? PickPointProvider(_this, Description.ProviderFiltersTagQueryOverride) : PointProvider;

		FVector Location;
		FRotator Rotation;

		if (DescriptionPointProvider == NULL || !GetProviderLocation(DescriptionPointProvider, Location, Rotation))
			continue;

		TWeakObjectPtr<AActor> WeakActor;
		WeakActor = SpawnActorDescription(_this, Description, Location, Rotation);

		if (WeakActor.Get() == NULL)
			continue;

		RuntimeData[EntryIndex].SpawnedActors.Add(WeakActor);
		WeakActor.Get()->OnDestroyed.AddUnique(WeakActor.Get(), DestroyDelegate);

		if (Get<bool>(_this, 0x2E8))
			AddActorOnMinimap(_this, WeakActor.Get());
	}
}

void FortAthenaLivingWorldManager::AddActorOnMinimap(UFortAthenaLivingWorldManager* _this, AActor* Actor)
{
	if (_this->CachedGameState == NULL || _this->CachedGameState->SpecialActorData == NULL)
		return;

	FSlateBrush* MinimapIconBrush = &_this->SpecialActorMinimapIconBrush;
	FVector2D MinimapIconScale = _this->SpecialActorMinimapIconScale;
	FSlateBrush* CompassIconBrush = &_this->SpecialActorCompassIconBrush;
	FVector2D CompassIconScale = _this->SpecialActorCompassIconScale;

	AFortPawn* Pawn = Cast<AFortPawn>(Actor);
	AFortAthenaAIBotController* Controller = Pawn ? Cast<AFortAthenaAIBotController>(Pawn->Controller) : NULL;

	if (Controller && Controller->DebugMinimapData.bIsOverridden)
	{
		MinimapIconBrush = &Controller->DebugMinimapData.DebugMinimapIconBrush;
		MinimapIconScale = Controller->DebugMinimapData.DebugMinimapIconScale;
		CompassIconBrush = &Controller->DebugMinimapData.DebugCompassIconBrush;
		CompassIconScale = Controller->DebugMinimapData.DebugCompassIconScale;
	}

	const std::string ActorName = Actor->GetName();
	alignas(16) uint8 InitData[0x1A0];

	reinterpret_cast<void*(*)(void*)>(ImageBase + 0x2537370)(InitData);
	Get<AActor*>(InitData, 0x0) = Actor;
	Get<FName>(InitData, 0x8) = FName((L"LW__" + std::wstring(ActorName.begin(), ActorName.end())).c_str());
	Get<float>(InitData, 0x20) = 3.f;
	Get<float>(InitData, 0x24) = 1.5f;
	reinterpret_cast<void*(*)(void*, const FSlateBrush*)>(ImageBase + 0xA2DBB0)(InitData + 0x28, MinimapIconBrush);
	Get<FVector2D>(InitData, 0xB0) = MinimapIconScale;
	reinterpret_cast<void*(*)(void*, const FSlateBrush*)>(ImageBase + 0xA2DBB0)(InitData + 0xB8, CompassIconBrush);
	Get<FVector2D>(InitData, 0x140) = CompassIconScale;

	reinterpret_cast<int32(*)(AFortSpecialActorReplicationInfo*, void*)>(ImageBase + 0x253ABA0)(_this->CachedGameState->SpecialActorData, InitData);
	reinterpret_cast<void(*)(void*)>(ImageBase + 0x1E35940)(InitData);
}

void FortAthenaLivingWorldManager::GenerateEvents(UFortAthenaLivingWorldManager* _this)
{
	UFortAthenaLivingWorldConfigData* Config = _this->LagerConfig.LoadSynchronous();
	UDataTable* CategoryTable = Config ? Config->CategoryTable.LoadSynchronous() : NULL;

	if (CategoryTable == NULL)
		return;

	const int32 Budget = (int32)Config->MaxActorCount.Evaluate(0.f) - GetSpawnedCount(_this, NULL, NULL);
	PointProviderCache.clear();

	for (int32 i = 0; i < Budget; i++)
	{
		FFortAthenaLivingWorldCategory* Category = PickWeightedRow<FFortAthenaLivingWorldCategory>(CategoryTable, [&](FFortAthenaLivingWorldCategory* Candidate) { return IsCategoryAvailable(_this, Candidate); });
		FFortAthenaLivingWorldEvent* Event = Category ? PickWeightedRow<FFortAthenaLivingWorldEvent>(Category->EventTable.LoadSynchronous(), [&](FFortAthenaLivingWorldEvent* Candidate) { return IsEventAvailable(_this, Candidate); }) : NULL;

		if (Event == NULL)
			break;

		Get<TArray<FLivingWorldEventRequest>>(_this, 0x258).Add({ Category, Event });
	}
}

void FortAthenaLivingWorldManager::Update(UFortAthenaLivingWorldManager* _this)
{
	if (Get<float>(_this, 0x2B8) < 0.f || Get<int32>(_this, 0x250) > 0)
		return;

	UFortAthenaLivingWorldConfigData* Config = _this->LagerConfig.LoadSynchronous();
	const float TimeSeconds = UGameplayStatics::GetTimeSeconds(GWorld);

	if (Config == NULL)
		return;

	if (Get<bool>(_this, 0x2E1) && TimeSeconds >= Get<float>(_this, 0x2B8))
	{
		Get<float>(_this, 0x2B8) = TimeSeconds + Config->MinimumEventGenerationInterval.Evaluate(0.f);
		GenerateEvents(_this);
	}

	TArray<FLivingWorldEventRequest>& PendingEventRequests = Get<TArray<FLivingWorldEventRequest>>(_this, 0x258);
	const int32 MaxEventSpawnPerTick = (int32)Config->MaxEventSpawnPerTick.Evaluate(0.f);

	for (int32 i = 0; i < MaxEventSpawnPerTick && PendingEventRequests.Num() > 0; i++)
	{
		FLivingWorldEventRequest Request = PendingEventRequests[0];
		PendingEventRequests.Remove(0);

		SpawnEvent(_this, Request.Category, Request.Event);
	}
}

void FortAthenaLivingWorldManager::Flush(UFortAthenaLivingWorldManager* _this)
{
	TArray<FLivingWorldEventRuntimeData>& RuntimeData = Get<TArray<FLivingWorldEventRuntimeData>>(_this, 0x268);
	TArray<AActor*> SpawnedActors;

	Get<TArray<FLivingWorldEventRequest>>(_this, 0x258).Free();

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
	static FTimerHandle UpdateTimerHandle;

	BeginPlayOG(_this);

	AActor* Owner = _this->GetOwner();

	if (Owner == NULL || Owner->Role != ENetRole::ROLE_Authority)
		return;

	_this->CachedGameState = Cast<AFortGameStateAthena>(GWorld->GameState);
	BindToPlaylistLoaded(_this);

	TWeakObjectPtr<UFortAthenaLivingWorldManager> WeakManager;
	WeakManager = _this;

	GWorld->GetTimerManager().ClearTimer(UpdateTimerHandle);
	GWorld->GetTimerManager().SetTimer(UpdateTimerHandle, [WeakManager]()
		{
			if (UFortAthenaLivingWorldManager* Manager = WeakManager.Get())
				Update(Manager);
			else
				GWorld->GetTimerManager().ClearTimer(UpdateTimerHandle);
		}, 0.25f, true);
}

void FortAthenaLivingWorldManager::VolumePostInitializeComponents(AFortAthenaLivingWorldVolume* _this)
{
	static UStaticMesh* Cube = StaticLoadObject<UStaticMesh>(L"/Engine/BasicShapes/Cube.Cube");

	if (_this->BrushComponent || !_this->Brush || !Cube || !Cube->BodySetup)
		return VolumePostInitializeComponentsOG(_this);

	TArray<AFortPoiVolume*> PoiVolumes = GetAll<AFortPoiVolume>();
	FVector Min{}, Max{};
	bool bFound = false;

	for (int32 i = 0; i < PoiVolumes.Num(); i++)
	{
		if (!PoiVolumes[i] || !PoiVolumes[i]->BrushComponent || !UBlueprintGameplayTagLibrary::HasAnyTags(PoiVolumes[i]->LocationTags, _this->FiltersTags, true))
			continue;

		FVector Origin, Extent;
		float SphereRadius;
		UKismetSystemLibrary::GetComponentBounds(PoiVolumes[i]->BrushComponent, &Origin, &Extent, &SphereRadius);

		Min = bFound ? UKismetMathLibrary::Vector_ComponentMin(Min, Origin - Extent) : Origin - Extent;
		Max = bFound ? UKismetMathLibrary::Vector_ComponentMax(Max, Origin + Extent) : Origin + Extent;
		bFound = true;
	}

	PoiVolumes.Free();

	if (!bFound)
		return VolumePostInitializeComponentsOG(_this);

	FTransform Identity = UKismetMathLibrary::MakeTransform(FVector{}, FRotator{}, FVector{ 1.f, 1.f, 1.f });
	UBrushComponent* BrushComponent = Cast<UBrushComponent>(_this->AddComponentByClass(UBrushComponent::StaticClass(), false, Identity, true));

	if (!BrushComponent)
		return VolumePostInitializeComponentsOG(_this);

	BrushComponent->Brush = _this->Brush;
	BrushComponent->BrushBodySetup = Cube->BodySetup;
	BrushComponent->SetMobility(EComponentMobility::Movable);
	BrushComponent->SetCollisionProfileName(FName(L"NoCollision"), false);

	_this->FinishAddComponent(BrushComponent, false, Identity);
	_this->BrushComponent = BrushComponent;

	const float HalfExtent = max(Max.X - Min.X, max(Max.Y - Min.Y, Max.Z - Min.Z)) * 0.5f;
	BrushComponent->SetWorldScale3D(FVector{ HalfExtent / 50.f, HalfExtent / 50.f, HalfExtent / 50.f });
	_this->K2_SetActorLocation((Min + Max) * 0.5f, false, nullptr, true);

	VolumePostInitializeComponentsOG(_this);
}

void FortAthenaLivingWorldManager::VolumeBeginPlay(AFortAthenaLivingWorldVolume* _this)
{
	VolumeBeginPlayOG(_this);

	if (_this->Role == ENetRole::ROLE_Authority)
		BindToPlaylistLoaded(_this);
}

void FortAthenaLivingWorldManager::VolumeEndPlay(AFortAthenaLivingWorldVolume* _this, EEndPlayReason EndPlayReason)
{
	if (_this->Role == ENetRole::ROLE_Authority)
		UnregisterPointProvider(_this);

	VolumeEndPlayOG(_this, EndPlayReason);
}

void FortAthenaLivingWorldManager::VolumeOnCurrentPlaylistLoaded(AFortAthenaLivingWorldVolume* _this, FName PlaylistName, const FGameplayTagContainer& PlaylistContextTags)
{
	if (UNavigationSystemV1* NavSystem = Cast<UNavigationSystemV1>(GWorld->NavigationSystem))
	{
		for (int32 i = 0; i < NavSystem->NavDataSet.Num(); i++)
		{
			if (NavSystem->NavDataSet[i])
				NavSystem->NavDataSet[i]->EnsureBuildCompletion();
		}
	}

	VolumeOnCurrentPlaylistLoadedOG(_this, PlaylistName, PlaylistContextTags);
}

void FortAthenaLivingWorldManager::OnEnvQueryFinished(AFortAthenaLivingWorldVolume* _this, void* Result)
{
	OnEnvQueryFinishedOG(_this, Result);

	if (Get<TArray<FVector>>(_this, 0x2C8).Num() > 0)
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

	UFortAthenaLivingWorldManager* Manager = static_cast<UFortAthenaLivingWorldManager*>(Context);

	if (Get<float>(Manager, 0x2B8) >= 0.f)
		GenerateEvents(Manager);
}

void FortAthenaLivingWorldManager::execLivingWorldManagerProviderDebugActor(UObject* Context, FFrame& Stack, void* const Result)
{
	int32 ProviderIndex = Stack.StepCompiledInRef<int32>();
	Stack.IncrementCode();

	Get<int32>(Context, 0x2E4) = ProviderIndex;
}

void FortAthenaLivingWorldManager::execLivingWorldManagerToggleActorMinimap(UObject* Context, FFrame& Stack, void* const Result)
{
	Stack.IncrementCode();

	Get<bool>(Context, 0x2E8) = !Get<bool>(Context, 0x2E8);
}

void FortAthenaLivingWorldManager::execLivingWorldManagerToggleGenerateEvents(UObject* Context, FFrame& Stack, void* const Result)
{
	Stack.IncrementCode();

	Get<bool>(Context, 0x2E1) = !Get<bool>(Context, 0x2E1);
}

void FortAthenaLivingWorldManager::execLivingWorldManagerToggleVerboseLogging(UObject* Context, FFrame& Stack, void* const Result)
{
	Stack.IncrementCode();

	Get<bool>(Context, 0x2E0) = !Get<bool>(Context, 0x2E0);
}

void FortAthenaLivingWorldManager::execLivingWorldManagerSpawn(UObject* Context, FFrame& Stack, void* const Result)
{
	FString EventDataName;
	Stack.StepCompiledIn(&EventDataName);
	Stack.IncrementCode();

	UFortAthenaLivingWorldManager* Manager = GetLivingWorldManager();
	UFortAthenaLivingWorldConfigData* Config = Manager && Get<float>(Manager, 0x2B8) >= 0.f ? Manager->LagerConfig.LoadSynchronous() : NULL;

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
	Hooking::Hook<AFortAthenaLivingWorldVolume>(0x480 / 8, VolumePostInitializeComponents, VolumePostInitializeComponentsOG);
	Hooking::Hook<AFortAthenaLivingWorldVolume>(0x328 / 8, VolumeBeginPlay, VolumeBeginPlayOG);
	Hooking::Hook<AFortAthenaLivingWorldVolume>(0x330 / 8, VolumeEndPlay, VolumeEndPlayOG);
	Hooking::Hook(ImageBase + 0x12216A0, VolumeOnCurrentPlaylistLoaded, VolumeOnCurrentPlaylistLoadedOG);
	Hooking::Hook(ImageBase + 0x1221D50, OnEnvQueryFinished, OnEnvQueryFinishedOG);

	Hooking::ExecHook(ManagerClass->GetFunction("LivingWorldManagerRegisterPointProvider"), execLivingWorldManagerRegisterPointProvider);
	Hooking::ExecHook(ManagerClass->GetFunction("LivingWorldManagerUnregisterPointProvider"), execLivingWorldManagerUnregisterPointProvider);
	Hooking::ExecHook(ManagerClass->GetFunction("LivingWorldManagerFlush"), execLivingWorldManagerFlush);
	Hooking::ExecHook(ManagerClass->GetFunction("LivingWorldManagerGenerateEvents"), execLivingWorldManagerGenerateEvents);
	Hooking::ExecHook(ManagerClass->GetFunction("LivingWorldManagerProviderDebugActor"), execLivingWorldManagerProviderDebugActor);
	Hooking::ExecHook(ManagerClass->GetFunction("LivingWorldManagerToggleActorMinimap"), execLivingWorldManagerToggleActorMinimap);
	Hooking::ExecHook(ManagerClass->GetFunction("LivingWorldManagerToggleGenerateEvents"), execLivingWorldManagerToggleGenerateEvents);
	Hooking::ExecHook(ManagerClass->GetFunction("LivingWorldManagerToggleVerboseLogging"), execLivingWorldManagerToggleVerboseLogging);
	Hooking::ExecHook(UFortCheatManager_LivingWorldManager::StaticClass()->GetFunction("LivingWorldManagerSpawn"), execLivingWorldManagerSpawn);
}
