#pragma once
#include "pch.h"

namespace FortAthenaAISpawnerDataComponent_CosmeticLibrary
{
	void GetLoadout_Implementation(UFortAthenaAISpawnerDataComponent_CosmeticLibrary* _this, FFortAthenaLoadout* OutLoadout);
	void GetDances_Implementation(UFortAthenaAISpawnerDataComponent_CosmeticLibrary* _this, TArray<UAthenaDanceItemDefinition*>* Dances);

	void Init();
}
