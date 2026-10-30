// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

static inline void FTimerDynamicDelegate_DelegateWrapper(const FScriptDelegate& TimerDynamicDelegate)
{
	TimerDynamicDelegate.ProcessDelegate<UObject>(NULL);
}

class FTimerDynamicDelegate : public TDelegate<void()>
{
public:
	/** Default constructor */
	FTimerDynamicDelegate()
	{
	}

	/** Construction from an FScriptDelegate must be explicit.  This is really only used by UObject system internals. */
	explicit FTimerDynamicDelegate( const FScriptDelegate& InScriptDelegate )
		: TDelegate<void()>( InScriptDelegate )
	{
	}

	/** Execute the delegate.  If the function pointer is not valid, an error will occur. */
	inline void Execute(  ) const
	{
		/* Verify that the user object is still valid.  We only have a weak reference to it. */
		checkSlow( IsBound() );
		FTimerDynamicDelegate_DelegateWrapper( *this );
	}
	/** Execute the delegate, but only if the function pointer is still valid */
	inline bool ExecuteIfBound(  ) const
	{
		if( IsBound() )
		{
			FTimerDynamicDelegate_DelegateWrapper( *this );
			return true;
		}
		return false;
	}
};
