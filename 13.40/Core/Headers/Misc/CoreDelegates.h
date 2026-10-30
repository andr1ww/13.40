// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class CORE_API FCoreDelegates
{
public:
	// Callback for handling an error
	typedef TMulticastDelegate<void()> FOnHandleSystemError;

	// Callback when an error (crash) has occurred
	static inline FOnHandleSystemError OnHandleSystemError;
};
