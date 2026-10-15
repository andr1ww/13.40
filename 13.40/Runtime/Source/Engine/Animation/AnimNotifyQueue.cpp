#include "pch.h"
#include "Core.h"
#include "Engine/Animation/AnimNotifyQueue.h"

DEFINE_INIT(AnimNotifyQueue)

void AnimNotifyQueue::AddAnimNotifiesToDest(FAnimNotifyQueue* _this, bool bSrcIsLeader, const TArray<FAnimNotifyEventReference>& NewNotifies, TArray<FAnimNotifyEventReference>& DestArray, float InstanceWeight)
{
	TArray<FAnimNotifyEventReference> ServerNotifies{};

	for (int32 i = 0; i < NewNotifies.Num(); i++)
	{
		const FAnimNotifyEvent* Notify = NewNotifies[i].GetNotify();

		if (Notify && Notify->bTriggerOnDedicatedServer)
			ServerNotifies.Add(NewNotifies[i]);
	}

	AddAnimNotifiesToDestOG(_this, bSrcIsLeader, ServerNotifies, DestArray, InstanceWeight);

	ServerNotifies.Free();
}

void AnimNotifyQueue::Init()
{
	Hooking::Hook(ImageBase + 0x477B970, AddAnimNotifiesToDest, AddAnimNotifiesToDestOG);
}
