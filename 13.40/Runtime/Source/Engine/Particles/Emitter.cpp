#include "pch.h"
#include "Core.h"
#include "Engine/Particles/Emitter.h"

DEFINE_INIT(Emitter)

void Emitter::PostInitializeComponents(AEmitter* _this)
{
	PostInitializeComponentsOG(_this);

	if (_this->RemoteRole == ENetRole::ROLE_None || _this->bNetTemporary)
		_this->SetLifeSpan(0.2f);
}

void Emitter::CameraLensEffectPostInitializeComponents(AEmitterCameraLensEffectBase* _this)
{
	CameraLensEffectPostInitializeComponentsOG(_this);

	if (_this->RemoteRole == ENetRole::ROLE_None || _this->bNetTemporary)
		_this->SetLifeSpan(0.2f);
}

void Emitter::Init()
{
	Hooking::Hook(ImageBase + 0x4B47200, PostInitializeComponents, PostInitializeComponentsOG);
	Hooking::Hook(ImageBase + 0x4B472E0, CameraLensEffectPostInitializeComponents, CameraLensEffectPostInitializeComponentsOG);
}
