#pragma once
#include "pch.h"
#include "Core.h"

namespace ParticleComponents
{
	DefineHook(void, TickComponent, UParticleSystemComponent* _this, float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);

	void Init();
}
