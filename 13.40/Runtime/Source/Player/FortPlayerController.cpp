#include "pch.h"
#include "Core.h"
#include "Player/FortPlayerController.h"
DEFINE_ENGINE_INIT(FortPlayerController)

void FortPlayerController::ServerExecuteInventoryItem_Implementation(AFortPlayerController* _this, const FGuid& ItemGuid)
{
	if (!ItemGuid.IsValid() || !_this->WorldInventory)
		return;

	if (UFortWorldItem* Item = _this->WorldInventory->GetItem(ItemGuid))
	{
		if (Item->ItemEntry.ItemDefinition)
			Item->ItemEntry.ItemDefinition->ServerExecute(Item, _this);
	}
}

ABuildingSMActor* FortPlayerController::ServerCreateBuildingActorInternal(AFortPlayerController* _this, FBuildingClassData* BuildingClassData, FVector* BuildLoc, FRotator* BuildRot, bool bMirrored, float SyncKey, bool bForceOverrideInteractBuildCheck, bool bIgnoreCostCheck, bool bIgnoreBuildingValidityCheck)
{
	if (!_this->FixUpCreateBuildingClassData(*BuildingClassData))
		return NULL;

	UClass* BuildingClass = BuildingClassData->BuildingClass;

	if (BuildingClass == NULL || !BuildingClass->IsSubclassOf(ABuildingActor::StaticClass()) || !BuildingClass->IsSubclassOf(ABuildingSMActor::StaticClass()))
		return NULL;

	if (!bForceOverrideInteractBuildCheck && !_this->CanPerformNativeAction(UFortGlobals::GameplayTags()->ActionPlayerInteractBuild))
		return NULL;

	bool bBuildingBlocked = _this->IsBuildingRestricted(*BuildingClassData, *BuildLoc, *BuildRot);

	TArray<ABuildingActor*> OverlappingActors;
	EFortStructuralGridQueryResults PlacementResult = EFortStructuralGridQueryResults::CanAdd;
	bool bCanAfford = bIgnoreCostCheck;
	bool bHasRequiredBuildingLevel = false;
	EFortResourceType ResourceType = static_cast<ABuildingSMActor*>(BuildingClass->ClassDefaultObject)->ResourceType;

	if (!bBuildingBlocked)
	{
		EFortBuildPreviewMarkerOptionalAdjustment MarkerAdjustment{};

		if (!bIgnoreCostCheck)
			bCanAfford = _this->CanAffordToPlaceBuildableClass(*BuildingClassData);

		bHasRequiredBuildingLevel = _this->HasRequiredBuildingLevel(BuildingClassData->UpgradeLevel, ResourceType);

		if (!bIgnoreBuildingValidityCheck)
			PlacementResult = _this->CanPlaceBuildableClassInStructuralGrid(BuildingClass, *BuildLoc, *BuildRot, bMirrored, &OverlappingActors, &MarkerAdjustment);
	}

	if (bCanAfford && bHasRequiredBuildingLevel && PlacementResult == EFortStructuralGridQueryResults::CanAdd)
	{
		for (ABuildingActor* Existing : OverlappingActors)
		{
			if (Existing && Existing->bDestroyOnPlayerBuildingPlacement)
				Existing->Die(UFortGlobals::GameplayTags()->EffectInstantDeath);
		}

		OverlappingActors.Free();

		FActorSpawnParameters SpawnParameters{};
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ABuildingSMActor* NewBuilding = static_cast<ABuildingSMActor*>(GWorld->SpawnActor(BuildingClass, *BuildLoc, *BuildRot, SpawnParameters));

		if (NewBuilding != NULL)
		{
			NewBuilding->CurrentBuildingLevel = BuildingClassData->UpgradeLevel;
			NewBuilding->SetMirrored(bMirrored);
			NewBuilding->InitializeKismetSpawnedBuildingActor(NewBuilding, _this, true);

			_this->PayBuildableClassPlacementCost(*BuildingClassData);

			if (AFortGameMode* GameMode = Cast<AFortGameMode>(GWorld->AuthorityGameMode))
				GameMode->ScoreBuildingConstruction(_this, NewBuilding);

			if (AFortPlayerPawn* Pawn = _this->MyFortPawn)
			{
				FVector NoiseLocation = NewBuilding->RootComponent ? NewBuilding->RootComponent->K2_GetComponentLocation() : FVector{};

				UFortAIFunctionLibrary::MakeNoiseEventAtLocation(Pawn, 1000.0f, NoiseLocation, FName());
				Pawn->OnBlueprintPlace();

				if (AFortWeap_BuildingTool* Tool = Cast<AFortWeap_BuildingTool>(Pawn->CurrentWeapon))
					Tool->PlayWeaponFireFX(false);
			}
		}

		return NewBuilding;
	}

	OverlappingActors.Free();

	FText FailureReason;

	if (bBuildingBlocked)
		FailureReason = *reinterpret_cast<FText*>(ImageBase + 0x8C39D98);
	else
		FailureReason = _this->GetErrorMessageFromStructuralQuery(PlacementResult);

	if (!FailureReason.IsEmpty())
		_this->ClientSendMessage(FailureReason, nullptr);

	_this->TriggerUIFeedbackEvent(!bBuildingBlocked && !bCanAfford ? FName(L"BuildPreviewUnableToAfford") : FName(L"BuildPreviewUnableToPlace"));

	return NULL;
}

void FortPlayerController::ServerCreateBuildingActor_Implementation(AFortPlayerController* _this, FCreateBuildingActorData& CreateBuildingData)
{
	if (!CreateBuildingData.BuildingClassData.BuildingClass)
	{
		if (AFortGameStateZone* GameState = Cast<AFortGameStateZone>(GWorld->GameState))
		{
			for (auto& Pair : GameState->AllPlayerBuildableClassesIndexLookup)
			{
				if (Pair.Value() == CreateBuildingData.BuildingClassHandle)
				{
					CreateBuildingData.BuildingClassData.BuildingClass = Pair.Key();
					break;
				}
			}
		}
	}

	ServerCreateBuildingActorInternal(_this, &CreateBuildingData.BuildingClassData, &CreateBuildingData.BuildLoc, &CreateBuildingData.BuildRot,
		CreateBuildingData.bMirrored, CreateBuildingData.SyncKey, false, false, false);
}

void FortPlayerController::ServerBeginEditingBuildingActor_Implementation(AFortPlayerController* _this, ABuildingSMActor* BuildingActorToEdit)
{
	if (BuildingActorToEdit == NULL || _this->MyFortPawn == NULL)
		return;

	if (!BuildingActorToEdit->CanBePlayerEdited(_this))
		return;

	AFortPlayerStateZone* PlayerState = Cast<AFortPlayerStateZone>(_this->PlayerState);

	if (PlayerState == NULL)
		return;

	BuildingActorToEdit->SetEditingPlayer(PlayerState);

	UFortEditToolItemDefinition* EditToolItem = UFortGameData::GetGameData()->EditToolItem.LoadSynchronous();
	UFortWorldItem* WorldItem = _this->WorldInventory ? _this->WorldInventory->FindExistingItemForDefinition(EditToolItem, false) : NULL;

	if (EditToolItem != NULL && WorldItem != NULL && EditToolItem->ServerExecute(WorldItem, _this))
	{
		if (AFortWeap_EditingTool* EditToolWeapon = Cast<AFortWeap_EditingTool>(_this->MyFortPawn->CurrentWeapon))
			EditToolWeapon->SetEditActor(BuildingActorToEdit);
	}
}

void FortPlayerController::ServerEditBuildingActor_Implementation(AFortPlayerController* _this, ABuildingSMActor* BuildingActorToEdit, TSubclassOf<ABuildingSMActor> NewBuildingClass, uint8 RotationIterations, bool bMirrored)
{
	if (BuildingActorToEdit == NULL || BuildingActorToEdit->EditingPlayer != _this->PlayerState || BuildingActorToEdit->bDestroyed)
		return;

	BuildingActorToEdit->SetEditingPlayer(NULL);

	int32 CurrentBuildingLevel = BuildingActorToEdit->CurrentBuildingLevel;
	BuildingActorToEdit->ReplaceBuildingActor(EBuildingReplacementType::BRT_Edited, NewBuildingClass, CurrentBuildingLevel, RotationIterations, bMirrored, _this);

	if (AFortGameMode* GameMode = Cast<AFortGameMode>(GWorld->AuthorityGameMode))
		GameMode->ScoreBuildingEdit(_this, BuildingActorToEdit);
}

void FortPlayerController::ServerEndEditingBuildingActor_Implementation(AFortPlayerController* _this, ABuildingSMActor* BuildingActorToStopEditing)
{
	if (BuildingActorToStopEditing == NULL || _this->MyFortPawn == NULL || BuildingActorToStopEditing->EditingPlayer != _this->PlayerState || BuildingActorToStopEditing->bDestroyed)
		return;

	BuildingActorToStopEditing->SetEditingPlayer(NULL);

	UFortEditToolItemDefinition* EditToolItem = UFortGameData::GetGameData()->EditToolItem.LoadSynchronous();
	UFortWorldItem* WorldItem = _this->WorldInventory ? _this->WorldInventory->FindExistingItemForDefinition(EditToolItem, false) : NULL;

	if (EditToolItem != NULL && WorldItem != NULL)
	{
		if (AFortWeap_EditingTool* EditToolWeapon = Cast<AFortWeap_EditingTool>(_this->MyFortPawn->CurrentWeapon))
			EditToolWeapon->SetEditActor(NULL);
	}
}

void FortPlayerController::ServerRepairBuildingActor_Implementation(AFortPlayerController* _this, ABuildingSMActor* BuildingActorToRepair)
{
	if (BuildingActorToRepair == NULL)
		return;

	const bool bNeedsRepair = BuildingActorToRepair->NeedsRepair();

	const FRotator BuildingRotation = BuildingActorToRepair->K2_GetActorRotation();
	const FVector BuildingLocation = BuildingActorToRepair->K2_GetActorLocation();

	if (_this->HasPermissionToEditWorld(BuildingActorToRepair, BuildingLocation, BuildingRotation))
	{
		if (BuildingActorToRepair->IsBeingPlayerEdited())
		{
			_this->ClientSendMessage(*reinterpret_cast<FText*>(ImageBase + 0x8C39E70), nullptr);
			_this->TriggerUIFeedbackEvent(FName(L"BuildPreviewUnableToPlace"));
			return;
		}
	}
	else if (!bNeedsRepair)
	{
		_this->ClientSendMessage(*reinterpret_cast<FText*>(ImageBase + 0x8C39E88), nullptr);
		_this->TriggerUIFeedbackEvent(FName(L"BuildPreviewUnableToPlace"));
		return;
	}

	if (!bNeedsRepair)
		return;

	if (!_this->CanAffordToRepair(BuildingActorToRepair))
	{
		_this->ClientSendMessage(*reinterpret_cast<FText*>(ImageBase + 0x8C39E40), nullptr);
		_this->TriggerUIFeedbackEvent(FName(L"BuildPreviewUnableToAfford"));
		return;
	}

	const int32 ResourcesSpent = _this->PayBuildingRepairCost(BuildingActorToRepair);

	BuildingActorToRepair->RepairBuilding(_this, ResourcesSpent);

	if (AFortGameMode* GameMode = Cast<AFortGameMode>(GWorld->AuthorityGameMode))
		GameMode->ScoreBuildingRepair(_this, BuildingActorToRepair);

	if (_this->MyFortPawn != NULL)
		UFortAIFunctionLibrary::MakeNoiseEventAtLocation(_this->MyFortPawn, 1000.0f, BuildingLocation, FName());
}

void FortPlayerController::Init()
{
	Hooking::HookEvery<AFortPlayerController>(0x1068 / 8, ServerExecuteInventoryItem_Implementation);
	Hooking::HookEvery<AFortPlayerController>(0x1180 / 8, ServerCreateBuildingActor_Implementation);
	Hooking::HookEvery<AFortPlayerController>(0x11B8 / 8, ServerBeginEditingBuildingActor_Implementation);
	Hooking::HookEvery<AFortPlayerController>(0x1190 / 8, ServerEditBuildingActor_Implementation);
	Hooking::HookEvery<AFortPlayerController>(0x11A8 / 8, ServerEndEditingBuildingActor_Implementation);
	Hooking::HookEvery<AFortPlayerController>(0x1160 / 8, ServerRepairBuildingActor_Implementation);

	Hooking::Rel32Swap(ImageBase + 0x2C95C15, ServerCreateBuildingActorInternal);
	Hooking::Rel32Swap(ImageBase + 0x2C960B9, ServerCreateBuildingActorInternal);
}
