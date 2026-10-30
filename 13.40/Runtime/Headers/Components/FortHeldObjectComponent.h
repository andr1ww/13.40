#pragma once
#include "pch.h"

namespace FortHeldObjectComponent
{
	void AttachToOwningPawn(UFortHeldObjectComponent* _this);
	bool PerformPlacementTrace(UFortHeldObjectComponent* _this, FHitResult& OutHit);

	UFortProjectileMovementComponent* GetProjectileMovement(UFortHeldObjectComponent* _this);
	void SetIgnorePawnWhenMoving(UFortHeldObjectComponent* _this, AFortPlayerPawn* PlayerPawn, bool bIgnore);
	void BindWeaponUnequipped(UFortHeldObjectComponent* _this, AFortPlayerPawn* PlayerPawn, bool bBind);

	void ValidateDetachLocation(UFortHeldObjectComponent* _this, FVector& InOutDetachLocation);
	void DetachFromOwningPawn(UFortHeldObjectComponent* _this);
	void SetupOwningPawn(UFortHeldObjectComponent* _this, AFortPlayerPawn* PlayerPawn);
	void ClearOwningPawn(UFortHeldObjectComponent* _this);
	void SetCollisionBlockedByPawns(UFortHeldObjectComponent* _this, bool bBlocked);
	bool EquipBackingWeapon(UFortHeldObjectComponent* _this);
	void UnequipBackingWeapon(UFortHeldObjectComponent* _this, AFortPlayerPawn* PlayerPawn);
	void StartProjectileMovement(UFortHeldObjectComponent* _this, const FVector& InVelocity);
	void StopProjectileMovement(UFortHeldObjectComponent* _this);

	void PickupHeldObject(UFortHeldObjectComponent* _this, AFortPlayerPawn* PlayerPawn);
	void DropHeldObject(UFortHeldObjectComponent* _this);
	void PlaceHeldObject(UFortHeldObjectComponent* _this);
	void ThrowHeldObject(UFortHeldObjectComponent* _this, const FVector& DetachLocation, const FRotator& ThrowDirection);
	void ThrownObjectStopped(UFortHeldObjectComponent* _this, const FHitResult& ImpactResult);
	void OnThrowComplete(UFortHeldObjectComponent* _this);
	void OnWeaponUnequipped(UFortHeldObjectComponent* _this);
	void HandleOwnerDestroyed(UFortHeldObjectComponent* _this, AActor* DestroyedActor);
	void HandleOwnerAsBuildingActorDestroyed(UFortHeldObjectComponent* _this, AActor* DamagedActor);

	void execPickupHeldObject(UObject* Context, FFrame& Stack, void* const Result);
	void execDropHeldObject(UObject* Context, FFrame& Stack, void* const Result);
	void execPlaceHeldObject(UObject* Context, FFrame& Stack, void* const Result);
	void execThrowHeldObject(UObject* Context, FFrame& Stack, void* const Result);
	void execThrownObjectStopped(UObject* Context, FFrame& Stack, void* const Result);
	void execOnThrowComplete(UObject* Context, FFrame& Stack, void* const Result);
	void execOnWeaponUnequipped(UObject* Context, FFrame& Stack, void* const Result);
	void execHandleOwnerDestroyed(UObject* Context, FFrame& Stack, void* const Result);
	void execHandleOwnerAsBuildingActorDestroyed(UObject* Context, FFrame& Stack, void* const Result);

	void Init();
}
