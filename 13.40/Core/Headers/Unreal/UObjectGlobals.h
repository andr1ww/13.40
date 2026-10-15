#pragma once
#include "pch.h"

template <typename T>
inline T* NewObject(UObject* Outer)
{
	return static_cast<T*>(UGameplayStatics::SpawnObject(T::StaticClass(), Outer));
}
