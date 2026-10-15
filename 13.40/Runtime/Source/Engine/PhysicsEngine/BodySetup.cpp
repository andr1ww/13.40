#include "pch.h"
#include "Core.h"
#include "Engine/PhysicsEngine/BodySetup.h"

DEFINE_INIT(BodySetup)

void* BodySetup::GetCookedData(UBodySetup* _this, FName Format, bool bRuntimeOnlyOptimizedVersion)
{
	if (void* CookedData = GetCookedDataOG(_this, Format, bRuntimeOnlyOptimizedVersion))
		return CookedData;

	return GetCookedDataOG(_this, FName(L"PhysXGeneric"), bRuntimeOnlyOptimizedVersion);
}

void BodySetup::Init()
{
	Hooking::Rel32Swap(ImageBase + 0x4BED624, GetCookedData, GetCookedDataOG);
}
