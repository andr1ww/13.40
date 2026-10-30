#pragma once
#include "pch.h"

namespace FortControllerComponent_IndicatedActorManagement
{
	// comp
	void AddActorsToIndicatedList(UFortControllerComponent_IndicatedActorManagement* _this, const TArray<AActor*>& IndicatedActors, const FIndicatedActorData& Data, bool bAddAsUnique);
	void AddActorsToStenciledList(UFortControllerComponent_IndicatedActorManagement* _this, const TArray<AActor*>& StenciledActors, const FStenciledActorData& Data, bool bAddAsUnique);
	void RemoveActorFromIndicatedList(UFortControllerComponent_IndicatedActorManagement* _this, AActor* IndicatedActor, const bool bIncludeSquad);
	void RemoveActorFromStenciledList(UFortControllerComponent_IndicatedActorManagement* _this, AActor* StenciledActor, const bool bIncludeSquad);
	void RemoveGroupFromIndicatedList(UFortControllerComponent_IndicatedActorManagement* _this, const FString& GroupIdentifier, const bool bIncludeSquad);
	void RemoveGroupFromStenciledList(UFortControllerComponent_IndicatedActorManagement* _this, const FString& GroupIdentifier, const bool bIncludeSquad);

	void execAddActorsToIndicatedList(UObject* Context, FFrame& Stack, void* const Result);
	void execAddActorsToStenciledList(UObject* Context, FFrame& Stack, void* const Result);
	void execRemoveActorFromIndicatedList(UObject* Context, FFrame& Stack, void* const Result);
	void execRemoveActorFromStenciledList(UObject* Context, FFrame& Stack, void* const Result);
	void execRemoveGroupFromIndicatedList(UObject* Context, FFrame& Stack, void* const Result);
	void execRemoveGroupFromStenciledList(UObject* Context, FFrame& Stack, void* const Result);

	// library
	void execLibraryAddActorsInRadiusToIndicatedList(UObject* Context, FFrame& Stack, void* const Result);
	void execLibraryAddActorsInRadiusToStenciledList(UObject* Context, FFrame& Stack, void* const Result);
	void execLibraryAddActorsToIndicatedList(UObject* Context, FFrame& Stack, void* const Result);
	void execLibraryAddActorsToStenciledList(UObject* Context, FFrame& Stack, void* const Result);
	void execLibraryRemoveActorFromIndicatedList(UObject* Context, FFrame& Stack, void* const Result);
	void execLibraryRemoveActorFromStenciledList(UObject* Context, FFrame& Stack, void* const Result);
	void execLibraryRemoveGroupFromIndicatedList(UObject* Context, FFrame& Stack, void* const Result);
	void execLibraryRemoveGroupFromStenciledList(UObject* Context, FFrame& Stack, void* const Result);

	void Init();
}
