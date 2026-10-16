#include "pch.h"
#include "Core.h"
#include "Engine/Components/StaticMeshComponent.h"

DEFINE_INIT(StaticMeshComponent)

bool StaticMeshComponent::IsNavigationRelevant(UStaticMeshComponent* _this)
{
	return _this->StaticMesh && _this->IsNavigationRelevant();
}

void StaticMeshComponent::Init()
{
	Hooking::Hook(ImageBase + 0x48F4CF0, IsNavigationRelevant);
}
