#include "pch.h"
#include "Core.h"
#include "Actors/BuildingFoundation.h"
DEFINE_ENGINE_INIT(BuildingFoundation)

void BuildingFoundation::execSetDynamicFoundationEnabled(UObject* Context, FFrame& Stack, void* const Result)
{
	bool bEnabled = Stack.StepCompiledInRef<bool>();
	Stack.IncrementCode();

	ABuildingFoundation* _this = static_cast<ABuildingFoundation*>(Context);

	if (_this->DynamicFoundationType == EDynamicFoundationType::Static)
		return;

	const EDynamicFoundationEnabledState EnabledState = bEnabled ? EDynamicFoundationEnabledState::Enabled : EDynamicFoundationEnabledState::Disabled;

	if (_this->FoundationEnabledState == EnabledState)
		return;

	_this->FoundationEnabledState = EnabledState;
	_this->DynamicFoundationRepData.EnabledState = EnabledState;

	if (EnabledState == EDynamicFoundationEnabledState::Enabled)
		_this->SelectAndSetupMyBuildingLevel();

	_this->OnRep_DynamicFoundationRepData();
	_this->SetDynamicFoundationTransform(_this->GetTransform());
}

void BuildingFoundation::execSetDynamicFoundationTransform(UObject* Context, FFrame& Stack, void* const Result)
{
	FTransform NewTransform = Stack.StepCompiledInRef<FTransform>();
	Stack.IncrementCode();

	ABuildingFoundation* _this = static_cast<ABuildingFoundation*>(Context);

	if (_this->DynamicFoundationType == EDynamicFoundationType::Static)
		return;

	_this->DynamicFoundationTransform = NewTransform;
	_this->DynamicFoundationRepData.Translation = NewTransform.Translation;
	_this->DynamicFoundationRepData.Rotation = UKismetMathLibrary::Quat_Rotator(NewTransform.Rotation);
	_this->StreamingData.FoundationLocation = NewTransform.Translation;

	_this->OnRep_DynamicFoundationRepData();
}

bool BuildingFoundation::StreamAdditionalWorld(ABuildingFoundation* _this)
{
	if (_this->LevelStreamInfo || _this->AdditionalWorlds.Num() == 0)
		return false;

	std::vector<std::string> PackageNames;

	for (auto& World : _this->AdditionalWorlds)
	{
		if (World.ObjectID.AssetPathName.ComparisonIndex == 0)
			continue;

		std::string AssetPath = World.ObjectID.AssetPathName.GetRawString();
		PackageNames.push_back(AssetPath.substr(0, AssetPath.find('.')));
	}

	if (PackageNames.empty())
		return false;

	const std::string& PackageName = PackageNames[UKismetMathLibrary::RandomInteger(static_cast<int32>(PackageNames.size()))];

	_this->LevelToStream = FName(PackageName.c_str());
	_this->bServerStreamedInLevel = true;

	if (!_this->StreamInMyBuilding())
	{
		_this->LevelToStream = FName();
		_this->bServerStreamedInLevel = false;
		return false;
	}

	_this->ForceNetUpdate();

	std::string FoundationName = _this->GetName();
	UE_LOG(LogTemp, Log, TEXT("BuildingFoundation::StreamAdditionalWorld: %s streaming %s"), std::wstring(FoundationName.begin(), FoundationName.end()).c_str(), std::wstring(PackageName.begin(), PackageName.end()).c_str());
	return true;
}

void BuildingFoundation::SelectAndSetupMyBuildingLevel(ABuildingFoundation* _this, void* RandomStream)
{
	SelectAndSetupMyBuildingLevelOG(_this, RandomStream);

	StreamAdditionalWorld(_this);
}

void BuildingFoundation::Init()
{
	Hooking::Hook(ImageBase + 0x2160960, SelectAndSetupMyBuildingLevel, SelectAndSetupMyBuildingLevelOG);

	UClass* FoundationClass = ABuildingFoundation::StaticClass();

	Hooking::ExecHook(FoundationClass->GetFunction("SetDynamicFoundationEnabled"), execSetDynamicFoundationEnabled);
	Hooking::ExecHook(FoundationClass->GetFunction("SetDynamicFoundationTransform"), execSetDynamicFoundationTransform);
}
