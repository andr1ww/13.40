#include "pch.h"
#include "Core.h"
#include "Engine/Components/SkeletalMeshComponent.h"

DEFINE_INIT(SkeletalMeshComponent)

void SkeletalMeshComponent::UpdateEndPhysicsTickRegisteredState(USkeletalMeshComponent* _this)
{
	if (!_this->bEnablePhysicsOnDedicatedServer)
		return _this->RegisterEndPhysicsTick(false);

	UpdateEndPhysicsTickRegisteredStateOG(_this);
}

void SkeletalMeshComponent::Init()
{
	Hooking::Hook(ImageBase + 0x48D43C0, UpdateEndPhysicsTickRegisteredState, UpdateEndPhysicsTickRegisteredStateOG);
	Hooking::RetFalse(ImageBase + 0x48D0AD0);
}
