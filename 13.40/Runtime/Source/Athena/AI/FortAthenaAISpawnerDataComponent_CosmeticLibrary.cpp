#include "pch.h"
#include "Core.h"
#include "Athena/AI/FortAthenaAISpawnerDataComponent_CosmeticLibrary.h"
DEFINE_ENGINE_INIT(FortAthenaAISpawnerDataComponent_CosmeticLibrary)

void UFortAthenaAIBotCosmeticLibraryData::PickLoadout(FFortAthenaLoadout& OutLoadout, const FGameplayTag& PredefinedCosmeticSetTag) const
{
	UDataTable* SetsDataTable = PredefinedCosmeticSetTag.TagName.ComparisonIndex != 0 && !OutLoadout.Character ? PredefineSetsDataTable.LoadSynchronous() : nullptr;

	FFortBotCosmeticItemSetDataTableRow* Set = PickWeightedRow<FFortBotCosmeticItemSetDataTableRow>(SetsDataTable, [&](FFortBotCosmeticItemSetDataTableRow* Row)
		{
			return Row->SetTag.TagName == PredefinedCosmeticSetTag.TagName;
		});

	if (Set)
	{
		OutLoadout.Character = LoadPrimaryAsset<UAthenaCharacterItemDefinition>(Set->CharacterAssetId);
		OutLoadout.Backpack = LoadPrimaryAsset<UAthenaBackpackItemDefinition>(Set->BackpackAssetId);
		OutLoadout.Glider = LoadPrimaryAsset<UAthenaGliderItemDefinition>(Set->GliderAssetId);
		OutLoadout.SkyDiveContrail = LoadPrimaryAsset<UAthenaSkyDiveContrailItemDefinition>(Set->SkyDiveContrailAssetId);
		OutLoadout.Pickaxe = LoadPrimaryAsset<UAthenaPickaxeItemDefinition>(Set->PickaxeAssetId);
	}

	auto PickItem = [](const TSoftObjectPtr<UDataTable>& DataTable) -> UObject*
		{
			FFortBotCosmeticItemDataTableRow* Row = PickWeightedRow<FFortBotCosmeticItemDataTableRow>(DataTable.LoadSynchronous(), [](FFortBotCosmeticItemDataTableRow* Candidate) { return true; });

			return Row ? LoadPrimaryAsset<UObject>(Row->PrimaryAssetId) : nullptr;
		};

	if (!OutLoadout.Character)
		OutLoadout.Character = Cast<UAthenaCharacterItemDefinition>(PickItem(CharactersDataTable));

	if (!OutLoadout.Character)
		OutLoadout.Character = Cast<UAthenaCharacterItemDefinition>(PickItem(FallbackCharactersDataTable));

	if (!OutLoadout.Backpack)
		OutLoadout.Backpack = Cast<UAthenaBackpackItemDefinition>(PickItem(BackpacksDataTable));

	if (!OutLoadout.Glider)
		OutLoadout.Glider = Cast<UAthenaGliderItemDefinition>(PickItem(GlidersDataTable));

	if (!OutLoadout.SkyDiveContrail)
		OutLoadout.SkyDiveContrail = Cast<UAthenaSkyDiveContrailItemDefinition>(PickItem(SkyDiveContrailsDataTable));

	if (!OutLoadout.Pickaxe)
		OutLoadout.Pickaxe = Cast<UAthenaPickaxeItemDefinition>(PickItem(PickaxesDataTable));
}

void FortAthenaAISpawnerDataComponent_CosmeticLibrary::GetLoadout_Implementation(UFortAthenaAISpawnerDataComponent_CosmeticLibrary* _this, FFortAthenaLoadout* OutLoadout)
{
	const int32 NumLibraries = _this->CosmeticLibraries.Num();
	const int32 StartIndex = NumLibraries > 0 ? rand() % NumLibraries : 0;

	for (int32 i = 0; i < NumLibraries; i++)
	{
		if (UFortAthenaAIBotCosmeticLibraryData* Library = _this->CosmeticLibraries[(StartIndex + i) % NumLibraries].LoadSynchronous())
			Library->PickLoadout(*OutLoadout, _this->PredefinedCosmeticSetTag);
	}
}

void FortAthenaAISpawnerDataComponent_CosmeticLibrary::GetDances_Implementation(UFortAthenaAISpawnerDataComponent_CosmeticLibrary* _this, TArray<UAthenaDanceItemDefinition*>* Dances)
{
	UDataTable* EmotesDataTable = _this->EmotesDataTable.LoadSynchronous();
	int32 MaxCount = static_cast<int32>(_this->EmotesMaxCount.Evaluate(0.f));

	if (MaxCount <= 0)
		MaxCount = INT32_MAX;

	TArray<FFortBotCosmeticItemDataTableRow*> PickedRows{};

	while (Dances->Num() < MaxCount)
	{
		FFortBotCosmeticItemDataTableRow* Row = PickWeightedRow<FFortBotCosmeticItemDataTableRow>(EmotesDataTable, [&](FFortBotCosmeticItemDataTableRow* Candidate)
			{
				return !PickedRows.Search([&](FFortBotCosmeticItemDataTableRow* Picked) { return Picked == Candidate; });
			});

		if (!Row)
			break;

		PickedRows.Add(Row);

		if (UAthenaDanceItemDefinition* Dance = LoadPrimaryAsset<UAthenaDanceItemDefinition>(Row->PrimaryAssetId))
			Dances->Add(Dance);
	}

	PickedRows.Free();
}

void FortAthenaAISpawnerDataComponent_CosmeticLibrary::Init()
{
	Hooking::Hook<UFortAthenaAISpawnerDataComponent_CosmeticLibrary>(0x268 / 8, GetDances_Implementation);
	Hooking::Hook<UFortAthenaAISpawnerDataComponent_CosmeticLibrary>(0x270 / 8, GetLoadout_Implementation);
}
