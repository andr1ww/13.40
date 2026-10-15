#pragma once
#include "pch.h"

namespace SkeletalMeshComponent
{
	DefineHook(void, UpdateEndPhysicsTickRegisteredState, USkeletalMeshComponent* _this);

	void Init();
}
