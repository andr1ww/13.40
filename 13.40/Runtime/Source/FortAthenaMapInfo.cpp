#include "pch.h"
#include "Core.h"
#include "FortAthenaMapInfo.h"
#include "Actors/BuildingActor.h"
DEFINE_ENGINE_INIT(FortAthenaMapInfo)

struct FContainerSpawnQuota
{
	UClass* Class;
	float Percent;
	int32 Total;
	int32 Quota;
	int32 Seen;
};

float PickSpawnPercent(FScalableFloat& Min, FScalableFloat& Max)
{
	float MinPercent = Min.Evaluate(0.0f);
	float MaxPercent = Max.Evaluate(0.0f);

	float Percent = MinPercent;

	if (MaxPercent > Percent)
		Percent += FRand(nullptr) * (MaxPercent - Percent);

	return std::clamp(Percent, 0.0f, 1.0f);
}

bool FortAthenaMapInfo::UpdateBuildingContainerSpawns(AFortAthenaMapInfo* _this)
{
	if (_this == NULL || GWorld == NULL)
		return false;

	TArray<ABuildingContainer*> Actors = GetAll<ABuildingContainer>();

	if (Actors.Num() == 0)
	{
		Actors.Free();
		return false;
	}

	for (int32 i = Actors.Num() - 1; i > 0; i--)
	{
		const int32 j = (int32)(FRand(nullptr) * (float)(i + 1));

		if (j < 0 || j >= i)
			continue;

		ABuildingContainer* Temp = Actors[i];
		Actors[i] = Actors[j];
		Actors[j] = Temp;
	}

	std::vector<FContainerSpawnQuota> Quotas;

	for (int32 i = 0; i < _this->TreasureChestSpawnInfos.Num(); i++)
	{
		FFortTreasureChestSpawnInfo& Info = _this->TreasureChestSpawnInfos[i];

		if (Info.TreasureChestClass)
			Quotas.push_back({ Info.TreasureChestClass, PickSpawnPercent(Info.TreasureChestMinSpawnPercent, Info.TreasureChestMaxSpawnPercent), 0, 0, 0 });
	}

	for (int32 i = 0; i < _this->AmmoBoxSpawnInfos.Num(); i++)
	{
		FFortAmmoBoxSpawnInfo& Info = _this->AmmoBoxSpawnInfos[i];

		if (Info.AmmoBoxClass)
			Quotas.push_back({ Info.AmmoBoxClass, PickSpawnPercent(Info.AmmoBoxMinSpawnPercent, Info.AmmoBoxMaxSpawnPercent), 0, 0, 0 });
	}

	auto FindQuota = [&](ABuildingContainer* Container) -> FContainerSpawnQuota*
		{
			for (FContainerSpawnQuota& Quota : Quotas)
			{
				if (Container->IsA(Quota.Class))
					return &Quota;
			}

			return nullptr;
		};

	for (int32 i = 0; i < Actors.Num(); i++)
	{
		if (FContainerSpawnQuota* Quota = Actors[i] ? FindQuota(Actors[i]) : nullptr)
			Quota->Total++;
	}

	for (FContainerSpawnQuota& Quota : Quotas)
		Quota.Quota = (int32)((float)Quota.Total * Quota.Percent);

	bool bSpawnedAny = false;

	for (int32 i = 0; i < Actors.Num(); i++)
	{
		ABuildingContainer* Container = Actors[i];

		if (Container == NULL)
			continue;

		bool bShouldExist = true;

		if (FContainerSpawnQuota* Quota = FindQuota(Container))
			bShouldExist = Quota->Seen++ < Quota->Quota;

		if (!bShouldExist)
		{
			if (Container->bDestroyContainerOnSearch)
			{
				Container->K2_DestroyActor();
				continue;
			}

			Container->SetActorHiddenInGame(true);
			Container->SetActorEnableCollision(false);
			continue;
		}

		if (Container->bStartAlreadySearched_Athena)
			BuildingActor::SpawnLoot(Container, NULL, EFortPickupSourceTypeFlag::FloorLoot, EFortPickupSpawnSource::Unset, false);

		bSpawnedAny = true;
	}

	Actors.Free();

	return bSpawnedAny;
}

void FortAthenaMapInfo::Init()
{
	Hooking::Rel32Swap(ImageBase + 0x1EBCAC4, UpdateBuildingContainerSpawns);
}
