#pragma once
#include "pch.h"

#include "Hooking/Hooking.hpp"
#include <sstream>
#include "Log/LogMacros.h"
#include "Log/ConsoleOutputDevice.h"
#include "Unreal/UnrealNames.h"

#define GWorld (*reinterpret_cast<UWorld**>(ImageBase + 0x8E94260))
#define GEngine (static_cast<UFortEngine*>(UEngine::GetEngine()))
