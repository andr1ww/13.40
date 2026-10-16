#include "pch.h"
#include "Core.h"
#include "Athena/FortSafeZoneIndicator.h"
DEFINE_ENGINE_INIT(FortSafeZoneIndicator)

void FortSafeZoneIndicator::SetShrinkDuration(AFortSafeZoneIndicator* _this, float WaitTime)
{
	_this->SafeZoneStartShrinkTime = UWorld::GetWorld()->GameState->GetServerWorldTimeSeconds() + WaitTime;
}

void FortSafeZoneIndicator::SetFinishShrinkDuration(AFortSafeZoneIndicator* _this, float ShrinkTime)
{
	_this->SafeZoneFinishShrinkTime = _this->SafeZoneStartShrinkTime + ShrinkTime;
}

void FortSafeZoneIndicator::Init()
{
	Hooking::Rel32Swap(ImageBase + 0x1ECE2C8, SetShrinkDuration);
	Hooking::Rel32Swap(ImageBase + 0x1ECE740, SetFinishShrinkDuration);
}
