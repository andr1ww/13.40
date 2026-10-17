#include "pch.h"
#include "Core.h"
#include "Athena/AI/FortAthenaBTTask_Interact.h"
DEFINE_ENGINE_INIT(FortAthenaBTTask_Interact)

void FortAthenaBTTask_Interact::OnTaskFinished(UFortAthenaBTTask_Interact* _this, UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, int32 TaskResult)
{
	AFortAthenaAIBotController* Controller = TaskResult == static_cast<int32>(EBTNodeResult::Succeeded) ? Cast<AFortAthenaAIBotController>(OwnerComp->AIOwner) : nullptr;
	UFortInteractContextInfo* ContextInfo = Controller ? Controller->InteractContextInfo : nullptr;
	AFortPlayerPawn* Pawn = Controller ? Cast<AFortPlayerPawn>(Controller->Pawn) : nullptr;
	AActor* ReceivingActor = Pawn && ContextInfo ? ContextInfo->ReceivingActor : nullptr;

	if (AFortPickup* Pickup = Cast<AFortPickup>(ReceivingActor))
	{
		if (!Pickup->bPickedUp)
			Pickup->SetPickupTarget(Pawn, Pickup->DefaultFlyTime / Pawn->PickupSpeedMultiplier, FVector{});
	}
	else if (IFortInteractInterface* Interactable = GetInterfaceAddress<IFortInteractInterface>(ReceivingActor))
	{
		FInteractionType InteractType{};
		InteractType.Initialize(Pawn, ETInteractionType::IT_LongPress);
		InteractType.InteractionBeingAttempted = ContextInfo->InteractionBeingAttempted;
		InteractType.InteractComponent = ContextInfo->InteractComponent;
		InteractType.OptionalObjectData = ContextInfo->OptionalObjectData;

		Interactable->ServerOnAttemptInteract_Implementation(InteractType);
	}

	OnTaskFinishedOG(_this, OwnerComp, NodeMemory, TaskResult);
}

void FortAthenaBTTask_Interact::Init()
{
	Hooking::Hook<UFortAthenaBTTask_Interact>(0x2D0 / 8, OnTaskFinished, OnTaskFinishedOG);
}
