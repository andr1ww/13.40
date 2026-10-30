#include "pch.h"
#include "Core.h"
#include "Components/FortHeldObjectComponent.h"
#include "Engine/TimerManager.h"
DEFINE_ENGINE_INIT(FortHeldObjectComponent)

void FortHeldObjectComponent::AttachToOwningPawn(UFortHeldObjectComponent* _this)
{
	reinterpret_cast<void(*)(UFortHeldObjectComponent*)>(ImageBase + 0x2274440)(_this);
}

bool FortHeldObjectComponent::PerformPlacementTrace(UFortHeldObjectComponent* _this, FHitResult& OutHit)
{
	return reinterpret_cast<bool(*)(UFortHeldObjectComponent*, FHitResult*)>(ImageBase + 0x2284410)(_this, &OutHit);
}

UFortProjectileMovementComponent* FortHeldObjectComponent::GetProjectileMovement(UFortHeldObjectComponent* _this)
{
	if (!_this->ProjectileMovementComponent)
	{
		if (AActor* Owner = _this->GetOwner())
			_this->ProjectileMovementComponent = Cast<UFortProjectileMovementComponent>(Owner->GetComponentByClass(UFortProjectileMovementComponent::StaticClass()));
	}

	return _this->ProjectileMovementComponent;
}

void FortHeldObjectComponent::SetIgnorePawnWhenMoving(UFortHeldObjectComponent* _this, AFortPlayerPawn* PlayerPawn, bool bIgnore)
{
	AActor* Owner = _this->GetOwner();
	if (!Owner || !PlayerPawn)
		return;

	if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Owner->RootComponent))
		RootPrimitive->IgnoreActorWhenMoving(PlayerPawn, bIgnore);
}

void FortHeldObjectComponent::BindWeaponUnequipped(UFortHeldObjectComponent* _this, AFortPlayerPawn* PlayerPawn, bool bBind)
{
	if (!PlayerPawn)
		return;

	if (bBind)
	{
		FScriptDelegate UnequippedDelegate;
		UnequippedDelegate.BindUFunction(_this, FName(L"OnWeaponUnequipped"));
		PlayerPawn->OnWeaponUnEquippedDelegate.AddUnique(UnequippedDelegate);
	}
	else
	{
		PlayerPawn->OnWeaponUnEquippedDelegate.Remove(_this, FName(L"OnWeaponUnequipped"));
	}
}

void FortHeldObjectComponent::ValidateDetachLocation(UFortHeldObjectComponent* _this, FVector& InOutDetachLocation)
{
	AActor* Owner = _this->GetOwner();
	AFortPlayerPawn* OwningPawn = _this->OwningPawn.Get();

	if (!Owner || !OwningPawn)
		return;

	FVector BoundsOrigin{};
	FVector BoundsExtent{};
	Owner->GetActorBounds(true, &BoundsOrigin, &BoundsExtent, false);

	const float Radius = min(min(BoundsExtent.X, BoundsExtent.Y), BoundsExtent.Z);

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwningPawn);
	ActorsToIgnore.Add(Owner);

	FHitResult Hit{};
	if (UKismetSystemLibrary::SphereTraceSingleByProfile(Owner, OwningPawn->K2_GetActorLocation(), InOutDetachLocation, Radius, FName(L"FortPickup"), false, ActorsToIgnore, EDrawDebugTrace::None, &Hit, true, FLinearColor(), FLinearColor(), 0.f) && Hit.bBlockingHit)
	{
		InOutDetachLocation = FVector{ Hit.Location.X, Hit.Location.Y, Hit.Location.Z };
	}
}

void FortHeldObjectComponent::DetachFromOwningPawn(UFortHeldObjectComponent* _this)
{
	AActor* Owner = _this->GetOwner();
	if (!Owner)
		return;

	Owner->FlushNetDormancy();
	Owner->K2_AddActorLocalOffset(FVector{ -_this->AttachmentOffset.X, -_this->AttachmentOffset.Y, -_this->AttachmentOffset.Z }, false, nullptr, false);
	Owner->K2_DetachFromActor(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld);
	Owner->ForceNetUpdate();
}

void FortHeldObjectComponent::SetupOwningPawn(UFortHeldObjectComponent* _this, AFortPlayerPawn* PlayerPawn)
{
	AActor* Owner = _this->GetOwner();
	if (!Owner || !PlayerPawn)
		return;

	Owner->FlushNetDormancy();

	if (!Owner->bReplicateMovement)
		Owner->SetReplicateMovement(true);

	_this->PreviousOwningPawn = nullptr;
	_this->OwningPawn = PlayerPawn;
	_this->ObjectPickupRotation = Owner->K2_GetActorRotation();
	_this->HeldObjectState = EHeldObjectState::Held;
	_this->OnRep_OwningPawn();

	PlayerPawn->HeldObject = Owner;

	if (AFortPlayerController* PlayerController = Cast<AFortPlayerController>(PlayerPawn->Controller))
	{
		PlayerController->bHoldingObject = true;
		PlayerController->OnRep_HoldingObject();
		PlayerController->ForceNetUpdate();
	}

	Owner->ForceNetUpdate();
	PlayerPawn->ForceNetUpdate();
}

void FortHeldObjectComponent::ClearOwningPawn(UFortHeldObjectComponent* _this)
{
	AActor* Owner = _this->GetOwner();
	AFortPlayerPawn* OwningPawn = _this->OwningPawn.Get();

	if (Owner)
		Owner->FlushNetDormancy();

	if (OwningPawn)
	{
		BindWeaponUnequipped(_this, OwningPawn, false);

		if (OwningPawn->HeldObject.Get() == Owner)
			OwningPawn->HeldObject = nullptr;

		if (AFortPlayerController* PlayerController = Cast<AFortPlayerController>(OwningPawn->Controller))
		{
			PlayerController->bHoldingObject = false;
			PlayerController->OnRep_HoldingObject();
			PlayerController->ForceNetUpdate();
		}

		OwningPawn->ForceNetUpdate();
	}

	_this->PreviousOwningPawn = OwningPawn;
	_this->OwningPawn = nullptr;
	_this->OnRep_OwningPawn();

	if (Owner)
		Owner->ForceNetUpdate();
}

void FortHeldObjectComponent::SetCollisionBlockedByPawns(UFortHeldObjectComponent* _this, bool bBlocked)
{
	if (_this->bCollisionBlockedByPawns == bBlocked)
		return;

	_this->bCollisionBlockedByPawns = bBlocked;
	_this->OnRep_bCollisionBlockedByPawns();

	if (AActor* Owner = _this->GetOwner())
		Owner->ForceNetUpdate();
}

bool FortHeldObjectComponent::EquipBackingWeapon(UFortHeldObjectComponent* _this)
{
	AFortPlayerPawn* OwningPawn = _this->OwningPawn.Get();
	UFortWeaponItemDefinition* WeaponDefinition = _this->EquippedWeaponItemDefinition.Get();
	AFortPlayerController* PlayerController = OwningPawn ? Cast<AFortPlayerController>(OwningPawn->Controller) : nullptr;

	if (!WeaponDefinition || !PlayerController || !PlayerController->WorldInventory)
		return false;

	AFortInventory* Inventory = PlayerController->WorldInventory;

	FFortItemEntry ItemEntry(WeaponDefinition, 1, 0);
	UFortWorldItem* Item = Inventory->GiveItemToInventoryOwner(ItemEntry);

	if (!Item)
		return false;

	Inventory->SetRequiresUpdate();
	Inventory->HandleInventoryLocalUpdate();

	_this->GrantedWeaponItem = Item;

	PlayerController->ServerExecuteInventoryItem(Item->ItemEntry.ItemGuid);
	PlayerController->ClientEquipItem(Item->ItemEntry.ItemGuid, true);

	if (OwningPawn->CurrentWeapon && OwningPawn->CurrentWeapon->WeaponData == WeaponDefinition)
		_this->GrantedWeapon = OwningPawn->CurrentWeapon;

	BindWeaponUnequipped(_this, OwningPawn, true);
	return true;
}

void FortHeldObjectComponent::UnequipBackingWeapon(UFortHeldObjectComponent* _this, AFortPlayerPawn* PlayerPawn)
{
	UFortWorldItem* GrantedItem = _this->GrantedWeaponItem.Get();

	_this->GrantedWeaponItem = nullptr;
	_this->GrantedWeapon = nullptr;

	BindWeaponUnequipped(_this, PlayerPawn, false);

	AFortPlayerController* PlayerController = PlayerPawn ? Cast<AFortPlayerController>(PlayerPawn->Controller) : nullptr;
	if (!GrantedItem || !PlayerController || !PlayerController->WorldInventory)
		return;

	const FGuid GrantedItemGuid = GrantedItem->ItemEntry.ItemGuid;
	const bool bWasEquipped = PlayerPawn->CurrentWeapon && PlayerPawn->CurrentWeapon->ItemEntryGuid == GrantedItemGuid;
	AFortWeapon* PreviousWeapon = PlayerPawn->PreviousWeapon;

	PlayerController->WorldInventory->RemoveInventoryItem(GrantedItemGuid, 1, true);

	if (!bWasEquipped || !PreviousWeapon || PreviousWeapon->ItemEntryGuid == GrantedItemGuid)
		return;

	if (!PlayerController->WorldInventory->GetItem(PreviousWeapon->ItemEntryGuid))
		return;

	PlayerController->ServerExecuteInventoryItem(PreviousWeapon->ItemEntryGuid);
	PlayerController->ClientEquipItem(PreviousWeapon->ItemEntryGuid, true);
}

void FortHeldObjectComponent::StartProjectileMovement(UFortHeldObjectComponent* _this, const FVector& InVelocity)
{
	AActor* Owner = _this->GetOwner();
	UFortProjectileMovementComponent* ProjectileMovement = GetProjectileMovement(_this);

	if (!Owner || !ProjectileMovement)
		return;

	FScriptDelegate StopDelegate;
	StopDelegate.BindUFunction(_this, FName(L"ThrownObjectStopped"));
	ProjectileMovement->OnProjectileStop.AddUnique(StopDelegate);

	ProjectileMovement->SetUpdatedComponent(Owner->RootComponent);
	ProjectileMovement->ProjectileGravityScale = _this->GetGravityScale();
	ProjectileMovement->Velocity = InVelocity;
	ProjectileMovement->Activate(true);
	ProjectileMovement->Velocity = InVelocity;

	Owner->ForceNetUpdate();
}

void FortHeldObjectComponent::StopProjectileMovement(UFortHeldObjectComponent* _this)
{
	UFortProjectileMovementComponent* ProjectileMovement = GetProjectileMovement(_this);
	if (!ProjectileMovement)
		return;

	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->Deactivate();
}

void FortHeldObjectComponent::PickupHeldObject(UFortHeldObjectComponent* _this, AFortPlayerPawn* PlayerPawn)
{
	AActor* Owner = _this->GetOwner();

	if (!Owner || !PlayerPawn || !_this->CanInteract(PlayerPawn))
		return;

	StopProjectileMovement(_this);
	SetIgnorePawnWhenMoving(_this, _this->PreviousOwningPawn.Get(), false);
	SetCollisionBlockedByPawns(_this, false);
	SetupOwningPawn(_this, PlayerPawn);

	_this->bDroppedFromWeaponSwap = false;

	AttachToOwningPawn(_this);

	Owner->ForceNetUpdate();
	PlayerPawn->ForceNetUpdate();

	_this->OnHeldObjectPickedUp.Broadcast();

	TWeakObjectPtr<UFortHeldObjectComponent> WeakComponent;
	WeakComponent = _this;
	TWeakObjectPtr<AFortPlayerPawn> WeakPawn;
	WeakPawn = PlayerPawn;

	FTimerHandle EquipTimerHandle;
	GWorld->GetTimerManager().SetTimer(EquipTimerHandle, [WeakComponent, WeakPawn]()
		{
			UFortHeldObjectComponent* Component = WeakComponent.Get();
			AFortPlayerPawn* Pawn = WeakPawn.Get();

			if (!Component || !Pawn || Component->HeldObjectState != EHeldObjectState::Held || Component->OwningPawn.Get() != Pawn)
				return;

			AttachToOwningPawn(Component);

			if (!EquipBackingWeapon(Component))
				DropHeldObject(Component);
		}, 0.1f, false);
}

void FortHeldObjectComponent::DropHeldObject(UFortHeldObjectComponent* _this)
{
	AActor* Owner = _this->GetOwner();
	AFortPlayerPawn* OwningPawn = _this->OwningPawn.Get();

	if (!Owner || !OwningPawn || _this->HeldObjectState != EHeldObjectState::Held)
		return;

	FVector DetachLocation = Owner->K2_GetActorLocation();
	ValidateDetachLocation(_this, DetachLocation);

	StopProjectileMovement(_this);
	DetachFromOwningPawn(_this);
	Owner->K2_SetActorLocationAndRotation(DetachLocation, FRotator{ 0.f, OwningPawn->K2_GetActorRotation().Yaw, 0.f }, false, nullptr, true);

	_this->HeldObjectState = EHeldObjectState::Dropped;

	ClearOwningPawn(_this);
	UnequipBackingWeapon(_this, OwningPawn);
	SetIgnorePawnWhenMoving(_this, OwningPawn, true);
	SetCollisionBlockedByPawns(_this, true);
	StartProjectileMovement(_this, FVector{});

	_this->OnHeldObjectDropped.Broadcast();
}

void FortHeldObjectComponent::PlaceHeldObject(UFortHeldObjectComponent* _this)
{
	AActor* Owner = _this->GetOwner();
	AFortPlayerPawn* OwningPawn = _this->OwningPawn.Get();

	if (!Owner || !OwningPawn || _this->HeldObjectState != EHeldObjectState::Held)
		return;

	FHitResult Hit{};
	if (!PerformPlacementTrace(_this, Hit))
		return;

	StopProjectileMovement(_this);
	DetachFromOwningPawn(_this);
	Owner->K2_SetActorLocationAndRotation(FVector{ Hit.Location.X, Hit.Location.Y, Hit.Location.Z }, FRotator{ 0.f, OwningPawn->K2_GetActorRotation().Yaw, 0.f }, false, nullptr, true);

	_this->HeldObjectState = EHeldObjectState::Placed;

	ClearOwningPawn(_this);
	UnequipBackingWeapon(_this, OwningPawn);
	SetCollisionBlockedByPawns(_this, false);

	_this->OnHeldObjectPlaced.Broadcast();
}

void FortHeldObjectComponent::ThrowHeldObject(UFortHeldObjectComponent* _this, const FVector& DetachLocation, const FRotator& ThrowDirection)
{
	AActor* Owner = _this->GetOwner();
	AFortPlayerPawn* OwningPawn = _this->OwningPawn.Get();

	if (!Owner || !OwningPawn || _this->HeldObjectState != EHeldObjectState::Held)
		return;

	FVector ValidatedDetachLocation = DetachLocation;
	ValidateDetachLocation(_this, ValidatedDetachLocation);

	StopProjectileMovement(_this);
	DetachFromOwningPawn(_this);
	Owner->K2_SetActorLocationAndRotation(ValidatedDetachLocation, FRotator{ 0.f, ThrowDirection.Yaw, 0.f }, false, nullptr, true);

	_this->HeldObjectState = EHeldObjectState::Thrown;

	ClearOwningPawn(_this);
	SetIgnorePawnWhenMoving(_this, OwningPawn, true);
	SetCollisionBlockedByPawns(_this, true);

	const FVector Direction = UKismetMathLibrary::GetForwardVector(ThrowDirection);
	const float Speed = _this->GetMaxSpeed();
	StartProjectileMovement(_this, FVector{ Direction.X * Speed, Direction.Y * Speed, Direction.Z * Speed });

	_this->OnHeldObjectThrown.Broadcast();
}

void FortHeldObjectComponent::ThrownObjectStopped(UFortHeldObjectComponent* _this, const FHitResult& ImpactResult)
{
	const EHeldObjectState PreviousState = _this->HeldObjectState;

	if (PreviousState != EHeldObjectState::Thrown && PreviousState != EHeldObjectState::Dropped)
		return;

	StopProjectileMovement(_this);
	SetIgnorePawnWhenMoving(_this, _this->PreviousOwningPawn.Get(), false);
	SetCollisionBlockedByPawns(_this, false);

	_this->HeldObjectState = EHeldObjectState::Dropped;

	if (_this->GrantedWeaponItem.Get())
		UnequipBackingWeapon(_this, _this->PreviousOwningPawn.Get());

	if (AActor* Owner = _this->GetOwner())
		Owner->ForceNetUpdate();

	if (PreviousState == EHeldObjectState::Thrown)
		_this->OnHeldObjectThrownLanded.Broadcast();
}

void FortHeldObjectComponent::OnThrowComplete(UFortHeldObjectComponent* _this)
{
	UnequipBackingWeapon(_this, _this->PreviousOwningPawn.Get());
}

void FortHeldObjectComponent::OnWeaponUnequipped(UFortHeldObjectComponent* _this)
{
	if (_this->HeldObjectState != EHeldObjectState::Held)
		return;

	AFortPlayerPawn* OwningPawn = _this->OwningPawn.Get();
	AFortWeapon* GrantedWeapon = _this->GrantedWeapon.Get();

	if (!OwningPawn || !GrantedWeapon || OwningPawn->CurrentWeapon == GrantedWeapon)
		return;

	BindWeaponUnequipped(_this, OwningPawn, false);

	TWeakObjectPtr<UFortHeldObjectComponent> WeakComponent;
	WeakComponent = _this;

	GWorld->GetTimerManager().SetTimerForNextTick([WeakComponent]()
		{
			UFortHeldObjectComponent* Component = WeakComponent.Get();
			if (!Component || Component->HeldObjectState != EHeldObjectState::Held)
				return;

			AFortPlayerPawn* Pawn = Component->OwningPawn.Get();
			AFortWeapon* Weapon = Component->GrantedWeapon.Get();
			if (Pawn && Weapon && Pawn->CurrentWeapon == Weapon)
			{
				BindWeaponUnequipped(Component, Pawn, true);
				return;
			}

			Component->bDroppedFromWeaponSwap = true;
			DropHeldObject(Component);
		});
}

void FortHeldObjectComponent::HandleOwnerDestroyed(UFortHeldObjectComponent* _this, AActor* DestroyedActor)
{
	AFortPlayerPawn* OwningPawn = _this->OwningPawn.Get();

	StopProjectileMovement(_this);
	BindWeaponUnequipped(_this, OwningPawn, false);

	if (OwningPawn)
	{
		ClearOwningPawn(_this);
		UnequipBackingWeapon(_this, OwningPawn);
	}

	_this->HeldObjectState = EHeldObjectState::Unheld;
	_this->OnHeldObjectDestroy.Broadcast();
}

void FortHeldObjectComponent::HandleOwnerAsBuildingActorDestroyed(UFortHeldObjectComponent* _this, AActor* DamagedActor)
{
	HandleOwnerDestroyed(_this, DamagedActor);
}

void FortHeldObjectComponent::execPickupHeldObject(UObject* Context, FFrame& Stack, void* const Result)
{
	AFortPlayerPawn* PlayerPawn = Stack.StepCompiledInRef<AFortPlayerPawn*>();
	Stack.IncrementCode();

	PickupHeldObject(static_cast<UFortHeldObjectComponent*>(Context), PlayerPawn);
}

void FortHeldObjectComponent::execDropHeldObject(UObject* Context, FFrame& Stack, void* const Result)
{
	Stack.IncrementCode();

	DropHeldObject(static_cast<UFortHeldObjectComponent*>(Context));
}

void FortHeldObjectComponent::execPlaceHeldObject(UObject* Context, FFrame& Stack, void* const Result)
{
	Stack.IncrementCode();

	PlaceHeldObject(static_cast<UFortHeldObjectComponent*>(Context));
}

void FortHeldObjectComponent::execThrowHeldObject(UObject* Context, FFrame& Stack, void* const Result)
{
	FVector DetachLocation = Stack.StepCompiledInRef<FVector>();
	FRotator ThrowDirection = Stack.StepCompiledInRef<FRotator>();
	Stack.IncrementCode();

	ThrowHeldObject(static_cast<UFortHeldObjectComponent*>(Context), DetachLocation, ThrowDirection);
}

void FortHeldObjectComponent::execThrownObjectStopped(UObject* Context, FFrame& Stack, void* const Result)
{
	FHitResult ImpactResult = Stack.StepCompiledInRef<FHitResult>();
	Stack.IncrementCode();

	ThrownObjectStopped(static_cast<UFortHeldObjectComponent*>(Context), ImpactResult);
}

void FortHeldObjectComponent::execOnThrowComplete(UObject* Context, FFrame& Stack, void* const Result)
{
	Stack.IncrementCode();

	OnThrowComplete(static_cast<UFortHeldObjectComponent*>(Context));
}

void FortHeldObjectComponent::execOnWeaponUnequipped(UObject* Context, FFrame& Stack, void* const Result)
{
	Stack.IncrementCode();

	OnWeaponUnequipped(static_cast<UFortHeldObjectComponent*>(Context));
}

void FortHeldObjectComponent::execHandleOwnerDestroyed(UObject* Context, FFrame& Stack, void* const Result)
{
	AActor* DestroyedActor = Stack.StepCompiledInRef<AActor*>();
	Stack.IncrementCode();

	HandleOwnerDestroyed(static_cast<UFortHeldObjectComponent*>(Context), DestroyedActor);
}

void FortHeldObjectComponent::execHandleOwnerAsBuildingActorDestroyed(UObject* Context, FFrame& Stack, void* const Result)
{
	AActor* DamagedActor = Stack.StepCompiledInRef<AActor*>();
	float Damage = Stack.StepCompiledInRef<float>();
	AController* InstigatedBy = Stack.StepCompiledInRef<AController*>();
	AActor* DamageCauser = Stack.StepCompiledInRef<AActor*>();
	FVector HitLocation = Stack.StepCompiledInRef<FVector>();
	UPrimitiveComponent* HitComponent = Stack.StepCompiledInRef<UPrimitiveComponent*>();
	FName BoneName = Stack.StepCompiledInRef<FName>();
	FVector Momentum = Stack.StepCompiledInRef<FVector>();
	Stack.IncrementCode();

	HandleOwnerAsBuildingActorDestroyed(static_cast<UFortHeldObjectComponent*>(Context), DamagedActor);
}

void FortHeldObjectComponent::Init()
{
	UClass* HeldObjectComponentClass = UFortHeldObjectComponent::StaticClass();

	Hooking::ExecHook(HeldObjectComponentClass->GetFunction("PickupHeldObject"), execPickupHeldObject);
	Hooking::ExecHook(HeldObjectComponentClass->GetFunction("DropHeldObject"), execDropHeldObject);
	Hooking::ExecHook(HeldObjectComponentClass->GetFunction("PlaceHeldObject"), execPlaceHeldObject);
	Hooking::ExecHook(HeldObjectComponentClass->GetFunction("ThrowHeldObject"), execThrowHeldObject);
	Hooking::ExecHook(HeldObjectComponentClass->GetFunction("ThrownObjectStopped"), execThrownObjectStopped);
	Hooking::ExecHook(HeldObjectComponentClass->GetFunction("OnThrowComplete"), execOnThrowComplete);
	Hooking::ExecHook(HeldObjectComponentClass->GetFunction("OnWeaponUnequipped"), execOnWeaponUnequipped);
	Hooking::ExecHook(HeldObjectComponentClass->GetFunction("HandleOwnerDestroyed"), execHandleOwnerDestroyed);
	Hooking::ExecHook(HeldObjectComponentClass->GetFunction("HandleOwnerAsBuildingActorDestroyed"), execHandleOwnerAsBuildingActorDestroyed);
}
