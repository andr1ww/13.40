#include "pch.h"
#include "Core.h"
#include "Athena/FortPoiVolume.h"
DEFINE_ENGINE_INIT(FortPoiVolume)

void FortPoiVolume::PostInitializeComponents(AFortPoiVolume* _this)
{
	static UStaticMesh* Cube = StaticLoadObject<UStaticMesh>(L"/Engine/BasicShapes/Cube.Cube");

	if (_this->BrushComponent || !_this->Brush || !Cube || !Cube->BodySetup)
		return PostInitializeComponentsOG(_this);

	FVector Min{}, Max{}, RelatedMin{}, RelatedMax{};
	bool bFound = false, bRelated = false;

	auto Include = [](FVector& BoxMin, FVector& BoxMax, bool& bValid, const FVector& Location)
	{
		BoxMin = bValid ? UKismetMathLibrary::Vector_ComponentMin(BoxMin, Location) : Location;
		BoxMax = bValid ? UKismetMathLibrary::Vector_ComponentMax(BoxMax, Location) : Location;
		bValid = true;
	};

	auto IsRelated = [_this](FName OtherTag)
	{
		std::string Other = OtherTag.ToString();
		std::string Leaf = Other.substr(Other.rfind('.') + 1);
		std::string Pair = Other.substr(Other.rfind('.', Other.rfind('.') - 1) + 1);
		std::erase(Pair, '.');

		for (int32 i = 0; i < _this->LocationTags.GameplayTags.Num(); i++)
		{
			std::string Tag = _this->LocationTags.GameplayTags[i].TagName.ToString();

			if ((Leaf.size() >= 4 && Tag.ends_with(Leaf)) || Tag.ends_with(Pair))
				return true;
		}

		return false;
	};

	UFortQuestIndicatorData* QuestIndicatorData = UFortGameData::GetGameData() ? UFortGameData::GetGameData()->QuestIndicatorData : nullptr;
	UDataTable* QuestLocations = QuestIndicatorData ? QuestIndicatorData->QuestTagToLocationDataTable : nullptr;

	for (int32 i = 0; i < _this->LocationTags.GameplayTags.Num() && !bFound; i++)
	{
		if (FFortQuestTagToLocationDataRow* Row = FindRow<FFortQuestTagToLocationDataRow>(QuestLocations, _this->LocationTags.GameplayTags[i].TagName))
			Include(Min, Max, bFound, Row->WorldLocation);
	}

	for (int32 i = 0; i < GWorld->StreamingLevels.Num() && !bFound; i++)
	{
		ULevelStreamingAlwaysLoaded* StreamingLevel = Cast<ULevelStreamingAlwaysLoaded>(GWorld->StreamingLevels[i]);
		UWorld* World = StreamingLevel ? StreamingLevel->WorldAsset.LoadSynchronous() : nullptr;
		AFortWorldSettings* WorldSettings = World && World->PersistentLevel ? Cast<AFortWorldSettings>(World->PersistentLevel->WorldSettings) : nullptr;

		for (int32 j = 0; WorldSettings && j < WorldSettings->BuildingFoundationStreamingData.Num(); j++)
		{
			FBuildingFoundationStreamingData& StreamingData = WorldSettings->BuildingFoundationStreamingData[j];

			if (UBlueprintGameplayTagLibrary::HasAnyTags(StreamingData.GameplayTags, _this->LocationTags, true))
			{
				Include(Min, Max, bFound, StreamingData.FoundationLocation);
				continue;
			}

			for (int32 k = 0; k < StreamingData.GameplayTags.GameplayTags.Num(); k++)
			{
				if (!IsRelated(StreamingData.GameplayTags.GameplayTags[k].TagName))
					continue;

				Include(RelatedMin, RelatedMax, bRelated, StreamingData.FoundationLocation);
				break;
			}
		}
	}

	if (!bFound)
	{
		TArray<ABuildingFoundation*> Foundations = GetAll<ABuildingFoundation>();

		for (int32 i = 0; i < Foundations.Num() && !bFound; i++)
		{
			if (!Foundations[i])
				continue;

			FVector Location = Foundations[i]->K2_GetActorLocation() + Foundations[i]->MapLocationTextOffset;

			if (UBlueprintGameplayTagLibrary::HasTag(_this->LocationTags, Foundations[i]->MapLocationTag, true))
				Include(Min, Max, bFound, Location);
			else if (IsRelated(Foundations[i]->MapLocationTag.TagName))
				Include(RelatedMin, RelatedMax, bRelated, Location);
		}

		Foundations.Free();

		ForEachRow<FFortQuestTagToLocationDataRow>(QuestLocations, [&](FName RowName, FFortQuestTagToLocationDataRow* Row)
			{
				if (IsRelated(RowName))
					Include(RelatedMin, RelatedMax, bRelated, Row->WorldLocation);
			});
	}

	if (!bFound && bRelated && UKismetMathLibrary::VSize(RelatedMax - RelatedMin) * 0.5f <= UKismetMathLibrary::Sqrt(_this->CurrentFortPoiVolumeSize / 3.f))
		Include(Min, Max, bFound, (RelatedMin + RelatedMax) * 0.5f);

	if (!bFound)
		return PostInitializeComponentsOG(_this);

	FTransform Identity = UKismetMathLibrary::MakeTransform(FVector{}, FRotator{}, FVector{ 1.f, 1.f, 1.f });
	UBrushComponent* BrushComponent = Cast<UBrushComponent>(_this->AddComponentByClass(UBrushComponent::StaticClass(), false, Identity, true));

	if (!BrushComponent)
		return PostInitializeComponentsOG(_this);

	BrushComponent->Brush = _this->Brush;
	BrushComponent->BrushBodySetup = Cube->BodySetup;
	BrushComponent->SetMobility(EComponentMobility::Movable);
	BrushComponent->SetCollisionProfileName(FName(L"OverlapAll"), false);

	_this->FinishAddComponent(BrushComponent, false, Identity);
	_this->BrushComponent = BrushComponent;

	FVector Origin, Extent;
	float SphereRadius;
	UKismetSystemLibrary::GetComponentBounds(BrushComponent, &Origin, &Extent, &SphereRadius);

	float Area = 4.f * (Extent.X * Extent.Y + Extent.Y * Extent.Z + Extent.Z * Extent.X);
	float Scale = _this->CurrentFortPoiVolumeSize > 0.f && Area > 0.f ? UKismetMathLibrary::Sqrt(_this->CurrentFortPoiVolumeSize / Area) : 1.f;

	BrushComponent->SetWorldScale3D(Extent * (Scale / 50.f));
	_this->K2_SetActorLocation((Min + Max) * 0.5f, false, nullptr, true);

	if (_this->PoiCollisionComp)
	{
		_this->PoiCollisionComp->SetMobility(EComponentMobility::Movable);
		_this->PoiCollisionComp->K2_AttachToComponent(BrushComponent, FName(), EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, false);
	}

	PostInitializeComponentsOG(_this);
}

void FortPoiVolume::Init()
{
	Hooking::Hook<AFortPoiVolume>(0x480 / 8, PostInitializeComponents, PostInitializeComponentsOG);
}
