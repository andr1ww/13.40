#include "pch.h"
#include "Core.h"
#include "Engine/SkeletalMeshComponentPhysics.h"

DEFINE_INIT(SkeletalMeshComponentPhysics)

void SkeletalMeshComponentPhysics::SetSimulatePhysics(USkeletalMeshComponent* _this, bool bSimulate)
{
	if (_this->bEnablePhysicsOnDedicatedServer)
		SetSimulatePhysicsOG(_this, bSimulate);
}

void SkeletalMeshComponentPhysics::SetEnableGravity(USkeletalMeshComponent* _this, bool bGravityEnabled)
{
	if (_this->bEnablePhysicsOnDedicatedServer)
		SetEnableGravityOG(_this, bGravityEnabled);
}

void SkeletalMeshComponentPhysics::ResetAllBodiesSimulatePhysics(USkeletalMeshComponent* _this)
{
	if (_this->bEnablePhysicsOnDedicatedServer)
		ResetAllBodiesSimulatePhysicsOG(_this);
}

void SkeletalMeshComponentPhysics::EndPhysicsTick(FSkeletalMeshComponentEndPhysicsTickFunction* _this, float DeltaTime, ELevelTick TickType, int32 CurrentThread, void* MyCompletionGraphEvent)
{
	USkeletalMeshComponent* Target = _this->GetTarget();
	if (!Target || Target->bEnablePhysicsOnDedicatedServer)
		return EndPhysicsTickOG(_this, DeltaTime, TickType, CurrentThread, MyCompletionGraphEvent);

	if (!Target->IsPendingKill() && Target->IsRegistered() && TickType != LEVELTICK_ViewportsOnly)
		Target->FinalizeBoneTransform();
}

void SkeletalMeshComponentPhysics::Init()
{
	Hooking::Hook(ImageBase + 0x4CE3E00, SetSimulatePhysics, SetSimulatePhysicsOG);
	Hooking::Hook(ImageBase + 0x4CE2DC0, SetEnableGravity, SetEnableGravityOG);
	Hooking::Hook(ImageBase + 0x4CE0FB0, ResetAllBodiesSimulatePhysics, ResetAllBodiesSimulatePhysicsOG);
	Hooking::Hook(ImageBase + 0x4CD3500, EndPhysicsTick, EndPhysicsTickOG);
}
