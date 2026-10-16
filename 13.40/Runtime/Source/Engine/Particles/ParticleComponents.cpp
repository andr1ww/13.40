#include "pch.h"
#include "Core.h"
#include "Engine/Particles/ParticleComponents.h"

DEFINE_INIT(ParticleComponents)

void ParticleComponents::TickComponent(UParticleSystemComponent* _this, float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (!_this->bUpdateOnDedicatedServer && _this->Template && _this->Template->Emitters.Num() > 0 && (_this->bIsActive || _this->bAutoActivate))
	{
		if (_this->IsDeactivateTriggered())
			_this->DeactivateSystem();

		if (_this->IsAutoDestroy())
			_this->DestroyComponent();
		else
			_this->SetComponentTickEnabled(false);

		return;
	}

	TickComponentOG(_this, DeltaTime, TickType, ThisTickFunction);
}

void ParticleComponents::Init()
{
	Hooking::Hook(ImageBase + 0x4B50C00, TickComponent, TickComponentOG);
}
