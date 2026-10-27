#include "pch.h"
#include "Core.h"
#include "Components/FortControllerComponent_IndicatedActorManagement.h"
DEFINE_ENGINE_INIT(FortControllerComponent_IndicatedActorManagement)

void FortControllerComponent_IndicatedActorManagement::AddActorsToStenciledList(UFortControllerComponent_IndicatedActorManagement *_this, TArray<class AActor *> &StenciledActors, const FStenciledActorData &Data, bool bAddAsUnique)
{
}

void FortControllerComponent_IndicatedActorManagement::AddActorsToIndicatedList(UFortControllerComponent_IndicatedActorManagement *_this, TArray<class AActor *> &IndicatedActors, const FIndicatedActorData &Data, bool bAddAsUnique)
{
}

void FortControllerComponent_IndicatedActorManagement::RemoveActorFromIndicatedList(UFortControllerComponent_IndicatedActorManagement *_this, class AActor *IndicatedActor, const bool bIncludeSquad)
{
	for (int32 i = _this->IndicatedActorList.Entries.Num() - 1; i >= 0; i--)
	{
		FIndicatedActorInfoEntry &Entry = _this->IndicatedActorList.Entries[i];
		if (Entry.Actor == IndicatedActor)
			_this->IndicatedActorList.Entries.Remove(i);
	}

	_this->IndicatedActorList.MarkArrayDirty();
}

void FortControllerComponent_IndicatedActorManagement::RemoveActorFromStenciledList(UFortControllerComponent_IndicatedActorManagement *_this, class AActor *StenciledActor, const bool bIncludeSquad)
{
	for (int32 i = _this->StenciledActorList.Entries.Num() - 1; i >= 0; i--)
	{
		FStenciledActorInfoEntry &Entry = _this->StenciledActorList.Entries[i];
		if (Entry.Actor == StenciledActor)
			_this->StenciledActorList.Entries.Remove(i);
	}

	_this->StenciledActorList.MarkArrayDirty();
}

void FortControllerComponent_IndicatedActorManagement::AddActorsInRadiusToIndicatedList(UFortIndicatedActorManagementLibrary *, FFrame &Stack)
{
	AFortPlayerControllerAthena *InstigatingController = nullptr;
	TArray<FIndicatedActorDataWithFilter> IndicatedActorFilterData;
	TArray<FIndicatedActorDataWithFilter> TempCopy;
	bool bAddAsUnique = false;

	IndicatedActorFilterData.Data = nullptr;
	IndicatedActorFilterData.NumElements = 0;
	IndicatedActorFilterData.MaxElements = 0;

	Stack.StepCompiledIn(&InstigatingController);
	if (Stack.Code)
	{
		FFrame::Step(&Stack, Stack.Object, &IndicatedActorFilterData);
	}
	else
	{
		FField *ArrayProp = Stack.PropertyChainForCompiledIn;
		Stack.PropertyChainForCompiledIn = ArrayProp->Next;
		FFrame::StepExplicitProperty(&Stack, &IndicatedActorFilterData, ArrayProp);
	}

	Stack.StepCompiledIn(&bAddAsUnique);
	Stack.IncrementCode();

	TempCopy.Data = nullptr;
	TempCopy.NumElements = 0;
	TempCopy.MaxElements = 0;

	if (IndicatedActorFilterData.Num() > 0)
	{
		TempCopy.Reserve(IndicatedActorFilterData.Num());
		for (int32 i = 0; i < IndicatedActorFilterData.Num(); i++)
		{
			TempCopy.Add(IndicatedActorFilterData[i]);
		}
	}

	if (InstigatingController->IsA<AFortAthenaAIBotController>())
		return;

	TArray<AActor *> IndicatedActors;

	TArray<AActor *> BuildingActors;
	UGameplayStatics::GetAllActorsOfClass(GWorld, ABuildingActor::StaticClass(), &BuildingActors);

	TArray<AActor *> PawnActors;
	UGameplayStatics::GetAllActorsOfClass(GWorld, AFortPawn::StaticClass(), &PawnActors);

	TArray<AActor *> Actors;

	for (int32 i = 0; i < BuildingActors.Num(); i++)
		Actors.Add(BuildingActors[i]);

	for (int32 i = 0; i < PawnActors.Num(); i++)
		Actors.Add(PawnActors[i]);

	AFortPlayerStateAthena *PlayerState =
		Cast<AFortPlayerStateAthena>(InstigatingController->PlayerState);

	TArray<AController *> Squad = PlayerState->PlayerTeam->TeamMembers;

	for (auto &Data : TempCopy)
	{
		for (int32 i = 0; i < Actors.Num(); i++)
		{
			AActor *Actor = Actors[i];
			if (!Actor)
				continue;

			if (ABuildingActor *BuildingActor = Cast<ABuildingActor>(Actor))
			{
				if (!BuildingActor->StaticGameplayTags.HasAny(Data.IndicatedActorTags))
					continue;
			}
			else if (AFortPawn *FortPawn = Cast<AFortPawn>(Actor))
			{
				if (!FortPawn->GameplayTags.HasAny(Data.IndicatedActorTags))
					continue;
			}
			else
				continue;

			if (UKismetMathLibrary::Vector_DistanceSquared(InstigatingController->MyFortPawn->K2_GetActorLocation(), Actor->K2_GetActorLocation()) > Data.OverlapRadius * Data.OverlapRadius)
				continue;

			IndicatedActors.Add(Actor);
		}
		if (IndicatedActors.Num() == 0)
			continue;

		for (int32 i = 0; i < Squad.Num(); i++)
		{
			AController *Base = Squad[i];
			if (!Base)
				continue;

			AFortPlayerControllerAthena *Controller = Cast<AFortPlayerControllerAthena>(Base);
			if (!Controller)
				continue;

			UFortControllerComponent_IndicatedActorManagement *Comp =
				GetComponentByClass<UFortControllerComponent_IndicatedActorManagement>(Controller);

			if (!Comp)
				continue;

			AddActorsToIndicatedList(Comp, IndicatedActors, Data.IndicatedData, bAddAsUnique);
		}
	}

	IndicatedActors.Free();
	Actors.Free();
	TempCopy.Free();
}

void FortControllerComponent_IndicatedActorManagement::AddActorsInRadiusToStenciledList(UFortIndicatedActorManagementLibrary *, FFrame &Stack)
{
	AFortPlayerControllerAthena *InstigatingController = nullptr;
	TArray<FIndicatedActorDataWithFilter> IndicatedActorFilterData;
	TArray<FIndicatedActorDataWithFilter> TempCopy;
	bool bAddAsUnique = false;

	IndicatedActorFilterData.Data = nullptr;
	IndicatedActorFilterData.NumElements = 0;
	IndicatedActorFilterData.MaxElements = 0;

	Stack.StepCompiledIn(&InstigatingController);
	if (Stack.Code)
	{
		FFrame::Step(&Stack, Stack.Object, &IndicatedActorFilterData);
	}
	else
	{
		FField *ArrayProp = Stack.PropertyChainForCompiledIn;
		Stack.PropertyChainForCompiledIn = ArrayProp->Next;
		FFrame::StepExplicitProperty(&Stack, &IndicatedActorFilterData, ArrayProp);
	}

	Stack.StepCompiledIn(&bAddAsUnique);
	Stack.IncrementCode();

	TempCopy.Data = nullptr;
	TempCopy.NumElements = 0;
	TempCopy.MaxElements = 0;

	if (IndicatedActorFilterData.Num() > 0)
	{
		TempCopy.Reserve(IndicatedActorFilterData.Num());
		for (int32 i = 0; i < IndicatedActorFilterData.Num(); i++)
		{
			TempCopy.Add(IndicatedActorFilterData[i]);
		}
	}

	if (InstigatingController->IsA<AFortAthenaAIBotController>())
		return;

	TArray<AActor *> IndicatedActors;

	TArray<AActor *> BuildingActors;
	UGameplayStatics::GetAllActorsOfClass(GWorld, ABuildingActor::StaticClass(), &BuildingActors);

	TArray<AActor *> PawnActors;
	UGameplayStatics::GetAllActorsOfClass(GWorld, AFortPawn::StaticClass(), &PawnActors);

	TArray<AActor *> Actors;

	for (int32 i = 0; i < BuildingActors.Num(); i++)
		Actors.Add(BuildingActors[i]);

	for (int32 i = 0; i < PawnActors.Num(); i++)
		Actors.Add(PawnActors[i]);

	AFortPlayerStateAthena *PlayerState = Cast<AFortPlayerStateAthena>(InstigatingController->PlayerState);
	TArray<AController *> Squad = PlayerState->PlayerTeam->TeamMembers;

	for (auto &Data : TempCopy)
	{
		for (int32 i = 0; i < Actors.Num(); i++)
		{
			AActor *Actor = Actors[i];
			if (!Actor)
				continue;

			if (ABuildingActor *BuildingActor = Cast<ABuildingActor>(Actor))
			{
				if (!BuildingActor->StaticGameplayTags.HasAny(Data.IndicatedActorTags))
					continue;
			}
			else if (AFortPawn *FortPawn = Cast<AFortPawn>(Actor))
			{
				if (!FortPawn->GameplayTags.HasAny(Data.IndicatedActorTags))
					continue;
			}
			else
				continue;

			if (UKismetMathLibrary::Vector_DistanceSquared(InstigatingController->MyFortPawn->K2_GetActorLocation(), Actor->K2_GetActorLocation()) > Data.OverlapRadius * Data.OverlapRadius)
				continue;

			IndicatedActors.Add(Actor);
		}
		if (IndicatedActors.Num() == 0)
			continue;

		for (int32 i = 0; i < Squad.Num(); i++)
		{
			AController *Base = Squad[i];
			if (!Base)
				continue;

			AFortPlayerControllerAthena *Controller = Cast<AFortPlayerControllerAthena>(Base);
			if (!Controller)
				continue;

			UFortControllerComponent_IndicatedActorManagement *Comp =
				(UFortControllerComponent_IndicatedActorManagement *)Controller->GetComponentByClass(UFortControllerComponent_IndicatedActorManagement::StaticClass());

			if (!Comp)
				continue;

			AddActorsToStenciledList(Comp, IndicatedActors, Data.StenciledData, bAddAsUnique);
		}
	}

	IndicatedActors.Free();
	Actors.Free();
	TempCopy.Free();
}

void FortControllerComponent_IndicatedActorManagement::RemoveActorFromIndicatedListKismet(UFortIndicatedActorManagementLibrary *, FFrame &Stack)
{
	AFortPlayerControllerAthena *InstigatingController = nullptr;
	AActor *IndicatedActor = nullptr;
	bool bIncludeSquad = false;
	Stack.StepCompiledIn(&InstigatingController);
	Stack.StepCompiledIn(&IndicatedActor);
	Stack.StepCompiledIn(&bIncludeSquad);
	Stack.IncrementCode();

	UFortControllerComponent_IndicatedActorManagement *Comp =
		GetComponentByClass<UFortControllerComponent_IndicatedActorManagement>(InstigatingController);

	if (!Comp)
		return;

	RemoveActorFromIndicatedList(Comp, IndicatedActor, bIncludeSquad);
}

void FortControllerComponent_IndicatedActorManagement::RemoveActorFromStenciledListKismet(UFortIndicatedActorManagementLibrary *, FFrame &Stack)
{
	AFortPlayerControllerAthena *InstigatingController = nullptr;
	AActor *StenciledActor = nullptr;
	bool bIncludeSquad = false;
	Stack.StepCompiledIn(&InstigatingController);
	Stack.StepCompiledIn(&StenciledActor);
	Stack.StepCompiledIn(&bIncludeSquad);
	Stack.IncrementCode();

	UFortControllerComponent_IndicatedActorManagement *Comp =
		GetComponentByClass<UFortControllerComponent_IndicatedActorManagement>(InstigatingController);

	if (!Comp)
		return;

	RemoveActorFromStenciledList(Comp, StenciledActor, bIncludeSquad);
}

void FortControllerComponent_IndicatedActorManagement::Init()
{
}