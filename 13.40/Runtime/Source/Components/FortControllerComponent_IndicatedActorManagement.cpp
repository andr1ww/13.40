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
}

void FortControllerComponent_IndicatedActorManagement::RemoveActorFromStenciledList(UFortControllerComponent_IndicatedActorManagement *_this, class AActor *StenciledActor, const bool bIncludeSquad)
{
}

void FortControllerComponent_IndicatedActorManagement::AddActorsInRadiusToIndicatedList(UFortIndicatedActorManagementLibrary *, FFrame &Stack)
{
}

void FortControllerComponent_IndicatedActorManagement::AddActorsInRadiusToStenciledList(UFortIndicatedActorManagementLibrary *, FFrame &Stack)
{
}

void FortControllerComponent_IndicatedActorManagement::RemoveActorFromIndicatedListKismet(UFortIndicatedActorManagementLibrary *, FFrame &Stack)
{
}

void FortControllerComponent_IndicatedActorManagement::RemoveActorFromStenciledListKismet(UFortIndicatedActorManagementLibrary *, FFrame &Stack)
{
}

void FortControllerComponent_IndicatedActorManagement::Init()
{
}