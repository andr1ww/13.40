// Copyright Epic Games, Inc. All Rights Reserved.

/**
*
* A lightweight multi-threaded CSV profiler which can be used for profiling in Test/Shipping builds
*/

#pragma once

#include "CoreMinimal.h"

#define TRACE_CSV_PROFILER_INLINE_STAT_EXCLUSIVE(...)

#define CSV_SCOPED_TIMING_STAT_EXCLUSIVE(StatName) \
	TRACE_CSV_PROFILER_INLINE_STAT_EXCLUSIVE(#StatName); \
	FScopedCsvStatExclusive _ScopedCsvStatExclusive_ ## StatName (#StatName);

#define CSV_DECLARE_CATEGORY_MODULE_EXTERN(Module_API,CategoryName)			extern Module_API FCsvCategory _GCsvCategory_##CategoryName

struct FCsvCategory;

/**
* FCsvProfiler class. This manages the CSV profiling session
*/
class FCsvProfiler
{
public:
	CORE_API static void BeginExclusiveStat(const char * StatName)
	{
		reinterpret_cast<void(*)(const char*)>(ImageBase + 0x32F2A40)(StatName);
	}

	CORE_API static void EndExclusiveStat(const char * StatName)
	{
		reinterpret_cast<void(*)(const char*)>(ImageBase + 0x32F79D0)(StatName);
	}
};

class FScopedCsvStatExclusive
{
public:
	FScopedCsvStatExclusive(const char * InStatName)
		: StatName(InStatName)
	{
		FCsvProfiler::BeginExclusiveStat(StatName);
	}

	~FScopedCsvStatExclusive()
	{
		FCsvProfiler::EndExclusiveStat(StatName);
	}
	const char * StatName;
};
