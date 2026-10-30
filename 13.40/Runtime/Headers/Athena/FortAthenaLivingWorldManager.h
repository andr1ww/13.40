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

	UFortAthenaLivingWorldManager* GetLivingWorldManager();
	void RegisterPointProvider(AActor* PointProvider);
	void UnregisterPointProvider(AActor* PointProvider);
	bool GetProviderLocation(AActor* PointProvider, FVector& OutLocation, FRotator& OutRotation);
	AActor* PickPointProvider(UFortAthenaLivingWorldManager* _this, const FGameplayTagQuery& FiltersTagQuery, FVector& OutLocation, FRotator& OutRotation);
	bool IsEventAvailable(UFortAthenaLivingWorldManager* _this, FFortAthenaLivingWorldEvent* Event);
	bool IsCategoryAvailable(UFortAthenaLivingWorldManager* _this, FFortAthenaLivingWorldCategory* Category);
	AActor* SpawnActorDescription(UFortAthenaLivingWorldManager* _this, FFortAthenaLivingWorldEventDataActorSpawnDescription& Description, const FVector& Location, const FRotator& Rotation);
	int32 SpawnEvent(UFortAthenaLivingWorldManager* _this, FFortAthenaLivingWorldCategory* Category, FFortAthenaLivingWorldEvent* Event);
	void GenerateEvents(UFortAthenaLivingWorldManager* _this);
	void Update(UFortAthenaLivingWorldManager* _this);
	void Flush(UFortAthenaLivingWorldManager* _this);

	DefineHook(void, BeginPlay, UFortAthenaLivingWorldManager* _this);
	DefineHook(void, VolumeBeginPlay, AFortAthenaLivingWorldVolume* _this);
	DefineHook(void, OnEnvQueryFinished, AFortAthenaLivingWorldVolume* _this, void* Result);

	void execLivingWorldManagerRegisterPointProvider(UObject* Context, FFrame& Stack, void* const Result);
	void execLivingWorldManagerUnregisterPointProvider(UObject* Context, FFrame& Stack, void* const Result);
	void execLivingWorldManagerFlush(UObject* Context, FFrame& Stack, void* const Result);
	void execLivingWorldManagerGenerateEvents(UObject* Context, FFrame& Stack, void* const Result);
	void execLivingWorldManagerToggleGenerateEvents(UObject* Context, FFrame& Stack, void* const Result);
	void execLivingWorldManagerToggleVerboseLogging(UObject* Context, FFrame& Stack, void* const Result);
	void execLivingWorldManagerSpawn(UObject* Context, FFrame& Stack, void* const Result);

	void Init();
}
