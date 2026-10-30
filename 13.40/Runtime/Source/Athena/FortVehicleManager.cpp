#include "pch.h"
#include "Core.h"
#include "Athena/FortVehicleManager.h"
DEFINE_ENGINE_INIT(FortVehicleManager)

using FOnVehicleSpawnsUpdated = TMulticastInlineDelegate<void(UFortVehicleItemDefinition*, TArray<AActor*>&, int32)>;

void FortVehicleManager::UpdateVehicleSpawns(AFortVehicleManager* _this)
{
	static AFortVehicleManager* LastManager = NULL;

	if (_this == NULL || GWorld == NULL || LastManager == _this)
		return;

	LastManager = _this;

	TArray<AFortAthenaVehicleSpawner*> Spawners = GetAll<AFortAthenaVehicleSpawner>();
	TArray<UFortVehicleItemDefinition*> ItemDefs;

	for (int32 i = Spawners.Num() - 1; i >= 0; i--)
	{
		if (Spawners[i] == NULL || Spawners[i]->GetVehicleClass() == NULL || Spawners[i]->CachedFortVehicleItemDef == NULL)
		{
			Spawners.Remove(i);
			continue;
		}

		if (!ItemDefs.Contains(Spawners[i]->CachedFortVehicleItemDef))
			ItemDefs.Add(Spawners[i]->CachedFortVehicleItemDef);
	}

	for (int32 i = Spawners.Num() - 1; i > 0; i--)
	{
		const int32 j = (int32)(FRand(nullptr) * (float)(i + 1));

		if (j < 0 || j >= i)
			continue;

		AFortAthenaVehicleSpawner* Temp = Spawners[i];
		Spawners[i] = Spawners[j];
		Spawners[j] = Temp;
	}

	FOnVehicleSpawnsUpdated* OnVehicleSpawnsUpdated = reinterpret_cast<FOnVehicleSpawnsUpdated*>(reinterpret_cast<uint8*>(_this) + 0x220);

	for (int32 i = 0; i < ItemDefs.Num(); i++)
	{
		UFortVehicleItemDefinition* ItemDef = ItemDefs[i];

		const float MinPercent = std::clamp(ItemDef->VehicleMinSpawnPercent.Evaluate(0.f) * 0.01f, 0.f, 1.f);
		const float MaxPercent = std::clamp(ItemDef->VehicleMaxSpawnPercent.Evaluate(0.f) * 0.01f, 0.f, 1.f);
		const float SpawnPercent = MinPercent + FRand(nullptr) * max(MaxPercent - MinPercent, 0.f);
		const int32 MaxNumberCanSpawn = (int32)ItemDef->MaxNumberCanSpawn.Evaluate(0.f);

		int32 Total = 0;

		for (int32 j = 0; j < Spawners.Num(); j++)
		{
			if (Spawners[j]->CachedFortVehicleItemDef == ItemDef)
				Total++;
		}

		int32 Quota = (int32)((float)Total * SpawnPercent + 0.5f);

		if (MaxNumberCanSpawn > 0)
			Quota = min(Quota, MaxNumberCanSpawn);

		TArray<AActor*> Vehicles;
		int32 Seen = 0;

		for (int32 j = 0; j < Spawners.Num(); j++)
		{
			AFortAthenaVehicleSpawner* Spawner = Spawners[j];

			if (Spawner->CachedFortVehicleItemDef != ItemDef || (!Spawner->GetForceSpawnAlways() && Seen++ >= Quota))
				continue;

			FActorSpawnParameters SpawnParameters{};
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			if (AActor* Vehicle = GWorld->SpawnActor(Spawner->GetVehicleClass(), Spawner->K2_GetActorLocation(), Spawner->K2_GetActorRotation(), SpawnParameters))
				Vehicles.Add(Vehicle);
		}

		OnVehicleSpawnsUpdated->Broadcast(ItemDef, Vehicles, Vehicles.Num());
	}

	Spawners.Free();
	ItemDefs.Free();
}

void FortVehicleManager::Init()
{
	Hooking::Rel32Swap(ImageBase + 0x1F1C4FA, UpdateVehicleSpawns);
}
