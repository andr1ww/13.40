#pragma once
#include "pch.h"
#include "Core.h"

namespace SkeletalMeshComponentPhysics
{
	DefineHook(void, SetSimulatePhysics, USkeletalMeshComponent* _this, bool bSimulate);
	DefineHook(void, SetEnableGravity, USkeletalMeshComponent* _this, bool bGravityEnabled);
	DefineHook(void, ResetAllBodiesSimulatePhysics, USkeletalMeshComponent* _this);
	DefineHook(void, EndPhysicsTick, FSkeletalMeshComponentEndPhysicsTickFunction* _this, float DeltaTime, ELevelTick TickType, int32 CurrentThread, void* MyCompletionGraphEvent);

	void Init();
}
