#pragma once
#include "pch.h"

template <typename T>
inline T* NewObject(UObject* Outer)
{
	return static_cast<T*>(UGameplayStatics::SpawnObject(T::StaticClass(), Outer));
}

inline UObject* StaticLoadObject(UClass* Class, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename = nullptr, uint32 LoadFlags = 0, UObject* Sandbox = nullptr, bool bAllowObjectReconciliation = true, void* InstancingContext = nullptr)
{
	return reinterpret_cast<UObject* (*)(UClass*, UObject*, const TCHAR*, const TCHAR*, uint32, UObject*, bool, void*)>(ImageBase + 0x3530060)(Class, InOuter, Name, Filename, LoadFlags, Sandbox, bAllowObjectReconciliation, InstancingContext);
}

template <typename T>
inline T* StaticLoadObject(const TCHAR* Path, UClass* InClass = T::StaticClass(), UObject* Outer = nullptr)
{
	return static_cast<T*>(StaticLoadObject(InClass, Outer, Path));
}

template <typename T = AActor>
inline TArray<T*> GetAll(UClass* Class = T::StaticClass())
{
	TArray<AActor*> Actors;

	if (Class)
		UGameplayStatics::GetAllActorsOfClass(GWorld, Class, &Actors);

	return *reinterpret_cast<TArray<T*>*>(&Actors);
}
