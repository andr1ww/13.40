// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"
#include <string>
#include <functional>
#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <utility>
#include <memory>
#include <cmath>
#include <thread>
#include <format>

#include "memcury.h"
#include "SDK/SDK/Basic.hpp"
#include "SDK/SDK/CoreUObject_structs.hpp"
#include "SDK/SDK/CoreUObject_classes.hpp"
#include "SDK/SDK/Engine_structs.hpp"
#include "SDK/SDK/Engine_classes.hpp"
#include "SDK/SDK/FortniteGame_structs.hpp"
#include "SDK/SDK/FortniteGame_classes.hpp"
#include "SDK/SDK/GameplayAbilities_structs.hpp"
#include "SDK/SDK/GameplayAbilities_classes.hpp"
#include "SDK/SDK/PlayerPawn_Athena_classes.hpp"
#include "SDK/SDK/GAB_AthenaDBNO_classes.hpp"
#include "SDK/SDK/McpProfileSys_classes.hpp"
#include "SDK/SDK/EngineSettings_Classes.hpp"
#include "SDK/SDK/GameplayTags_classes.hpp"
#include "SDK/SDK/UACBase_classes.hpp"
#include "SDK/SDK/OnlineSubsystem_structs.hpp"

using namespace SDK;
using namespace UC;


using std::cout;
using std::cin;
using std::endl;
using std::vector;
using std::string;
using std::wstring;
using std::u16string;
using std::u32string;
using std::map;
using std::unordered_map;
using std::pair;
using std::make_pair;
using std::unique_ptr;
using std::shared_ptr;
using std::make_unique;
using std::make_shared;
using std::move;
using std::swap;
using std::sort;
using std::find;
using std::max;
using std::min;
using std::function;
using std::bind;
using std::ref;
using std::thread;

template <typename T>
__forceinline T& Get(void* Obj, uint32_t Off) { return *(T*)((uintptr_t)Obj + Off); }

template <typename T>
__forceinline T& Get(const void* Obj, uint32_t Off) { return *(T*)((uintptr_t)Obj + Off); }

template <typename T>
__forceinline T* Get(const void* Obj, uint32_t Off) { return (T*)((uintptr_t)Obj + Off); }

template <typename T>
__forceinline T& Set(const void* Obj, uint32_t Off, const T& Val) { return *(T*)((uintptr_t)Obj + Off) = Val; }

template <typename T>
__forceinline T& Set(void* Obj, uint32_t Off, const T& Val) { return *(T*)((uintptr_t)Obj + Off) = Val; }

template <typename T>
__forceinline T* Set(const void* Obj, uint32_t Off, const T* Val) { return (T*)((uintptr_t)Obj + Off) = Val; }

template <typename T>
__forceinline T* Set(const void* Obj, uint32_t Off, const T Val) { return (T*)((uintptr_t)Obj + Off) = Val; }

template <typename T>
__forceinline T* Set(void* Obj, uint32_t Off, const T* Val) { return (T*)((uintptr_t)Obj + Off) = Val; }

template <typename T>
__forceinline T* Set(void* Obj, uint32_t Off, const T Val) { return (T*)((uintptr_t)Obj + Off) = Val; }

__forceinline void* GetPtr(const void* Obj, uint32_t Off) { return (void*)((uintptr_t)Obj + Off); }

inline std::vector<std::function<void()>>& Initters()
{
	static std::vector<std::function<void()>> Initters;
	return Initters;
}

inline std::vector<std::function<void()>>& EngineInitters()
{
	static std::vector<std::function<void()>> Initters;
	return Initters;
}

#define DEFINE_INIT(Name) \
    namespace { \
        struct Name##_Reg { \
            Name##_Reg() { \
                Initters().push_back(Name::Init); \
            } \
        }; \
        static Name##_Reg erm_##Name##_reg; \
    }

#define DEFINE_ENGINE_INIT(Name) \
    namespace { \
        struct Name##_Reg { \
            Name##_Reg() { \
                EngineInitters().push_back(Name::Init); \
            } \
        }; \
        static Name##_Reg erm_##Name##_reg; \
    }

#endif //PCH_H
