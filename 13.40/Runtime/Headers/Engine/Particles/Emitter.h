#pragma once
#include "pch.h"

namespace Emitter
{
	DefineHook(void, PostInitializeComponents, AEmitter* _this);
	DefineHook(void, CameraLensEffectPostInitializeComponents, AEmitterCameraLensEffectBase* _this);

	void Init();
}
