// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Symbol information associated with a program counter. ANSI version.
 */
struct FProgramCounterSymbolInfo final
{
	enum
	{
		/** Length of the string used to store the symbol's names, including the trailing character. */
		MAX_NAME_LENGTH = 1024,
	};

	/** Module name. */
	ANSICHAR	ModuleName[MAX_NAME_LENGTH];

	/** Function name. */
	ANSICHAR	FunctionName[MAX_NAME_LENGTH];

	/** Filename. */
	ANSICHAR	Filename[MAX_NAME_LENGTH];

	/** Line number in file. */
	int32		LineNumber;

	/** Symbol displacement of address.	*/
	int32		SymbolDisplacement;

	/** Program counter offset into module. */
	uint64		OffsetInModule;

	/** Program counter. */
	uint64		ProgramCounter;

	/** Default constructor. */
	CORE_API FProgramCounterSymbolInfo()
	{
		reinterpret_cast<FProgramCounterSymbolInfo*(*)(FProgramCounterSymbolInfo*)>(ImageBase + 0x31EB110)(this);
	}
};

static_assert(sizeof(FProgramCounterSymbolInfo) == 0xC18, "Wrong size on FProgramCounterSymbolInfo");

/**
 * Generic implementation for most platforms
 **/
struct CORE_API FWindowsPlatformStackWalk
{
	static void ProgramCounterToSymbolInfo( uint64 ProgramCounter, FProgramCounterSymbolInfo& out_SymbolInfo )
	{
		reinterpret_cast<void(*)(uint64, FProgramCounterSymbolInfo&)>(ImageBase + 0x3334370)(ProgramCounter, out_SymbolInfo);
	}
};

typedef FWindowsPlatformStackWalk FPlatformStackWalk;
