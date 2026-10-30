#pragma once
#include "pch.h"

namespace World
{
	bool Listen(UWorld* _this, FURL& InURL);

	void Init();
}

/**
 * A helper RAII class to set the relevant context on a UWorld for a particular FLevelCollection within a scope.
 * The constructor will set the PersistentLevel, GameState, NetDriver, and DemoNetDriver on the world and
 * the destructor will restore the original values.
 */
class FScopedLevelCollectionContextSwitch
{
public:
	/**
	 * Constructor that will save the current relevant values of InWorld
	 * and set the collection's context values for InWorld.
	 *
	 * @param InLevelCollectionIndex The index of the collection to use
	 * @param InWorld The world on which to set the context.
	 */
	FScopedLevelCollectionContextSwitch(int32 InLevelCollectionIndex, UWorld* const InWorld)
	{
		reinterpret_cast<FScopedLevelCollectionContextSwitch*(*)(FScopedLevelCollectionContextSwitch*, int32, UWorld*)>(ImageBase + 0x4E06000)(this, InLevelCollectionIndex, InWorld);
	}

	/** The destructor restores the context on the world that was saved in the constructor. */
	~FScopedLevelCollectionContextSwitch()
	{
		reinterpret_cast<void(*)(FScopedLevelCollectionContextSwitch*)>(ImageBase + 0x4E07470)(this);
	}

private:
	class UWorld* World;
	int32 SavedTickingCollectionIndex;
};
