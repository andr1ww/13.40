#include "pch.h"
#include "Core.h"
#include "Athena/AI/FortAthenaAIBotCustomizationData.h"
DEFINE_ENGINE_INIT(FortAthenaAIBotCustomizationData)

void FortAthenaAIBotCustomizationData::GetOverrideCosmeticLoadout(const UFortAthenaAIBotCustomizationData* _this, AFortPlayerPawnAthena* Pawn, FFortAthenaLoadout& OutLoadout, const FGameplayTag& PredefinedCosmeticSetTag)
{
	if (_this->OverrideCosmeticMode == EBotDataOverrideCosmeticMode::SpecificLoadout)
	{
		if (UFortAthenaAIBotCharacterCustomization* CharacterCustomization = _this->CharacterCustomization)
		{
			const FFortAthenaLoadout& Loadout = CharacterCustomization->CustomizationLoadout;

			OutLoadout.Character = Loadout.Character;
			OutLoadout.Backpack = Loadout.Backpack;
			OutLoadout.Glider = Loadout.Glider;
			OutLoadout.SkyDiveContrail = Loadout.SkyDiveContrail;
			OutLoadout.Pickaxe = Loadout.Pickaxe;
		}

		return;
	}

	if (UFortAthenaAIBotCosmeticLibraryData* Library = _this->CosmeticCustomizationLibrary.LoadSynchronous())
		Library->PickLoadout(OutLoadout, PredefinedCosmeticSetTag);
}

void FortAthenaAIBotCustomizationData::Init()
{
	Hooking::Rel32Swap(ImageBase + 0x1FC835D, GetOverrideCosmeticLoadout);
}
