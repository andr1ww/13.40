// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 *	Learn about the Stats System at docs.unrealengine.com
 */

struct TStatId;

// used by the profiler
enum EStatType
{
	STATTYPE_CycleCounter,
	STATTYPE_AccumulatorFLOAT,
	STATTYPE_AccumulatorDWORD,
	STATTYPE_CounterFLOAT,
	STATTYPE_CounterDWORD,
	STATTYPE_MemoryCounter,
	STATTYPE_Error
};

/*----------------------------------------------------------------------------
	Stats helpers
----------------------------------------------------------------------------*/

	#define STAT(x)

#define FORCEINLINE_STATS FORCEINLINE

struct TStatId {};

class FScopeCycleCounter
{
public:
	FORCEINLINE_STATS FScopeCycleCounter(TStatId, bool bAlways = false)
	{
	}
};

FORCEINLINE void StatsMasterEnableAdd(int32 Value = 1)
{
}
FORCEINLINE void StatsMasterEnableSubtract(int32 Value = 1)
{
}

#define GHitchDetected (*reinterpret_cast<bool*>(ImageBase + 0x8D2661B))

class FLightweightStatScope
{
	const TCHAR* StatString;
public:
	FORCEINLINE FLightweightStatScope(const TCHAR* InStat)
	{
		StatString = GHitchDetected ? nullptr : InStat;
	}

	FORCEINLINE ~FLightweightStatScope()
	{
		if (GHitchDetected && StatString)
		{
			ReportHitch();
		}
	}

	void ReportHitch()
	{
		reinterpret_cast<void(*)(FLightweightStatScope*)>(ImageBase + 0x3318B40)(this);
	}
};

#define DECLARE_SCOPE_CYCLE_COUNTER(CounterName,Stat,GroupId) \
	FLightweightStatScope LightweightStatScope_##Stat(TEXT(#Stat));

#define QUICK_SCOPE_CYCLE_COUNTER(Stat) \
	FLightweightStatScope LightweightStatScope_##Stat(TEXT(#Stat));

#define SCOPE_CYCLE_COUNTER(Stat) \
	FLightweightStatScope LightweightStatScope_##Stat(TEXT(#Stat));

#define CONDITIONAL_SCOPE_CYCLE_COUNTER(Stat,bCondition) \
	FLightweightStatScope LightweightStatScope_##Stat(bCondition ? TEXT(#Stat) : nullptr);

#define RETURN_QUICK_DECLARE_CYCLE_STAT(StatId,GroupId) return TStatId();
#define GET_STATID(Stat) (TStatId())

#define SCOPE_SECONDS_ACCUMULATOR(Stat)
#define SCOPE_MS_ACCUMULATOR(Stat)
#define DEFINE_STAT(Stat)
#define QUICK_USE_CYCLE_STAT(StatId,GroupId) TStatId()
#define DECLARE_CYCLE_STAT(CounterName,StatId,GroupId)
#define DECLARE_FLOAT_COUNTER_STAT(CounterName,StatId,GroupId)
#define DECLARE_DWORD_COUNTER_STAT(CounterName,StatId,GroupId)
#define DECLARE_FLOAT_ACCUMULATOR_STAT(CounterName,StatId,GroupId)
#define DECLARE_DWORD_ACCUMULATOR_STAT(CounterName,StatId,GroupId)
#define DECLARE_FNAME_STAT(CounterName,StatId,GroupId, API)
#define DECLARE_PTR_STAT(CounterName,StatId,GroupId)
#define DECLARE_MEMORY_STAT(CounterName,StatId,GroupId)
#define DECLARE_MEMORY_STAT_POOL(CounterName,StatId,GroupId,Pool)
#define DECLARE_CYCLE_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_FLOAT_COUNTER_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_DWORD_COUNTER_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_FLOAT_ACCUMULATOR_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_FNAME_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_PTR_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_MEMORY_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_MEMORY_STAT_POOL_EXTERN(CounterName,StatId,GroupId,Pool, API)

#define DECLARE_STATS_GROUP(GroupDesc,GroupId,GroupCat)
#define DECLARE_STATS_GROUP_VERBOSE(GroupDesc,GroupId,GroupCat)
#define DECLARE_STATS_GROUP_MAYBE_COMPILED_OUT(GroupDesc,GroupId,GroupCat,CompileIn)

#define SET_CYCLE_COUNTER(Stat,Cycles)
#define INC_DWORD_STAT(StatId)
#define INC_FLOAT_STAT_BY(StatId,Amount)
#define INC_DWORD_STAT_BY(StatId,Amount)
#define INC_DWORD_STAT_FNAME_BY(StatId,Amount)
#define INC_MEMORY_STAT_BY(StatId,Amount)
#define DEC_DWORD_STAT(StatId)
#define DEC_FLOAT_STAT_BY(StatId,Amount)
#define DEC_DWORD_STAT_BY(StatId,Amount)
#define DEC_DWORD_STAT_FNAME_BY(StatId,Amount)
#define DEC_MEMORY_STAT_BY(StatId,Amount)
#define SET_MEMORY_STAT(StatId,Value)
#define SET_DWORD_STAT(StatId,Value)
#define SET_FLOAT_STAT(StatId,Value)
#define STAT_ADD_CUSTOMMESSAGE_NAME(StatId,Value)
#define STAT_ADD_CUSTOMMESSAGE_PTR(StatId,Value)

#define SET_CYCLE_COUNTER_FName(Stat,Cycles)
#define INC_DWORD_STAT_FName(Stat)
#define INC_FLOAT_STAT_BY_FName(Stat, Amount)
#define INC_DWORD_STAT_BY_FName(Stat, Amount)
#define INC_MEMORY_STAT_BY_FName(Stat, Amount)
#define DEC_DWORD_STAT_FName(Stat)
