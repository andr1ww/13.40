#pragma once
#include "pch.h"

namespace FortControllerComponent_IndicatedActorManagement
{
	// comp
	void AddActorsToStenciledList(UFortControllerComponent_IndicatedActorManagement *_this, TArray<class AActor *> &StenciledActors, const struct FStenciledActorData &Data, bool bAddAsUnique);
	void AddActorsToIndicatedList(UFortControllerComponent_IndicatedActorManagement *_this, TArray<class AActor *> &IndicatedActors, const struct FIndicatedActorData &Data, bool bAddAsUnique);
	void RemoveActorFromIndicatedList(UFortControllerComponent_IndicatedActorManagement *_this, class AActor *IndicatedActor, const bool bIncludeSquad);
	void RemoveActorFromStenciledList(UFortControllerComponent_IndicatedActorManagement *_this, class AActor *StenciledActor, const bool bIncludeSquad);

	// library
	void AddActorsInRadiusToIndicatedList(UFortIndicatedActorManagementLibrary *, FFrame &Stack);
	void AddActorsInRadiusToStenciledList(UFortIndicatedActorManagementLibrary *, FFrame &Stack);
	void RemoveActorFromIndicatedListKismet(UFortIndicatedActorManagementLibrary *, FFrame &Stack);
	void RemoveActorFromStenciledListKismet(UFortIndicatedActorManagementLibrary *, FFrame &Stack);

	void Init();
}