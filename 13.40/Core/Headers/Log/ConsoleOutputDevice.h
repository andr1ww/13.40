#pragma once
#include "pch.h"
#include "LogMacros.h"
#include <string>

class FConsoleOutputDevice final : public FOutputDevice
{
public:
	static FConsoleOutputDevice& Get()
	{
		static FConsoleOutputDevice* Device = new FConsoleOutputDevice();
		return *Device;
	}

	void Show()
	{
		{
			FScopedLock ScopedLock(Lock);

			if (ConsoleHandle)
				return;

			AllocConsole();

			HANDLE Handle = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
			if (Handle == INVALID_HANDLE_VALUE)
				return;

			DWORD Mode = 0;
			GetConsoleMode(Handle, &Mode);
			bVirtualTerminal = SetConsoleMode(Handle, Mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;

			COORD Size = { 160, 4000 };
			SetConsoleScreenBufferSize(Handle, Size);

			ConsoleHandle = Handle;
		}

		GLog->AddOutputDevice(this);
	}

	bool IsShown() const
	{
		return ConsoleHandle != nullptr;
	}

	void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
	{
		Serialize(V, Verbosity, Category, -1.0);
	}

	void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category, const double Time) override
	{
		if (!ConsoleHandle || !V)
			return;

		if (Verbosity == ELogVerbosity::SetColor)
		{
			FScopedLock ScopedLock(Lock);
			OverrideColor = V;
			return;
		}

		FString Formatted = FOutputDeviceHelper::FormatLogLine(Verbosity, Category, V, GPrintLogTimes, Time);
		const TCHAR* Line = Formatted.Num() ? Formatted.CStr() : V;

		FScopedLock ScopedLock(Lock);

		const TCHAR* Color = COLOR_NONE;
		if (!OverrideColor.empty())
			Color = OverrideColor.c_str();
		else if (Verbosity == ELogVerbosity::Fatal || Verbosity == ELogVerbosity::Error)
			Color = COLOR_RED;
		else if (Verbosity == ELogVerbosity::Warning)
			Color = COLOR_YELLOW;

		if (bVirtualTerminal)
		{
			std::wstring Output;
			if (*Color)
				Output += ToEscapeSequence(Color);

			Output += Line;

			if (*Color)
				Output += L"\x1b[0m";

			Output += L"\r\n";
			Write(Output.c_str(), Output.size());
		}
		else
		{
			if (*Color)
				SetConsoleTextAttribute(ConsoleHandle, ToAttributes(Color));

			Write(Line, wcslen(Line));
			Write(L"\r\n", 2);

			if (*Color)
				SetConsoleTextAttribute(ConsoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		}

		Formatted.Free();
	}

	bool CanBeUsedOnAnyThread() const override
	{
		return true;
	}

	bool CanBeUsedOnMultipleThreads() const override
	{
		return true;
	}

private:
	struct FScopedLock
	{
		SRWLOCK& Lock;

		explicit FScopedLock(SRWLOCK& InLock) : Lock(InLock) { AcquireSRWLockExclusive(&Lock); }
		~FScopedLock() { ReleaseSRWLockExclusive(&Lock); }

		FScopedLock(const FScopedLock&) = delete;
		FScopedLock& operator=(const FScopedLock&) = delete;
	};

	FConsoleOutputDevice() = default;

	void Write(const TCHAR* Text, size_t Length)
	{
		DWORD Written = 0;
		WriteConsoleW(ConsoleHandle, Text, (DWORD)Length, &Written, nullptr);
	}

	static void ParseColor(const TCHAR* Color, bool (&Bits)[8])
	{
		for (int Index = 0; Index < 8 && Color[Index]; ++Index)
			Bits[Index] = Color[Index] != L'0';
	}

	static std::wstring ToEscapeSequence(const TCHAR* Color)
	{
		bool Bits[8] = {};
		ParseColor(Color, Bits);

		const int Foreground = (Bits[0] ? 1 : 0) | (Bits[1] ? 2 : 0) | (Bits[2] ? 4 : 0);
		const int Background = (Bits[4] ? 1 : 0) | (Bits[5] ? 2 : 0) | (Bits[6] ? 4 : 0);

		std::wstring Sequence = L"\x1b[" + std::to_wstring((Bits[3] ? 90 : 30) + Foreground);
		if (Background || Bits[7])
			Sequence += L";" + std::to_wstring((Bits[7] ? 100 : 40) + Background);

		return Sequence + L"m";
	}

	static WORD ToAttributes(const TCHAR* Color)
	{
		bool Bits[8] = {};
		ParseColor(Color, Bits);

		return (Bits[0] ? FOREGROUND_RED : 0) |
			(Bits[1] ? FOREGROUND_GREEN : 0) |
			(Bits[2] ? FOREGROUND_BLUE : 0) |
			(Bits[3] ? FOREGROUND_INTENSITY : 0) |
			(Bits[4] ? BACKGROUND_RED : 0) |
			(Bits[5] ? BACKGROUND_GREEN : 0) |
			(Bits[6] ? BACKGROUND_BLUE : 0) |
			(Bits[7] ? BACKGROUND_INTENSITY : 0);
	}

	HANDLE ConsoleHandle = nullptr;
	bool bVirtualTerminal = false;
	std::wstring OverrideColor;
	SRWLOCK Lock = SRWLOCK_INIT;
};
