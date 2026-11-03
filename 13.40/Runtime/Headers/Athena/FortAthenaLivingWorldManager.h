#pragma once
#include "pch.h"

namespace FortAthenaLivingWorldManager
{
	struct FLivingWorldEventRuntimeData
	{
		FFortAthenaLivingWorldCategory* Category;
		FFortAthenaLivingWorldEvent* Event;
		UFortAthenaLivingWorldEventData* EventData;
		TArray<TWeakObjectPtr<AActor>> SpawnedActors;
		float NextSpawnTime;
	};

	struct FLivingWorldEventRequest
	{
		FFortAthenaLivingWorldCategory* Category;
		FFortAthenaLivingWorldEvent* Event;
	};

	static inline std::unordered_map<UFortAthenaLivingWorldEventData*, bool> PointProviderCache;

	template <typename T>
	void BindToPlaylistLoaded(T* Object);

	UFortAthenaLivingWorldManager* GetLivingWorldManager();
	void RegisterPointProvider(AActor* PointProvider);
	void UnregisterPointProvider(AActor* PointProvider);
	bool MatchesTagQuery(const FGameplayTagContainer& Tags, const FGameplayTagQuery& Query);
	bool MatchesPlaylistTags(UFortAthenaLivingWorldManager* _this, const FGameplayTagContainer& RequirePlaylistTags);
	bool GetProviderLocation(AActor* PointProvider, FVector& OutLocation, FRotator& OutRotation);
	AActor* PickPointProvider(UFortAthenaLivingWorldManager* _this, const FGameplayTagQuery& FiltersTagQuery);
	bool HasPointProvider(UFortAthenaLivingWorldManager* _this, UFortAthenaLivingWorldEventData* EventData);
	int32 GetSpawnedCount(UFortAthenaLivingWorldManager* _this, FFortAthenaLivingWorldCategory* Category, FFortAthenaLivingWorldEvent* Event);
	bool IsEventAvailable(UFortAthenaLivingWorldManager* _this, FFortAthenaLivingWorldEvent* Event);
	bool IsCategoryAvailable(UFortAthenaLivingWorldManager* _this, FFortAthenaLivingWorldCategory* Category);
	AActor* SpawnActorDescription(UFortAthenaLivingWorldManager* _this, FFortAthenaLivingWorldEventDataActorSpawnDescription& Description, const FVector& Location, const FRotator& Rotation);
	void SpawnEvent(UFortAthenaLivingWorldManager* _this, FFortAthenaLivingWorldCategory* Category, FFortAthenaLivingWorldEvent* Event);
	void AddActorOnMinimap(UFortAthenaLivingWorldManager* _this, AActor* Actor);
	void GenerateEvents(UFortAthenaLivingWorldManager* _this);
	void Update(UFortAthenaLivingWorldManager* _this);
	void Flush(UFortAthenaLivingWorldManager* _this);

	DefineHook(void, BeginPlay, UFortAthenaLivingWorldManager* _this);
	DefineHook(void, VolumePostInitializeComponents, AFortAthenaLivingWorldVolume* _this);
	DefineHook(void, VolumeBeginPlay, AFortAthenaLivingWorldVolume* _this);
	DefineHook(void, VolumeEndPlay, AFortAthenaLivingWorldVolume* _this, EEndPlayReason EndPlayReason);
	DefineHook(void, VolumeOnCurrentPlaylistLoaded, AFortAthenaLivingWorldVolume* _this, FName PlaylistName, const FGameplayTagContainer& PlaylistContextTags);
	DefineHook(void, OnEnvQueryFinished, AFortAthenaLivingWorldVolume* _this, void* Result);

	void execLivingWorldManagerRegisterPointProvider(UObject* Context, FFrame& Stack, void* const Result);
	void execLivingWorldManagerUnregisterPointProvider(UObject* Context, FFrame& Stack, void* const Result);
	void execLivingWorldManagerFlush(UObject* Context, FFrame& Stack, void* const Result);
	void execLivingWorldManagerGenerateEvents(UObject* Context, FFrame& Stack, void* const Result);
	void execLivingWorldManagerProviderDebugActor(UObject* Context, FFrame& Stack, void* const Result);
	void execLivingWorldManagerToggleActorMinimap(UObject* Context, FFrame& Stack, void* const Result);
	void execLivingWorldManagerToggleGenerateEvents(UObject* Context, FFrame& Stack, void* const Result);
	void execLivingWorldManagerToggleVerboseLogging(UObject* Context, FFrame& Stack, void* const Result);
	void execLivingWorldManagerSpawn(UObject* Context, FFrame& Stack, void* const Result);

	void Init();
}
