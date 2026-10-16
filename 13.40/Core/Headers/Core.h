#pragma once
#include "pch.h"

#include "Hooking/Hooking.hpp"

#define GWorld (*reinterpret_cast<UWorld**>(ImageBase + 0x8E94260))
#define GEngine (static_cast<UFortEngine*>(UEngine::GetEngine()))
#define GTimeBetweenPurgingPendingKillObjectsOnIdleServerMultiplier (*reinterpret_cast<float*>(ImageBase + 0x879101C))
#define GSecondsPerCycle (*reinterpret_cast<double*>(ImageBase + 0x8CEC248))

#include <sstream>
#include "Log/LogMacros.h"
#include "Log/ConsoleOutputDevice.h"
#include "Unreal/UnrealNames.h"
#include "Unreal/UObjectGlobals.h"
#include "Unreal/Stack.h"

struct FPlatformTime
{
	static double Seconds()
	{
		LARGE_INTEGER Cycles;
		QueryPerformanceCounter(&Cycles);
		return Cycles.QuadPart * GSecondsPerCycle + 16777216.0;
	}
};

enum ELevelTick : int32
{
	LEVELTICK_TimeOnly,
	LEVELTICK_ViewportsOnly,
	LEVELTICK_All,
	LEVELTICK_PauseTick
};

inline FGuid NewGuid()
{
	return UKismetGuidLibrary::NewGuid();
}

template <typename T>
inline T* GetInterfaceAddress(UObject* Object)
{
	if (!Object)
		return nullptr;

	return reinterpret_cast<T*(*)(UObject*, UClass*)>(ImageBase + 0x351FAE0)(Object, T::StaticClass());
}

template <typename InterfaceType>
inline InterfaceType* GetInterface(UObject* Object)
{
	if (!Object || !UKismetSystemLibrary::DoesImplementInterface(Object, InterfaceType::StaticClass()))
		return nullptr;

	return reinterpret_cast<InterfaceType*>(Object);
}

class UFortLootLevel
{
public:
	static int32 GetItemLevel(const FDataTableCategoryHandle* LootLevelData, int32 WorldLevel)
	{
		return reinterpret_cast<int32(*)(const FDataTableCategoryHandle*, int32)>(ImageBase + 0x26214C0)(LootLevelData, WorldLevel);
	}
};

inline UFortPlayerAttrSet* GetPlayerAttrSet(UAbilitySystemComponent* AbilitySystemComponent)
{
	if (!AbilitySystemComponent)
		return nullptr;

	for (int32 i = 0; i < AbilitySystemComponent->SpawnedAttributes.Num(); i++)
	{
		if (UFortPlayerAttrSet* AttrSet = Cast<UFortPlayerAttrSet>(AbilitySystemComponent->SpawnedAttributes[i]))
			return AttrSet;
	}

	return nullptr;
}

struct FReservedRandomValues
{
	TArray<float> RandomValues;
	int32 CurrentIndex;
	int32 NumReservedRandomValues;

	float GetNextRandomValue()
	{
		if (NumReservedRandomValues <= 0)
			return 0.0f;

		if (CurrentIndex >= NumReservedRandomValues)
			CurrentIndex = 0;

		return RandomValues[CurrentIndex++];
	}
};

inline float FRand(FReservedRandomValues* ReservedRandomValues)
{
	if (ReservedRandomValues && ReservedRandomValues->NumReservedRandomValues > 0)
		return ReservedRandomValues->GetNextRandomValue();

	return (float)rand() * (1.0f / (float)RAND_MAX);
}

template <typename RowType, typename FuncType>
inline void ForEachRow(UDataTable* Table, FuncType&& Fn)
{
	if (!Table || !Table->RowStruct)
		return;

	for (auto& Pair : Table->RowMap)
	{
		if (Pair.Value())
			Fn(Pair.Key(), reinterpret_cast<RowType*>(Pair.Value()));
	}
}

template <typename RowType>
inline RowType* FindRow(UDataTable* Table, FName RowName)
{
	RowType* Result = nullptr;

	ForEachRow<RowType>(Table, [&](FName Key, RowType* Row)
		{
			if (!Result && Key == RowName)
				Result = Row;
		});

	return Result;
}

inline int32 GetWeaponLoadedAmmo(UFortWeaponItemDefinition* WeaponDefinition, int32 ItemLevel)
{
	if (!WeaponDefinition)
		return 0;

	FFortBaseWeaponStats* Row = FindRow<FFortBaseWeaponStats>(WeaponDefinition->WeaponStatHandle.DataTable, WeaponDefinition->WeaponStatHandle.RowName);

	if (!Row)
		return 0;

	return Row->GetClipSize((float)(ItemLevel < 0 ? 0 : ItemLevel));
}

inline FGameplayTagContainer GetOwnedGameplayTags(UObject* Object)
{
	FGameplayTagContainer Tags{};

	if (IGameplayTagAssetInterface* Interface = GetInterfaceAddress<IGameplayTagAssetInterface>(Object))
		(*reinterpret_cast<void(***)(IGameplayTagAssetInterface*, FGameplayTagContainer*)>(Interface))[2](Interface, &Tags);

	return Tags;
}

inline void FreeGameplayTags(FGameplayTagContainer& Tags)
{
	Tags.GameplayTags.Free();
	Tags.ParentTags.Free();
}
