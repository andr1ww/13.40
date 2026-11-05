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

void FortPlayerController::ServerAttemptInventoryDrop_Implementation(AFortPlayerController* _this, const FGuid& ItemGuid, int32 Count, bool bTrash)
{
	AFortPlayerPawn* Pawn = _this->MyFortPawn;
	AFortInventory* Inventory = _this->WorldInventory;

	if (Count <= 0 || !Pawn || Pawn->bIsDBNO || Pawn->bIsSkydiving || !Inventory || !_this->CanRemoveInventoryItem(ItemGuid))
		return;

	UFortWorldItem* Item = Inventory->GetItem(ItemGuid);
	UFortWorldItemDefinition* ItemDefinition = Item ? Cast<UFortWorldItemDefinition>(Item->ItemEntry.ItemDefinition) : nullptr;

	if (!ItemDefinition || Item->ItemEntry.Count <= 0)
		return;

	Count = std::clamp(Count, 1, Item->ItemEntry.Count);

	FFortCreatePickupData CreatePickupData{};
	CreatePickupData.World = GWorld;
	CreatePickupData.ItemEntry = Item->ItemEntry;
	CreatePickupData.ItemEntry.SetCount(Count);
	CreatePickupData.ItemEntry.SetItemGuid(NewGuid());
	CreatePickupData.PickupSourceTypeFlags = EFortPickupSourceTypeFlag::Player;

	FVector Forward = Pawn->GetActorForwardVector();
	Forward.Z = 0.f;
	Forward.Normalize();

	CreatePickupData.SpawnLocation = Pawn->K2_GetActorLocation() + Forward * 70.f + FVector{ 0.f, 0.f, 50.f };
	FVector DropTarget = Pawn->K2_GetActorLocation() + Forward * 300.f;

	AFortPickup* Pickup = bTrash || ItemDefinition->DropBehavior == EWorldItemDropBehavior::DestroyOnDrop ? nullptr : AFortPickup::CreateFromData(CreatePickupData);

	if (!Inventory->RemoveInventoryItem(ItemGuid, Count, false))
	{
		if (Pickup)
			Pickup->K2_DestroyActor();

		return;
	}

	if (!Pickup)
		return;

	Pickup->PawnWhoDroppedPickup = Pawn;
	Pickup->TossPickup(DropTarget, Pawn, -1, true, true, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset);
}

void FortPlayerController::ServerPlayEmoteItem_Implementation(AFortPlayerController* _this, UFortMontageItemDefinitionBase* EmoteAsset, float EmoteRandomNumber)
{
	AFortPlayerPawn* PlayerPawn = _this->MyFortPawn;

	if (PlayerPawn == NULL)
		return;

	UClass* AbilityClass = NULL;

	if (UAthenaToyItemDefinition* ToyItemDefinition = Cast<UAthenaToyItemDefinition>(EmoteAsset))
	{
		AbilityClass = ToyItemDefinition->ToySpawnAbility.LoadSynchronous();
	}
	else if (UAthenaSprayItemDefinition* SprayItemDefinition = Cast<UAthenaSprayItemDefinition>(EmoteAsset))
	{
		AbilityClass = UGameDataCosmetics::GetGameDataCosmetics()->SprayGameplayAbility.LoadSynchronous();
	}
	else if (UAthenaDanceItemDefinition* DanceItemDefinition = Cast<UAthenaDanceItemDefinition>(EmoteAsset))
	{
		UClass* CustomDanceAbility = DanceItemDefinition->CustomDanceAbility.LoadSynchronous();

		AbilityClass = CustomDanceAbility != NULL ? CustomDanceAbility : UGameDataCosmetics::GetGameDataCosmetics()->EmoteGameplayAbility.LoadSynchronous();

		PlayerPawn->bMovingEmote = DanceItemDefinition->bMovingEmote;
		PlayerPawn->EmoteWalkSpeed = DanceItemDefinition->WalkForwardSpeed;
		PlayerPawn->bMovingEmoteForwardOnly = DanceItemDefinition->bMoveForwardOnly;
	}

	if (AbilityClass != NULL)
	{
		PlayerPawn->EmoteRandomNum = EmoteRandomNumber;

		FGameplayAbilitySpec Spec;
		Spec.ConstructAbilitySpec(AbilityClass->ClassDefaultObject, 1, -1, EmoteAsset);

		PlayerPawn->AbilitySystemComponent->GiveAbilityAndActivateOnce(Spec);
	}
}

void FortPlayerController::ServerCombineInventoryItems_Implementation(AFortPlayerController* _this, const FGuid& TargetItemGuid, const FGuid& SourceItemGuid)
{
	if (TargetItemGuid == SourceItemGuid)
		return;

	AFortInventory* Inventory = _this->WorldInventory;

	if (Inventory == NULL)
		return;

	UFortWorldItem* TargetItem = Inventory->GetItem(TargetItemGuid);
	UFortWorldItem* SourceItem = Inventory->GetItem(SourceItemGuid);

	if (TargetItem == NULL || SourceItem == NULL)
		return;

	UFortWorldItemDefinition* TargetDefinition = Cast<UFortWorldItemDefinition>(TargetItem->ItemEntry.ItemDefinition);

	if (TargetDefinition == NULL || SourceItem->ItemEntry.ItemDefinition != TargetItem->ItemEntry.ItemDefinition)
		return;

	int32 MaxStackSize = max(1, AFortInventory::GetItemMaxStackSize(_this, TargetDefinition));
	int32 AmountToTransfer = min(SourceItem->ItemEntry.Count, MaxStackSize - TargetItem->ItemEntry.Count);

	if (AmountToTransfer <= 0)
		return;

	TargetItem->SetNumInStack(TargetItem->ItemEntry.Count + AmountToTransfer, false);
	Inventory->RemoveInventoryItem(SourceItemGuid, AmountToTransfer, false);
	Inventory->HandleInventoryLocalUpdate();
}

int32 FortPlayerController::RemoveItemFromPlayer(AFortPlayerController* _this, UFortWorldItemDefinition* ItemDefinition, int32 AmountToRemove, bool bForceRemoval)
{
	AFortInventory* Inventory = _this != NULL ? _this->WorldInventory : NULL;

	if (Inventory == NULL || ItemDefinition == NULL)
		return 0;

	int32 AmountRemoved = 0;

	for (int32 ItemIndex = Inventory->Inventory.ItemInstances.Num() - 1; ItemIndex >= 0; ItemIndex--)
	{
		UFortWorldItem* Item = Inventory->Inventory.ItemInstances[ItemIndex];

		if (Item == NULL || Item->ItemEntry.ItemDefinition != ItemDefinition)
			continue;

		int32 CountToRemove = AmountToRemove < 0 ? Item->ItemEntry.Count : min(Item->ItemEntry.Count, AmountToRemove - AmountRemoved);

		if (CountToRemove <= 0 && !bForceRemoval)
			continue;

		if (!Inventory->RemoveInventoryItem(Item->ItemEntry.ItemGuid, CountToRemove, bForceRemoval))
			continue;

		AmountRemoved += CountToRemove;

		if (AmountToRemove >= 0 && AmountRemoved >= AmountToRemove)
			break;
	}

	return AmountRemoved;
}

void FortPlayerController::GetPlayerViewPoint(AFortPlayerController* _this, FVector* OutLocation, FRotator* OutRotation)
{
	if (_this->MyFortPawn)
	{
		*OutLocation = _this->MyFortPawn->K2_GetActorLocation();
		*OutRotation = _this->MyFortPawn->GetControlRotation();
		return;
	}

	if (_this->StateName == NAME_Spectating && _this->Role == ENetRole::ROLE_Authority)
	{
		if (AActor* ViewTarget = _this->GetViewTarget())
		{
			*OutLocation = ViewTarget->K2_GetActorLocation();
			*OutRotation = ViewTarget->K2_GetActorRotation();
			return;
		}
	}

	return GetPlayerViewPointOG(_this, OutLocation, OutRotation);
}

AActor* FortPlayerController::SpawnToyInstance(AFortPlayerController* _this, TSubclassOf<AActor> ToyClass, const FTransform& SpawnPosition)
{
	UClass* ToyActorClass = ToyClass.Get();

	if (_this == NULL || ToyActorClass == NULL)
		return NULL;

	static int32** NumToysAlivePerPlayer = reinterpret_cast<int32**>(ImageBase + 0x8C39C90);

	int32 MaxToysAlive = **NumToysAlivePerPlayer;

	for (int32 ToyIndex = _this->ActiveToyInstances.Num() - 1; ToyIndex >= 0; ToyIndex--)
	{
		AActor* ActiveToyInstance = _this->ActiveToyInstances[ToyIndex];

		if (ActiveToyInstance == NULL || ActiveToyInstance->IsActorBeingDestroyed())
			_this->ActiveToyInstances.Remove(ToyIndex);
	}

	while (_this->ActiveToyInstances.Num() > 0 && _this->ActiveToyInstances.Num() >= MaxToysAlive)
	{
		AActor* OldestToyInstance = _this->ActiveToyInstances[0];

		if (IFortToyInterface* OldestToyInterface = GetInterface<IFortToyInterface>(OldestToyInstance))
			OldestToyInterface->StartToyFadeOutDueToNewPlacement();
		else
			OldestToyInstance->K2_DestroyActor();

		_this->ActiveToyInstances.Remove(0);
	}

	int32& NumTimesSummoned = _this->ToySummonCounts.FindOrAdd(ToyActorClass);
	NumTimesSummoned++;

	AActor* ToyOwner = _this->PlayerState != NULL ? static_cast<AActor*>(_this->PlayerState) : static_cast<AActor*>(_this);

	AActor* ToyInstance = UGameplayStatics::BeginDeferredActorSpawnFromClass(_this, ToyClass, SpawnPosition, ESpawnActorCollisionHandlingMethod::AlwaysSpawn, ToyOwner);

	if (ToyInstance == NULL)
		return NULL;

	UGameplayStatics::FinishSpawningActor(ToyInstance, SpawnPosition);

	if (IFortToyInterface* ToyInterface = GetInterface<IFortToyInterface>(ToyInstance))
		ToyInterface->InitializeToyInstance(_this, NumTimesSummoned);

	_this->ActiveToyInstances.Add(ToyInstance);

	return ToyInstance;
}

void FortPlayerController::OnPossess(AFortPlayerController* _this, APawn* InPawn)
{
	OnPossessOG(_this, InPawn);

	if (_this->Role != ENetRole::ROLE_Authority || InPawn == NULL)
		return;

	_this->OnPlayerPawnPossessed.Broadcast(InPawn);
}

void FortPlayerController::Init()
{
	Hooking::HookEvery<AFortPlayerController>(0x1068 / 8, ServerExecuteInventoryItem_Implementation);
	Hooking::HookEvery<AFortPlayerController>(0x10E8 / 8, ServerAttemptInventoryDrop_Implementation);
	Hooking::HookEvery<AFortPlayerController>(0xE48 / 8, ServerPlayEmoteItem_Implementation);
	Hooking::HookEvery<AFortPlayerController>(0x10F8 / 8, ServerCombineInventoryItems_Implementation);
	Hooking::HookEvery<AFortPlayerController>(0x1180 / 8, ServerCreateBuildingActor_Implementation);
	Hooking::HookEvery<AFortPlayerController>(0x11B8 / 8, ServerBeginEditingBuildingActor_Implementation);
	Hooking::HookEvery<AFortPlayerController>(0x1190 / 8, ServerEditBuildingActor_Implementation);
	Hooking::HookEvery<AFortPlayerController>(0x11A8 / 8, ServerEndEditingBuildingActor_Implementation);
	Hooking::HookEvery<AFortPlayerController>(0x1160 / 8, ServerRepairBuildingActor_Implementation);

	Hooking::Hook(ImageBase + 0x295B660, GetPlayerViewPoint, GetPlayerViewPointOG);
	Hooking::Hook(ImageBase + 0x2975710, OnPossess, OnPossessOG);
	Hooking::Hook(ImageBase + 0x24D8E50, RemoveItemFromPlayer);

	Hooking::Rel32Swap(ImageBase + 0x2C95C15, ServerCreateBuildingActorInternal);
	Hooking::Rel32Swap(ImageBase + 0x2C960B9, ServerCreateBuildingActorInternal);
	Hooking::Rel32Swap(ImageBase + 0x306EB5C, SpawnToyInstance);
}
