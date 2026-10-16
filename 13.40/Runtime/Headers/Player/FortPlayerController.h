#pragma once
#include "pch.h"

namespace FortPlayerController
{
	void ServerExecuteInventoryItem_Implementation(AFortPlayerController* _this, const FGuid& ItemGuid);
	ABuildingSMActor* ServerCreateBuildingActorInternal(AFortPlayerController* _this, FBuildingClassData* BuildingClassData, FVector* BuildLoc, FRotator* BuildRot, bool bMirrored, float SyncKey, bool bForceOverrideInteractBuildCheck, bool bIgnoreCostCheck, bool bIgnoreBuildingValidityCheck);
	void ServerCreateBuildingActor_Implementation(AFortPlayerController* _this, FCreateBuildingActorData& CreateBuildingData);
	void ServerBeginEditingBuildingActor_Implementation(AFortPlayerController* _this, ABuildingSMActor* BuildingActorToEdit);
	void ServerEditBuildingActor_Implementation(AFortPlayerController* _this, ABuildingSMActor* BuildingActorToEdit, TSubclassOf<ABuildingSMActor> NewBuildingClass, uint8 RotationIterations, bool bMirrored);
	void ServerEndEditingBuildingActor_Implementation(AFortPlayerController* _this, ABuildingSMActor* BuildingActorToStopEditing);
	void ServerRepairBuildingActor_Implementation(AFortPlayerController* _this, ABuildingSMActor* BuildingActorToRepair);
	void ServerAttemptInventoryDrop_Implementation(AFortPlayerController* _this, const FGuid& ItemGuid, int32 Count, bool bTrash);
	void ServerPlayEmoteItem_Implementation(AFortPlayerController* _this, UFortMontageItemDefinitionBase* EmoteAsset, float EmoteRandomNumber);
	void ServerCombineInventoryItems_Implementation(AFortPlayerController* _this, const FGuid& TargetItemGuid, const FGuid& SourceItemGuid);
	int32 RemoveItemFromPlayer(AFortPlayerController* _this, UFortWorldItemDefinition* ItemDefinition, int32 AmountToRemove, bool bForceRemoval);
	AActor* SpawnToyInstance(AFortPlayerController* _this, TSubclassOf<AActor> ToyClass, const FTransform& SpawnPosition);

	DefineHook(void, GetPlayerViewPoint, AFortPlayerController* _this, FVector* OutLocation, FRotator* OutRotation);

	void Init();
}
