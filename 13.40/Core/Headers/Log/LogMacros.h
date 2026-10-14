#pragma once
#include "pch.h"
#include <atomic>
#include <type_traits>

typedef char ANSICHAR;

#ifndef FORCEINLINE
	#define FORCEINLINE __forceinline
#endif

#ifndef FORCENOINLINE
	#define FORCENOINLINE __declspec(noinline)
#endif

#define PREPROCESSOR_NOTHING
#define PREPROCESSOR_JOIN(x, y) PREPROCESSOR_JOIN_INNER(x, y)
#define PREPROCESSOR_JOIN_INNER(x, y) x##y

#define CA_ASSUME(Expr)
#define CA_CONSTANT_IF(Condition) if (Condition)

#define TRACE_LOG_MESSAGE(...)

#define UE_DEBUG_BREAK() ((void)(::IsDebuggerPresent() && ([] () { __debugbreak(); } (), 1)))
#define _DebugBreakAndPromptForRemote() UE_DEBUG_BREAK();

#define UE_LOG_TEXT(Format) ([]() -> const TCHAR* { static constexpr ::UE4Logging_Private::TLogFormat UE_LOG_Converted(Format); return UE_LOG_Converted.Data; }())

namespace UE4Logging_Private
{
	template <typename FuncType>
	FORCEINLINE FuncType GameFunction(uintptr_t Offset)
	{
		return reinterpret_cast<FuncType>(ImageBase + Offset);
	}

	inline bool IsProcessShuttingDown()
	{
		static const auto RtlDllShutdownInProgress = reinterpret_cast<BOOLEAN(NTAPI*)()>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlDllShutdownInProgress"));
		return RtlDllShutdownInProgress && RtlDllShutdownInProgress();
	}

	template <typename CharType, size_t N>
	struct TLogFormat
	{
		TCHAR Data[N] = {};

		consteval TLogFormat(const CharType (&Source)[N])
		{
			if constexpr (sizeof(CharType) == sizeof(TCHAR))
			{
				for (size_t Index = 0; Index < N; ++Index)
					Data[Index] = (TCHAR)Source[Index];
			}
			else
			{
				size_t Out = 0;
				size_t In = 0;
				while (In < N && Source[In])
				{
					const uint32 Lead = (uint8)Source[In++];
					const uint32 Extra = Lead >= 0xF0 ? 3 : Lead >= 0xE0 ? 2 : Lead >= 0xC0 ? 1 : 0;
					uint32 CodePoint = Extra ? (Lead & (0x3F >> Extra)) : Lead;

					for (uint32 Index = 0; Index < Extra && In < N && Source[In]; ++Index)
						CodePoint = (CodePoint << 6) | ((uint8)Source[In++] & 0x3F);

					if (CodePoint >= 0x10000)
					{
						CodePoint -= 0x10000;
						Data[Out++] = (TCHAR)(0xD800 + (CodePoint >> 10));
						Data[Out++] = (TCHAR)(0xDC00 + (CodePoint & 0x3FF));
					}
					else
					{
						Data[Out++] = (TCHAR)CodePoint;
					}
				}
				Data[Out] = 0;
			}
		}
	};

	struct FScopedString
	{
		FString Value;

		FScopedString(FString InValue) : Value(InValue) {}
		FScopedString(const FScopedString&) = delete;
		FScopedString& operator=(const FScopedString&) = delete;
		~FScopedString() { Value.Free(); }

		const TCHAR* operator*() const { return Value.Num() ? Value.CStr() : TEXT(""); }
	};
}

namespace ELogVerbosity
{
	enum Type : uint8
	{
		NoLogging		= 0,
		Fatal,
		Error,
		Warning,
		Display,
		Log,
		Verbose,
		VeryVerbose,

		All				= VeryVerbose,
		NumVerbosity,
		VerbosityMask	= 0xf,
		SetColor		= 0x40,
		BreakOnLog		= 0x80
	};
}

#define GPrintLogTimes (*reinterpret_cast<ELogTimes*>(ImageBase + 0x8D26644))

class FOutputDevice
{
public:
	FOutputDevice() = default;
	virtual ~FOutputDevice() = default;

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) = 0;
	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category, const double Time)
	{
		Serialize(V, Verbosity, Category);
	}

	virtual void Flush() {}
	virtual void TearDown() {}
	virtual void Dump(void* Ar) {}
	virtual bool IsMemoryOnly() const { return false; }
	virtual bool CanBeUsedOnAnyThread() const { return false; }
	virtual bool CanBeUsedOnMultipleThreads() const { return false; }

	bool bSuppressEventTag = false;
	bool bAutoEmitLineTerminator = true;
};

struct FOutputDeviceHelper
{
	static FString FormatLogLine(ELogVerbosity::Type Verbosity, const FName& Category, const TCHAR* Message = nullptr, ELogTimes LogTime = ELogTimes::None, const double Time = -1.0)
	{
		FString Result;
		UE4Logging_Private::GameFunction<FString*(*)(FString*, ELogVerbosity::Type, const FName&, const TCHAR*, int32, double)>(0x32D3B00)(&Result, Verbosity, Category, Message, static_cast<int32>(LogTime), Time);
		return Result;
	}
};

class FOutputDeviceConsole : public FOutputDevice
{
public:
	void Show(bool ShowWindow)
	{
		InSDKUtils::GetVirtualFunction<void(*)(FOutputDeviceConsole*, bool)>(this, 0x9)(this, ShowWindow);
	}

	bool IsShown()
	{
		return InSDKUtils::GetVirtualFunction<bool(*)(FOutputDeviceConsole*)>(this, 0xA)(this);
	}

	bool IsAttached()
	{
		return InSDKUtils::GetVirtualFunction<bool(*)(FOutputDeviceConsole*)>(this, 0xB)(this);
	}
};

#define GLogConsole (*reinterpret_cast<FOutputDeviceConsole**>(ImageBase + 0x8D1C5D0))

class FOutputDeviceRedirector : public FOutputDevice
{
public:
	void AddOutputDevice(FOutputDevice* OutputDevice)
	{
		UE4Logging_Private::GameFunction<void(*)(FOutputDeviceRedirector*, FOutputDevice*)>(0x32C9860)(this, OutputDevice);
	}

	void RemoveOutputDevice(FOutputDevice* OutputDevice)
	{
		UE4Logging_Private::GameFunction<void(*)(FOutputDeviceRedirector*, FOutputDevice*)>(0x32E4CB0)(this, OutputDevice);
	}

	void FlushThreadedLogs()
	{
		UE4Logging_Private::GameFunction<void(*)(FOutputDeviceRedirector*)>(0x32D3760)(this);
	}
};

inline FOutputDeviceRedirector* GetGlobalLogSingleton()
{
	return UE4Logging_Private::GameFunction<FOutputDeviceRedirector*(*)()>(0x32D7CF0)();
}

#define GLog GetGlobalLogSingleton()

struct FLogCategoryBase
{
	FORCEINLINE bool IsSuppressed(ELogVerbosity::Type VerbosityLevel) const
	{
		if ((VerbosityLevel & ELogVerbosity::VerbosityMask) <= Verbosity)
		{
			return false;
		}
		return true;
	}

	void PostTrigger(ELogVerbosity::Type VerbosityLevel)
	{
		if (DebugBreakOnLog || (VerbosityLevel & ELogVerbosity::BreakOnLog))
		{
			GLog->FlushThreadedLogs();
			DebugBreakOnLog = false;
			UE_DEBUG_BREAK();
		}
	}

	FORCEINLINE FName GetCategoryName() const
	{
		return CategoryFName;
	}

	ELogVerbosity::Type GetVerbosity() const
	{
		return (ELogVerbosity::Type)Verbosity;
	}

	void SetVerbosity(ELogVerbosity::Type NewVerbosity)
	{
		const uint8 NewWorkingVerbosity = (uint8)(NewVerbosity & ELogVerbosity::VerbosityMask);
		Verbosity = (ELogVerbosity::Type)(CompileTimeVerbosity < NewWorkingVerbosity ? CompileTimeVerbosity : NewWorkingVerbosity);
		DebugBreakOnLog = !!(NewVerbosity & ELogVerbosity::BreakOnLog);
	}

protected:

	constexpr FLogCategoryBase(ELogVerbosity::Type InDefaultVerbosity, ELogVerbosity::Type InCompileTimeVerbosity)
		: Verbosity((ELogVerbosity::Type)(InCompileTimeVerbosity < (InDefaultVerbosity & ELogVerbosity::VerbosityMask) ? InCompileTimeVerbosity : (InDefaultVerbosity & ELogVerbosity::VerbosityMask)))
		, DebugBreakOnLog(!!(InDefaultVerbosity & ELogVerbosity::BreakOnLog))
		, DefaultVerbosity(InDefaultVerbosity)
		, CompileTimeVerbosity(InCompileTimeVerbosity)
		, CategoryFName()
	{
	}

	FLogCategoryBase(const FLogCategoryBase&) = delete;
	FLogCategoryBase& operator=(const FLogCategoryBase&) = delete;

	ELogVerbosity::Type Verbosity;

	bool DebugBreakOnLog;

	uint8 DefaultVerbosity;

	ELogVerbosity::Type CompileTimeVerbosity;

	FName CategoryFName;
};

struct FDeferredLogCategory : public FLogCategoryBase
{
	FORCEINLINE bool IsSuppressed(ELogVerbosity::Type VerbosityLevel) const
	{
		Register();
		return FLogCategoryBase::IsSuppressed(VerbosityLevel);
	}

	void PostTrigger(ELogVerbosity::Type VerbosityLevel)
	{
		Register();
		FLogCategoryBase::PostTrigger(VerbosityLevel);
	}

	FORCEINLINE FName GetCategoryName() const
	{
		Register();
		return FLogCategoryBase::GetCategoryName();
	}

	ELogVerbosity::Type GetVerbosity() const
	{
		Register();
		return FLogCategoryBase::GetVerbosity();
	}

	void SetVerbosity(ELogVerbosity::Type NewVerbosity)
	{
		Register();
		FLogCategoryBase::SetVerbosity(NewVerbosity);
	}

	FORCEINLINE void Register() const
	{
		if (!bRegistered.load(std::memory_order_acquire))
		{
			RegisterSlow();
		}
	}

protected:
	constexpr FDeferredLogCategory(const TCHAR* InCategoryName, ELogVerbosity::Type InDefaultVerbosity, ELogVerbosity::Type InCompileTimeVerbosity)
		: FLogCategoryBase(InDefaultVerbosity, InCompileTimeVerbosity)
		, PendingCategoryName(InCategoryName)
	{
	}

	~FDeferredLogCategory()
	{
		if (bRegistered.load(std::memory_order_acquire) && !UE4Logging_Private::IsProcessShuttingDown())
		{
			UE4Logging_Private::GameFunction<void(*)(FLogCategoryBase*)>(0x3280E00)(this);
		}
	}

private:
	static BOOL CALLBACK RegisterOnceCallback(PINIT_ONCE, PVOID Parameter, PVOID*)
	{
		FDeferredLogCategory* Category = static_cast<FDeferredLogCategory*>(Parameter);
		const FName CategoryName(Category->PendingCategoryName);

		UE4Logging_Private::GameFunction<FLogCategoryBase*(*)(FLogCategoryBase*, const FName&, ELogVerbosity::Type, ELogVerbosity::Type)>(0x327FF80)
			(Category, CategoryName, (ELogVerbosity::Type)Category->DefaultVerbosity, Category->CompileTimeVerbosity);

		Category->bRegistered.store(true, std::memory_order_release);
		return TRUE;
	}

	FORCENOINLINE void RegisterSlow() const
	{
		InitOnceExecuteOnce(&RegisterOnce, &RegisterOnceCallback, const_cast<FDeferredLogCategory*>(this), nullptr);
	}

	const TCHAR* PendingCategoryName;
	mutable INIT_ONCE RegisterOnce = INIT_ONCE_STATIC_INIT;
	mutable std::atomic<bool> bRegistered = false;
};

template<uint8 InDefaultVerbosity, uint8 InCompileTimeVerbosity>
struct FLogCategory : public FDeferredLogCategory
{
	enum
	{
		CompileTimeVerbosity = InCompileTimeVerbosity
	};

	constexpr FLogCategory(const TCHAR* CategoryName)
		: FDeferredLogCategory(CategoryName, ELogVerbosity::Type(InDefaultVerbosity), ELogVerbosity::Type(CompileTimeVerbosity))
	{
	}
};

class FLogScopedCategoryAndVerbosityOverride
{
public:

	struct FOverride
	{
		ELogVerbosity::Type Verbosity;
		FName Category;
		FOverride()
			: Verbosity(ELogVerbosity::Log)
		{
		}
		FOverride(FName InCategory, ELogVerbosity::Type InVerbosity)
			: Verbosity(InVerbosity)
			, Category(InCategory)
		{
		}
		void operator=(const FOverride& Other)
		{
			Verbosity = Other.Verbosity;
			Category = Other.Category;
		}
	};
private:

	FOverride Backup;
public:

	FLogScopedCategoryAndVerbosityOverride(FName Category, ELogVerbosity::Type Verbosity)
	{
		UE4Logging_Private::GameFunction<FLogScopedCategoryAndVerbosityOverride*(*)(FLogScopedCategoryAndVerbosityOverride*, FName, ELogVerbosity::Type)>(0x3280050)(this, Category, Verbosity);
	}

	~FLogScopedCategoryAndVerbosityOverride()
	{
		UE4Logging_Private::GameFunction<void(*)(FLogScopedCategoryAndVerbosityOverride*)>(0x3280EB0)(this);
	}

	FLogScopedCategoryAndVerbosityOverride(const FLogScopedCategoryAndVerbosityOverride&) = delete;
	FLogScopedCategoryAndVerbosityOverride& operator=(const FLogScopedCategoryAndVerbosityOverride&) = delete;

	static FOverride* GetTLSCurrent()
	{
		return UE4Logging_Private::GameFunction<FOverride*(*)()>(0x328C450)();
	}
};

typedef FLogScopedCategoryAndVerbosityOverride FScopedCategoryAndVerbosityOverride;

class FLogScopedVerbosityOverride
{
private:

	FLogCategoryBase* SavedCategory;
	ELogVerbosity::Type SavedVerbosity;

public:

	template <typename CategoryType>
	FLogScopedVerbosityOverride(CategoryType* Category, ELogVerbosity::Type Verbosity)
		: SavedCategory(Category)
	{
		SavedVerbosity = Category->GetVerbosity();
		Category->SetVerbosity(Verbosity);
	}

	~FLogScopedVerbosityOverride()
	{
		SavedCategory->SetVerbosity(SavedVerbosity);
	}

	FLogScopedVerbosityOverride(const FLogScopedVerbosityOverride&) = delete;
	FLogScopedVerbosityOverride& operator=(const FLogScopedVerbosityOverride&) = delete;
};

struct FDebug
{
	template <typename... Types>
	static void AssertFailed(const ANSICHAR* Expr, const ANSICHAR* File, int32 Line, const TCHAR* Format = TEXT(""), Types... Args)
	{
		UE4Logging_Private::GameFunction<void(*)(const ANSICHAR*, const ANSICHAR*, int32, const TCHAR*, ...)>(0x32A3F70)(Expr, File, Line, Format, Args...);
	}

	static void ProcessFatalError()
	{
		UE4Logging_Private::GameFunction<void(*)()>(0x32B65A0)();
	}
};

template <typename... Types>
void LowLevelFatalErrorHandler(const ANSICHAR* File, int32 Line, const TCHAR* Format = TEXT(""), Types... Args)
{
	UE4Logging_Private::GameFunction<void(*)(const ANSICHAR*, int32, const TCHAR*, ...)>(0x32B2D90)(File, Line, Format, Args...);
}

#define LowLevelFatalError(Format, ...) \
	{ \
		LowLevelFatalErrorHandler(__FILE__, __LINE__, UE_LOG_TEXT(Format), ##__VA_ARGS__); \
		_DebugBreakAndPromptForRemote(); \
		FDebug::ProcessFatalError(); \
	}

#ifndef NO_LOGGING
	#define NO_LOGGING 0
#endif

struct FMsg
{
	template <typename FmtType, typename... Types>
	static void Logf(const ANSICHAR* File, int32 Line, const FName& Category, ELogVerbosity::Type Verbosity, const FmtType& Fmt, Types... Args)
	{
		LogfImpl(File, Line, Category, Verbosity, Fmt, Args...);
	}

	template <typename FmtType, typename... Types>
	static void Logf_Internal(const ANSICHAR* File, int32 Line, const FName& Category, ELogVerbosity::Type Verbosity, const FmtType& Fmt, Types... Args)
	{
		Logf_InternalImpl(File, Line, Category, Verbosity, Fmt, Args...);
	}

private:
	typedef void (*FLogfImplFn)(const ANSICHAR* File, int32 Line, const FName& Category, ELogVerbosity::Type Verbosity, const TCHAR* Fmt, ...);

	template <typename... Types>
	static void LogfImpl(const ANSICHAR* File, int32 Line, const FName& Category, ELogVerbosity::Type Verbosity, const TCHAR* Fmt, Types... Args)
	{
		UE4Logging_Private::GameFunction<FLogfImplFn>(0x328D470)(File, Line, Category, Verbosity, Fmt, Args...);
	}

	template <typename... Types>
	static void Logf_InternalImpl(const ANSICHAR* File, int32 Line, const FName& Category, ELogVerbosity::Type Verbosity, const TCHAR* Fmt, Types... Args)
	{
		UE4Logging_Private::GameFunction<FLogfImplFn>(0x328D6B0)(File, Line, Category, Verbosity, Fmt, Args...);
	}
};

#ifndef COMPILED_IN_MINIMUM_VERBOSITY
	#define COMPILED_IN_MINIMUM_VERBOSITY VeryVerbose
#endif

#define UE_LOG_EXPAND_IS_FATAL(Verbosity, ActiveBlock, InactiveBlock) PREPROCESSOR_JOIN(UE_LOG_EXPAND_IS_FATAL_, Verbosity)(ActiveBlock, InactiveBlock)

#define UE_LOG_EXPAND_IS_FATAL_Fatal(      ActiveBlock, InactiveBlock) ActiveBlock
#define UE_LOG_EXPAND_IS_FATAL_Error(      ActiveBlock, InactiveBlock) InactiveBlock
#define UE_LOG_EXPAND_IS_FATAL_Warning(    ActiveBlock, InactiveBlock) InactiveBlock
#define UE_LOG_EXPAND_IS_FATAL_Display(    ActiveBlock, InactiveBlock) InactiveBlock
#define UE_LOG_EXPAND_IS_FATAL_Log(        ActiveBlock, InactiveBlock) InactiveBlock
#define UE_LOG_EXPAND_IS_FATAL_Verbose(    ActiveBlock, InactiveBlock) InactiveBlock
#define UE_LOG_EXPAND_IS_FATAL_VeryVerbose(ActiveBlock, InactiveBlock) InactiveBlock
#define UE_LOG_EXPAND_IS_FATAL_All(        ActiveBlock, InactiveBlock) InactiveBlock
#define UE_LOG_EXPAND_IS_FATAL_SetColor(   ActiveBlock, InactiveBlock) InactiveBlock

#define UE_LOG_SOURCE_FILE(File) File

#if NO_LOGGING

	struct FNoLoggingCategory {};

	#define UE_LOG(CategoryName, Verbosity, Format, ...) \
	{ \
		if (ELogVerbosity::Verbosity == ELogVerbosity::Fatal) \
		{ \
			LowLevelFatalErrorHandler(UE_LOG_SOURCE_FILE(__FILE__), __LINE__, UE_LOG_TEXT(Format), ##__VA_ARGS__); \
			_DebugBreakAndPromptForRemote(); \
			FDebug::ProcessFatalError(); \
			UE_LOG_EXPAND_IS_FATAL(Verbosity, CA_ASSUME(false);, PREPROCESSOR_NOTHING) \
		} \
	}

	#define UE_CLOG(Condition, CategoryName, Verbosity, Format, ...) \
	{ \
		if (ELogVerbosity::Verbosity == ELogVerbosity::Fatal) \
		{ \
			if (Condition) \
			{ \
				LowLevelFatalErrorHandler(UE_LOG_SOURCE_FILE(__FILE__), __LINE__, UE_LOG_TEXT(Format), ##__VA_ARGS__); \
				_DebugBreakAndPromptForRemote(); \
				FDebug::ProcessFatalError(); \
				UE_LOG_EXPAND_IS_FATAL(Verbosity, CA_ASSUME(false);, PREPROCESSOR_NOTHING) \
			} \
		} \
	}

	#define UE_LOG_ACTIVE(...)				(0)
	#define UE_LOG_ANY_ACTIVE(...)			(0)
	#define UE_SUPPRESS(...) {}
	#define UE_GET_LOG_VERBOSITY(...)		(ELogVerbosity::NoLogging)
	#define UE_SET_LOG_VERBOSITY(...)
	#define DECLARE_LOG_CATEGORY_EXTERN(CategoryName, DefaultVerbosity, CompileTimeVerbosity) extern FNoLoggingCategory CategoryName;
	#define DEFINE_LOG_CATEGORY(...)
	#define DEFINE_LOG_CATEGORY_STATIC(...)
	#define DECLARE_LOG_CATEGORY_CLASS(...)
	#define DEFINE_LOG_CATEGORY_CLASS(...)
	#define UE_SECURITY_LOG(...)
	#define LOG_SCOPE_VERBOSITY_OVERRIDE(...)

	#define DECLARE_GAME_LOG_CATEGORY(CategoryName, Offset, InCompileTimeVerbosity) inline FNoLoggingCategory CategoryName;

#else

	namespace UE4Asserts_Private
	{
		template <int32 VerbosityToCheck, typename CategoryType>
		FORCEINLINE
			typename std::enable_if<
				((VerbosityToCheck & ELogVerbosity::VerbosityMask) <= CategoryType::CompileTimeVerbosity &&
				(VerbosityToCheck & ELogVerbosity::VerbosityMask) <= ELogVerbosity::COMPILED_IN_MINIMUM_VERBOSITY),
				bool>::type
			IsLogActive(const CategoryType& Category)
		{
			return !Category.IsSuppressed((ELogVerbosity::Type)VerbosityToCheck);
		}

		template <int32 VerbosityToCheck, typename CategoryType>
		FORCEINLINE
			typename std::enable_if<
				!((VerbosityToCheck & ELogVerbosity::VerbosityMask) <= CategoryType::CompileTimeVerbosity &&
				(VerbosityToCheck & ELogVerbosity::VerbosityMask) <= ELogVerbosity::COMPILED_IN_MINIMUM_VERBOSITY),
				bool>::type
			IsLogActive(const CategoryType& Category)
		{
			return false;
		}
	}

	#define UE_LOG_ACTIVE(CategoryName, Verbosity) (::UE4Asserts_Private::IsLogActive<(int32)ELogVerbosity::Verbosity>(CategoryName))

	#define UE_GET_LOG_VERBOSITY(CategoryName) \
		CategoryName.GetVerbosity()

	#define UE_SET_LOG_VERBOSITY(CategoryName, Verbosity) \
		CategoryName.SetVerbosity(ELogVerbosity::Verbosity);

	#define UE_LOG(CategoryName, Verbosity, Format, ...) \
	{ \
		CA_CONSTANT_IF((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) <= ELogVerbosity::COMPILED_IN_MINIMUM_VERBOSITY && (ELogVerbosity::Warning & ELogVerbosity::VerbosityMask) <= FLogCategory##CategoryName::CompileTimeVerbosity) \
		{ \
			UE_LOG_EXPAND_IS_FATAL(Verbosity, PREPROCESSOR_NOTHING, if (!CategoryName.IsSuppressed(ELogVerbosity::Verbosity))) \
			{ \
				TRACE_LOG_MESSAGE(CategoryName, Verbosity, Format, ##__VA_ARGS__) \
				UE_LOG_EXPAND_IS_FATAL(Verbosity, \
					{ \
						FMsg::Logf_Internal(UE_LOG_SOURCE_FILE(__FILE__), __LINE__, CategoryName.GetCategoryName(), ELogVerbosity::Verbosity, UE_LOG_TEXT(Format), ##__VA_ARGS__); \
						_DebugBreakAndPromptForRemote(); \
						FDebug::ProcessFatalError(); \
						CA_ASSUME(false); \
					}, \
					{ \
						FMsg::Logf_Internal(nullptr, 0, CategoryName.GetCategoryName(), ELogVerbosity::Verbosity, UE_LOG_TEXT(Format), ##__VA_ARGS__); \
					} \
				) \
			} \
		} \
	}

	#define UE_SECURITY_LOG(NetConnection, SecurityEventType, Format, ...) \
	{ \
		CA_CONSTANT_IF((ELogVerbosity::Warning & ELogVerbosity::VerbosityMask) <= ELogVerbosity::COMPILED_IN_MINIMUM_VERBOSITY && (ELogVerbosity::Warning & ELogVerbosity::VerbosityMask) <= FLogCategoryLogSecurity::CompileTimeVerbosity) \
		{ \
			if (!LogSecurity.IsSuppressed(ELogVerbosity::Warning)) \
			{ \
				FMsg::Logf_Internal(UE_LOG_SOURCE_FILE(__FILE__), __LINE__, LogSecurity.GetCategoryName(), ELogVerbosity::Warning, UE_LOG_TEXT(TEXT("%s: %s: ") Format), *::UE4Logging_Private::FScopedString(NetConnection->RemoteAddressToString()), ToString(SecurityEventType), ##__VA_ARGS__); \
			} \
		} \
	}

	#define UE_CLOG(Condition, CategoryName, Verbosity, Format, ...) \
	{ \
		CA_CONSTANT_IF((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) <= ELogVerbosity::COMPILED_IN_MINIMUM_VERBOSITY && (ELogVerbosity::Warning & ELogVerbosity::VerbosityMask) <= FLogCategory##CategoryName::CompileTimeVerbosity) \
		{ \
			UE_LOG_EXPAND_IS_FATAL(Verbosity, PREPROCESSOR_NOTHING, if (!CategoryName.IsSuppressed(ELogVerbosity::Verbosity))) \
			{ \
				if (Condition) \
				{ \
					TRACE_LOG_MESSAGE(CategoryName, Verbosity, Format, ##__VA_ARGS__) \
					UE_LOG_EXPAND_IS_FATAL(Verbosity, \
						{ \
							FMsg::Logf_Internal(UE_LOG_SOURCE_FILE(__FILE__), __LINE__, CategoryName.GetCategoryName(), ELogVerbosity::Verbosity, UE_LOG_TEXT(Format), ##__VA_ARGS__); \
							_DebugBreakAndPromptForRemote(); \
							FDebug::ProcessFatalError(); \
							CA_ASSUME(false); \
						}, \
						{ \
							FMsg::Logf_Internal(nullptr, 0, CategoryName.GetCategoryName(), ELogVerbosity::Verbosity, UE_LOG_TEXT(Format), ##__VA_ARGS__); \
						} \
					) \
					CA_ASSUME(true); \
				} \
			} \
		} \
	}

	#define UE_SUPPRESS(CategoryName, Verbosity, ExecuteIfUnsuppressed) \
	{ \
		CA_CONSTANT_IF((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) <= ELogVerbosity::COMPILED_IN_MINIMUM_VERBOSITY && (ELogVerbosity::Warning & ELogVerbosity::VerbosityMask) <= FLogCategory##CategoryName::CompileTimeVerbosity) \
		{ \
			if (!CategoryName.IsSuppressed(ELogVerbosity::Verbosity)) \
			{ \
				FScopedCategoryAndVerbosityOverride TEMP__##CategoryName(CategoryName.GetCategoryName(), ELogVerbosity::Type(ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask)); \
				ExecuteIfUnsuppressed; \
				CategoryName.PostTrigger(ELogVerbosity::Verbosity); \
			} \
		} \
	}

	#define DECLARE_LOG_CATEGORY_EXTERN(CategoryName, DefaultVerbosity, CompileTimeVerbosity) \
		extern struct FLogCategory##CategoryName : public FLogCategory<ELogVerbosity::DefaultVerbosity, ELogVerbosity::CompileTimeVerbosity> \
		{ \
			constexpr FLogCategory##CategoryName() : FLogCategory(TEXT(#CategoryName)) {} \
		} CategoryName;

	#define DEFINE_LOG_CATEGORY(CategoryName) FLogCategory##CategoryName CategoryName;

	#define DEFINE_LOG_CATEGORY_STATIC(CategoryName, DefaultVerbosity, CompileTimeVerbosity) \
		static struct FLogCategory##CategoryName : public FLogCategory<ELogVerbosity::DefaultVerbosity, ELogVerbosity::CompileTimeVerbosity> \
		{ \
			constexpr FLogCategory##CategoryName() : FLogCategory(TEXT(#CategoryName)) {} \
		} CategoryName;

	#define DECLARE_LOG_CATEGORY_CLASS(CategoryName, DefaultVerbosity, CompileTimeVerbosity) \
		DEFINE_LOG_CATEGORY_STATIC(CategoryName, DefaultVerbosity, CompileTimeVerbosity)

	#define DEFINE_LOG_CATEGORY_CLASS(Class, CategoryName) Class::FLogCategory##CategoryName Class::CategoryName;

	#define LOG_SCOPE_VERBOSITY_OVERRIDE(CategoryName, ScopeVerbosity) FLogScopedVerbosityOverride LogCategory##CategoryName##Override(&CategoryName, ScopeVerbosity)

	#define DECLARE_GAME_LOG_CATEGORY(CategoryName, Offset, InCompileTimeVerbosity) \
		struct FLogCategory##CategoryName : public FLogCategoryBase \
		{ \
			enum { CompileTimeVerbosity = ELogVerbosity::InCompileTimeVerbosity }; \
		}; \
		inline FLogCategory##CategoryName& CategoryName = *reinterpret_cast<FLogCategory##CategoryName*>(ImageBase + (Offset));

#endif

DECLARE_GAME_LOG_CATEGORY(AFA_Log,                                     0x8E609E0, All)
DECLARE_GAME_LOG_CATEGORY(AGeometryCollectionActorLogging,             0x8E60F40, All)
DECLARE_GAME_LOG_CATEGORY(AmbientAudio,                                0x8BC7420, All)
DECLARE_GAME_LOG_CATEGORY(ChromeTraversePoint,                         0x8C028A8, All)
DECLARE_GAME_LOG_CATEGORY(ControlRigProfilingLog,                      0x8BBCCC0, All)
DECLARE_GAME_LOG_CATEGORY(FGeometryCollectionLogging,                  0x8E60040, All)
DECLARE_GAME_LOG_CATEGORY(FGeometryCollectionSceneProxyLogging,        0x8E61420, All)
DECLARE_GAME_LOG_CATEGORY(FGeometryCollectionUtilityLogging,           0x8E60110, All)
DECLARE_GAME_LOG_CATEGORY(FManagedArrayCollectionLogging,              0x8E60120, All)
DECLARE_GAME_LOG_CATEGORY(FMeshElementCollector_AddMesh,               0x8E8BA70, All)
DECLARE_GAME_LOG_CATEGORY(FortInstallBundleManager,                    0x8BBF7C0, All)
DECLARE_GAME_LOG_CATEGORY(FortLogMinigameSettingsBuilding,             0x8C4DDD0, All)
DECLARE_GAME_LOG_CATEGORY(FortPlayerPerksItemDefinition,               0x8C2A268, All)
DECLARE_GAME_LOG_CATEGORY(FortSoundCueTemplates,                       0x8BC7240, All)
DECLARE_GAME_LOG_CATEGORY(FSC_Log,                                     0x8E60850, All)
DECLARE_GAME_LOG_CATEGORY(FSCA_Log,                                    0x8E2B610, All)
DECLARE_GAME_LOG_CATEGORY(FSSP_Log,                                    0x8E60A50, All)
DECLARE_GAME_LOG_CATEGORY(FTransformCollectionLogging,                 0x8E60170, All)
DECLARE_GAME_LOG_CATEGORY(GameplayMediaEncoder,                        0x8EA6A40, All)
DECLARE_GAME_LOG_CATEGORY(GCS_Log,                                     0x8E60370, All)
DECLARE_GAME_LOG_CATEGORY(GeometryCollectionAlgoLog,                   0x8E60090, All)
DECLARE_GAME_LOG_CATEGORY(HighlightRecorder,                           0x8EA6AF0, All)
DECLARE_GAME_LOG_CATEGORY(JPEGLog,                                     0x8D6C760, All)
DECLARE_GAME_LOG_CATEGORY(LogAbilitySystem,                            0x8B9C7E8, All)
DECLARE_GAME_LOG_CATEGORY(LogAbilitySystemComponent,                   0x8B9C4D0, All)
DECLARE_GAME_LOG_CATEGORY(LogAbilityTargetingValidation,               0x8BE3FC0, All)
DECLARE_GAME_LOG_CATEGORY(LogAccessibility,                            0x8D62470, All)
DECLARE_GAME_LOG_CATEGORY(LogAccount,                                  0x8BB90E0, All)
DECLARE_GAME_LOG_CATEGORY(LogActor,                                    0x8E6E0E0, Warning)
DECLARE_GAME_LOG_CATEGORY(LogActorComponent,                           0x8E70180, All)
DECLARE_GAME_LOG_CATEGORY(LogAdvertising,                              0x8E6DD08, All)
DECLARE_GAME_LOG_CATEGORY(LogAIBlueprint,                              0x8EBDD50, All)
DECLARE_GAME_LOG_CATEGORY(LogAIDropper,                                0x8BE5DB0, All)
DECLARE_GAME_LOG_CATEGORY(LogAIHotSpot,                                0x8BE5168, All)
DECLARE_GAME_LOG_CATEGORY(LogAIModule,                                 0x8EBDB50, All)
DECLARE_GAME_LOG_CATEGORY(LogAINavigation,                             0x8EBDB00, All)
DECLARE_GAME_LOG_CATEGORY(LogAIPerception,                             0x8EBE0A0, All)
DECLARE_GAME_LOG_CATEGORY(LogAIService,                                0x8BF9D60, All)
DECLARE_GAME_LOG_CATEGORY(LogAISpawner,                                0x8BF9E08, All)
DECLARE_GAME_LOG_CATEGORY(LogAISpawnerData,                            0x8BF9320, All)
DECLARE_GAME_LOG_CATEGORY(LogAISub,                                    0x8EBDB70, All)
DECLARE_GAME_LOG_CATEGORY(LogAnalytics,                                0x8E2A130, All)
DECLARE_GAME_LOG_CATEGORY(LogAnalyticsBPLib,                           0x8EBFCC0, All)
DECLARE_GAME_LOG_CATEGORY(LogAndroidPermission,                        0x8BBB330, All)
DECLARE_GAME_LOG_CATEGORY(LogAnimation,                                0x8E6E610, All)
DECLARE_GAME_LOG_CATEGORY(LogAnimationCompression,                     0x8E6E6A0, All)
DECLARE_GAME_LOG_CATEGORY(LogAnimationCore,                            0x8E5FCE8, All)
DECLARE_GAME_LOG_CATEGORY(LogAnimationSharing,                         0x8BBB628, All)
DECLARE_GAME_LOG_CATEGORY(LogAnimMarkerSync,                           0x8E6ECE0, All)
DECLARE_GAME_LOG_CATEGORY(LogAnimMontage,                              0x8E6EB28, All)
DECLARE_GAME_LOG_CATEGORY(LogAnimNotify,                               0x8E6E938, All)
DECLARE_GAME_LOG_CATEGORY(LogAnimTrails,                               0x8E6EC78, All)
DECLARE_GAME_LOG_CATEGORY(LogApp,                                      0x8D26660, All)
DECLARE_GAME_LOG_CATEGORY(LogAppES,                                    0x8BE18C8, All)
DECLARE_GAME_LOG_CATEGORY(LogAR,                                       0x8E2BCC0, All)
DECLARE_GAME_LOG_CATEGORY(LogArchiveDiff,                              0x8D5BCF8, All)
DECLARE_GAME_LOG_CATEGORY(LogArchiveObjectCrc32,                       0x8D5BCE8, All)
DECLARE_GAME_LOG_CATEGORY(LogArchiveUObject,                           0x8D5BD08, All)
DECLARE_GAME_LOG_CATEGORY(LogArray,                                    0x8E7D5A8, All)
DECLARE_GAME_LOG_CATEGORY(LogAssetData,                                0x8E6B900, All)
DECLARE_GAME_LOG_CATEGORY(LogAssetManager,                             0x8E6EEB8, All)
DECLARE_GAME_LOG_CATEGORY(LogAssetRegistry,                            0x8E6B928, All)
DECLARE_GAME_LOG_CATEGORY(LogAssetTags,                                0x8BD32B8, All)
DECLARE_GAME_LOG_CATEGORY(LogAsyncArchive,                             0x8D5BDB8, All)
DECLARE_GAME_LOG_CATEGORY(LogAsyncMixin,                               0x8BF83B0, All)
DECLARE_GAME_LOG_CATEGORY(LogAsyncTaskQueue,                           0x8BF8400, All)
DECLARE_GAME_LOG_CATEGORY(LogAthenaAI,                                 0x8BE5200, Warning)
DECLARE_GAME_LOG_CATEGORY(LogAthenaAISystem,                           0x8BE5DC0, Warning)
DECLARE_GAME_LOG_CATEGORY(LogAthenaBots,                               0x8C017E0, All)
DECLARE_GAME_LOG_CATEGORY(LogAthenaBroadcastSquadStatusWidget,         0x8EC91F0, All)
DECLARE_GAME_LOG_CATEGORY(LogAthenaEventMatchScoreboard,               0x8EC9D40, All)
DECLARE_GAME_LOG_CATEGORY(LogAthenaExitCraft,                          0x8BFABD8, All)
DECLARE_GAME_LOG_CATEGORY(LogAthenaExitCraftSpawner,                   0x8BFABE8, All)
DECLARE_GAME_LOG_CATEGORY(LogAthenaHUDContext,                         0x8ECB6F0, All)
DECLARE_GAME_LOG_CATEGORY(LogAthenaItemShop_OfferDebugging,            0x8EC6F10, All)
DECLARE_GAME_LOG_CATEGORY(LogAthenaLevelEffects,                       0x8EC6120, All)
DECLARE_GAME_LOG_CATEGORY(LogAthenaNavMesh,                            0x8BEABF0, All)
DECLARE_GAME_LOG_CATEGORY(LogAthenaPathing,                            0x8BE5DF0, Warning)
DECLARE_GAME_LOG_CATEGORY(LogAthenaSpectatorUIContext,                 0x8ECB700, All)
DECLARE_GAME_LOG_CATEGORY(LogAthenaSupplyDrop,                         0x8BFBA10, All)
DECLARE_GAME_LOG_CATEGORY(LogAthenaWinCondition,                       0x8BFD040, All)
DECLARE_GAME_LOG_CATEGORY(LogAudio,                                    0x8E6EFA0, All)
DECLARE_GAME_LOG_CATEGORY(LogAudioAnalyzer,                            0x8F0A200, All)
DECLARE_GAME_LOG_CATEGORY(LogAudioDebug,                               0x8E6EFB0, All)
DECLARE_GAME_LOG_CATEGORY(LogAudioDerivedData,                         0x8E6F1C8, All)
DECLARE_GAME_LOG_CATEGORY(LogAudioEndpoints,                           0x8E6AF70, All)
DECLARE_GAME_LOG_CATEGORY(LogAudioMixer,                               0x8E6AE38, All)
DECLARE_GAME_LOG_CATEGORY(LogAudioMixerDebug,                          0x8E6AE48, All)
DECLARE_GAME_LOG_CATEGORY(LogAudioResampler,                           0x8E6AF28, All)
DECLARE_GAME_LOG_CATEGORY(LogAudioSynesthesia,                         0x8BB8570, All)
DECLARE_GAME_LOG_CATEGORY(LogAudioSynesthesiaCore,                     0x8BB8520, All)
DECLARE_GAME_LOG_CATEGORY(LogAutoDestroySubsystem,                     0x8E74970, All)
DECLARE_GAME_LOG_CATEGORY(LogAutomationTest,                           0x8D266E8, All)
DECLARE_GAME_LOG_CATEGORY(LogAvatar,                                   0x8E6FB90, All)
DECLARE_GAME_LOG_CATEGORY(LogAVEncoder,                                0x8EA6960, All)
DECLARE_GAME_LOG_CATEGORY(LogAvfMediaFactory,                          0x8BD3260, All)
DECLARE_GAME_LOG_CATEGORY(LogAVIWriter,                                0x8E6CC98, All)
DECLARE_GAME_LOG_CATEGORY(LogBackgroundHttpFileHash,                   0x8EA9970, All)
DECLARE_GAME_LOG_CATEGORY(LogBackgroundHttpManager,                    0x8EA99D0, All)
DECLARE_GAME_LOG_CATEGORY(LogBackgroundHttpRequest,                    0x8EA99E0, All)
DECLARE_GAME_LOG_CATEGORY(LogBatchedElements,                          0x8E6F7B0, All)
DECLARE_GAME_LOG_CATEGORY(LogBattleMap,                                0x8C0A140, All)
DECLARE_GAME_LOG_CATEGORY(LogBattlEye,                                 0x8EBD3B8, All)
DECLARE_GAME_LOG_CATEGORY(LogBeacon,                                   0x8BA2638, All)
DECLARE_GAME_LOG_CATEGORY(LogBehaviorTree,                             0x8EBDBC0, All)
DECLARE_GAME_LOG_CATEGORY(LogBlackboard,                               0x8EBDC30, All)
DECLARE_GAME_LOG_CATEGORY(LogBlueprint,                                0x8E6F7D0, All)
DECLARE_GAME_LOG_CATEGORY(LogBlueprintContext,                         0x8BBC980, All)
DECLARE_GAME_LOG_CATEGORY(LogBlueprintFuncLibrary,                     0x8E6F7E0, All)
DECLARE_GAME_LOG_CATEGORY(LogBlueprintSupport,                         0x8D5BB80, All)
DECLARE_GAME_LOG_CATEGORY(LogBlueprintUserMessages,                    0x8E6E1D0, All)
DECLARE_GAME_LOG_CATEGORY(LogBrain,                                    0x8EBDD70, All)
DECLARE_GAME_LOG_CATEGORY(LogBrush,                                    0x8E6F820, All)
DECLARE_GAME_LOG_CATEGORY(LogBrushComponent,                           0x8E70208, All)
DECLARE_GAME_LOG_CATEGORY(LogBufferVisualization,                      0x8E8AB08, All)
DECLARE_GAME_LOG_CATEGORY(LogBuilding,                                 0x8C0A4A8, All)
DECLARE_GAME_LOG_CATEGORY(LogBuildingCustomizableSpawnContainer,       0x8C0A8E0, All)
DECLARE_GAME_LOG_CATEGORY(LogBuildPatchServices,                       0x8E2DB10, All)
DECLARE_GAME_LOG_CATEGORY(LogBuildStreamer,                            0x8E2E058, All)
DECLARE_GAME_LOG_CATEGORY(LogBulkDataRuntime,                          0x8D5C150, All)
DECLARE_GAME_LOG_CATEGORY(LogCamera,                                   0x8E6F9B8, All)
DECLARE_GAME_LOG_CATEGORY(LogCameraAnim,                               0x8E6F860, All)
DECLARE_GAME_LOG_CATEGORY(LogCameraPhotography,                        0x8E6F9C8, All)
DECLARE_GAME_LOG_CATEGORY(LogCanvas,                                   0x8E91538, All)
DECLARE_GAME_LOG_CATEGORY(LogCardMaterialPooler,                       0x8ED49C8, All)
DECLARE_GAME_LOG_CATEGORY(LogCasts,                                    0x8D5C608, All)
DECLARE_GAME_LOG_CATEGORY(LogCatalogHelper,                            0x8BBA938, All)
DECLARE_GAME_LOG_CATEGORY(LogCbor,                                     0x8EA5F40, All)
DECLARE_GAME_LOG_CATEGORY(LogCEF3Utils,                                0x8EA87A0, All)
DECLARE_GAME_LOG_CATEGORY(LogChaos,                                    0x8E2B560, All)
DECLARE_GAME_LOG_CATEGORY(LogChaosCloth,                               0x8BD04D8, All)
DECLARE_GAME_LOG_CATEGORY(LogChaosDebug,                               0x8E2B550, All)
DECLARE_GAME_LOG_CATEGORY(LogChaosGeneral,                             0x8E2B520, All)
DECLARE_GAME_LOG_CATEGORY(LogChaosJoint,                               0x8E2AD60, Warning)
DECLARE_GAME_LOG_CATEGORY(LogChaosMinEvolution,                        0x8E2A5C0, Warning)
DECLARE_GAME_LOG_CATEGORY(LogChaosProximity,                           0x8E60100, All)
DECLARE_GAME_LOG_CATEGORY(LogChaosSimulation,                          0x8E2B540, All)
DECLARE_GAME_LOG_CATEGORY(LogChaosSpatialHash,                         0x8E2B410, All)
DECLARE_GAME_LOG_CATEGORY(LogChaosThread,                              0x8E2B530, All)
DECLARE_GAME_LOG_CATEGORY(LogCharacter,                                0x8E6FB80, All)
DECLARE_GAME_LOG_CATEGORY(LogCharacterMovement,                        0x8E70250, All)
DECLARE_GAME_LOG_CATEGORY(LogCharacterNetSmoothing,                    0x8E70270, All)
DECLARE_GAME_LOG_CATEGORY(LogChartCreation,                            0x8E6FBB8, All)
DECLARE_GAME_LOG_CATEGORY(LogCheatManager,                             0x8E6FC50, All)
DECLARE_GAME_LOG_CATEGORY(LogCheckComponents,                          0x8E6E288, All)
DECLARE_GAME_LOG_CATEGORY(LogCheckSubobjects,                          0x8D5D1F8, All)
DECLARE_GAME_LOG_CATEGORY(LogChildActorComponent,                      0x8E705B8, All)
DECLARE_GAME_LOG_CATEGORY(LogChunkDatabaseWriter,                      0x8E2E008, All)
DECLARE_GAME_LOG_CATEGORY(LogChunkDbChunkSource,                       0x8E2E0A8, All)
DECLARE_GAME_LOG_CATEGORY(LogChunkDeltaOptimiser,                      0x8E2E018, All)
DECLARE_GAME_LOG_CATEGORY(LogChunkInstaller,                           0x8CECA70, All)
DECLARE_GAME_LOG_CATEGORY(LogChunkReferenceTracker,                    0x8E2E0B8, All)
DECLARE_GAME_LOG_CATEGORY(LogChunkWriter,                              0x8E2E028, All)
DECLARE_GAME_LOG_CATEGORY(LogClass,                                    0x8D5C628, All)
DECLARE_GAME_LOG_CATEGORY(LogClearQuad,                                0x8E44B88, Log)
DECLARE_GAME_LOG_CATEGORY(LogClient,                                   0x8E90B78, All)
DECLARE_GAME_LOG_CATEGORY(LogClothingAsset,                            0x8EA6650, All)
DECLARE_GAME_LOG_CATEGORY(LogClothingMeshUtils,                        0x8EA6720, All)
DECLARE_GAME_LOG_CATEGORY(LogCloudChunkSource,                         0x8E2E0C8, All)
DECLARE_GAME_LOG_CATEGORY(LogCloudEnumeration,                         0x8E2E038, All)
DECLARE_GAME_LOG_CATEGORY(LogCMSManager,                               0x8BA03D8, All)
DECLARE_GAME_LOG_CATEGORY(LogCodeGuard,                                0x8BC7B60, All)
DECLARE_GAME_LOG_CATEGORY(LogCollision,                                0x8E6FF70, All)
DECLARE_GAME_LOG_CATEGORY(LogCollisionProfile,                         0x8E6FC90, All)
DECLARE_GAME_LOG_CATEGORY(LogColorList,                                0x8CF77D0, All)
DECLARE_GAME_LOG_CATEGORY(LogCommonInput,                              0x8B9EBE8, All)
DECLARE_GAME_LOG_CATEGORY(LogCommonUI,                                 0x8B9ED00, All)
DECLARE_GAME_LOG_CATEGORY(LogCompression,                              0x8D267D0, All)
DECLARE_GAME_LOG_CATEGORY(LogConfig,                                   0x8D26838, All)
DECLARE_GAME_LOG_CATEGORY(LogConsoleManager,                           0x8CEEA20, All)
DECLARE_GAME_LOG_CATEGORY(LogConsoleResponse,                          0x8CEEA10, All)
DECLARE_GAME_LOG_CATEGORY(LogContentBeacon,                            0x8C2F1A8, All)
DECLARE_GAME_LOG_CATEGORY(LogContentComparisonCommandlet,              0x8D26AF8, All)
DECLARE_GAME_LOG_CATEGORY(LogContentControlsManager,                   0x8C2F320, All)
DECLARE_GAME_LOG_CATEGORY(LogContentEncryptionManager,                 0x8C125D8, All)
DECLARE_GAME_LOG_CATEGORY(LogContentStreaming,                         0x8E8F880, All)
DECLARE_GAME_LOG_CATEGORY(LogContextTutorial,                          0x8C07A40, All)
DECLARE_GAME_LOG_CATEGORY(LogController,                               0x8E71D50, All)
DECLARE_GAME_LOG_CATEGORY(LogControlRig,                               0x8BBCC00, All)
DECLARE_GAME_LOG_CATEGORY(LogCore,                                     0x8D272D0, All)
DECLARE_GAME_LOG_CATEGORY(LogCoreNet,                                  0x8D5C648, All)
DECLARE_GAME_LOG_CATEGORY(LogCoreSettings,                             0x8E71DB0, All)
DECLARE_GAME_LOG_CATEGORY(LogCosmeticAssetStreaming,                   0x8C13A30, All)
DECLARE_GAME_LOG_CATEGORY(LogCosmeticsVariants,                        0x8C13A20, All)
DECLARE_GAME_LOG_CATEGORY(LogCrashContext,                             0x8CECA80, All)
DECLARE_GAME_LOG_CATEGORY(LogCreativeBudgetTracker,                    0x8C1AC50, All)
DECLARE_GAME_LOG_CATEGORY(LogCreativeHUDContext,                       0x8ECB760, All)
DECLARE_GAME_LOG_CATEGORY(LogCreativeIslandLink,                       0x8ECD480, All)
DECLARE_GAME_LOG_CATEGORY(LogCreativeTest,                             0x8C45BD0, All)
DECLARE_GAME_LOG_CATEGORY(LogCrowdFollowing,                           0x8EBDF00, All)
DECLARE_GAME_LOG_CATEGORY(LogCsvProfiler,                              0x8D28578, All)
DECLARE_GAME_LOG_CATEGORY(LogCSVScoreboard,                            0x8C1BDA0, All)
DECLARE_GAME_LOG_CATEGORY(LogCurie,                                    0x8BB53E0, All)
DECLARE_GAME_LOG_CATEGORY(LogCurveTable,                               0x8E720D8, All)
DECLARE_GAME_LOG_CATEGORY(LogD3D11RHI,                                 0x8D6E2A0, All)
DECLARE_GAME_LOG_CATEGORY(LogD3D12GapRecorder,                         0x8D6ECF8, All)
DECLARE_GAME_LOG_CATEGORY(LogD3D12RHI,                                 0x8D6ECE8, All)
DECLARE_GAME_LOG_CATEGORY(LogDadBro,                                   0x8C044E0, All)
DECLARE_GAME_LOG_CATEGORY(LogDamage,                                   0x8E869E8, All)
DECLARE_GAME_LOG_CATEGORY(LogDataCompactifier,                         0x8E2DBA8, All)
DECLARE_GAME_LOG_CATEGORY(LogDataEnumeration,                          0x8E2DFF8, All)
DECLARE_GAME_LOG_CATEGORY(LogDataTable,                                0x8E72AD8, All)
DECLARE_GAME_LOG_CATEGORY(LogDebugRaycastCrash,                        0x8EA7B80, All)
DECLARE_GAME_LOG_CATEGORY(LogDeimosAIPerception,                       0x8BEB820, All)
DECLARE_GAME_LOG_CATEGORY(LogDeimosAvoidance,                          0x8BE9AD8, All)
DECLARE_GAME_LOG_CATEGORY(LogDeimosMove,                               0x8C2DE80, Warning)
DECLARE_GAME_LOG_CATEGORY(LogDeltaEnumeration,                         0x8E2E048, All)
DECLARE_GAME_LOG_CATEGORY(LogDemo,                                     0x8E72BF0, All)
DECLARE_GAME_LOG_CATEGORY(LogDeployableBase,                           0x8C22C88, All)
DECLARE_GAME_LOG_CATEGORY(LogDetectionML,                              0x8BE1960, All)
DECLARE_GAME_LOG_CATEGORY(LogDeviceProfile,                            0x8E72FD8, All)
DECLARE_GAME_LOG_CATEGORY(LogDevObjectVersion,                         0x8D33150, All)
DECLARE_GAME_LOG_CATEGORY(LogDiffManifests,                            0x8E2DFE8, All)
DECLARE_GAME_LOG_CATEGORY(LogDiscordRPC,                               0x8BBEF70, All)
DECLARE_GAME_LOG_CATEGORY(LogDiscoverability,                          0x8BF9EA0, All)
DECLARE_GAME_LOG_CATEGORY(LogDistanceField,                            0x8D95248, All)
DECLARE_GAME_LOG_CATEGORY(LogDistributions,                            0x8E73908, Warning)
DECLARE_GAME_LOG_CATEGORY(LogDownloadConnectionCount,                  0x8E2E0D8, All)
DECLARE_GAME_LOG_CATEGORY(LogDownloadService,                          0x8E2E0E8, All)
DECLARE_GAME_LOG_CATEGORY(LogDSSReplay,                                0x8BBF500, All)
DECLARE_GAME_LOG_CATEGORY(LogEasyAntiCheatClient,                      0x8EBD8D0, All)
DECLARE_GAME_LOG_CATEGORY(LogEasyAntiCheatNetComponent,                0x8EBD4F0, All)
DECLARE_GAME_LOG_CATEGORY(LogEdGraph,                                  0x8E74868, All)
DECLARE_GAME_LOG_CATEGORY(LogEditableMesh,                             0x8BD0770, All)
DECLARE_GAME_LOG_CATEGORY(LogEncryptedState,                           0x8C00418, All)
DECLARE_GAME_LOG_CATEGORY(LogEngine,                                   0x8E90C20, All)
DECLARE_GAME_LOG_CATEGORY(LogEngineService,                            0x8E749A8, All)
DECLARE_GAME_LOG_CATEGORY(LogEngineSessionManager,                     0x8E749B8, All)
DECLARE_GAME_LOG_CATEGORY(LogEngineUtils,                              0x8E74C30, All)
DECLARE_GAME_LOG_CATEGORY(LogEntityActor,                              0x8BCB6D0, All)
DECLARE_GAME_LOG_CATEGORY(LogEntityCore,                               0x8BCB9F0, All)
DECLARE_GAME_LOG_CATEGORY(LogEnum,                                     0x8D5C8B0, All)
DECLARE_GAME_LOG_CATEGORY(LogEOS,                                      0x8BA3328, All)
DECLARE_GAME_LOG_CATEGORY(LogEOSSDK,                                   0x8BC8C20, All)
DECLARE_GAME_LOG_CATEGORY(LogEOSVoiceChat,                             0x8BC8C30, All)
DECLARE_GAME_LOG_CATEGORY(LogEpicCMS,                                  0x8BA06B0, All)
DECLARE_GAME_LOG_CATEGORY(LogEpicCMSUIFramework,                       0x8BA0430, All)
DECLARE_GAME_LOG_CATEGORY(LogEpicGameplayStatsRuntime,                 0x8BCFE48, All)
DECLARE_GAME_LOG_CATEGORY(LogEQS,                                      0x8EBDD90, All)
DECLARE_GAME_LOG_CATEGORY(LogEventLeaderboard,                         0x8ECD588, All)
DECLARE_GAME_LOG_CATEGORY(LogExec,                                     0x8D27270, All)
DECLARE_GAME_LOG_CATEGORY(LogExit,                                     0x8D27260, All)
DECLARE_GAME_LOG_CATEGORY(LogExporter,                                 0x8E911B0, All)
DECLARE_GAME_LOG_CATEGORY(LogF5Player,                                 0x8BC8E90, All)
DECLARE_GAME_LOG_CATEGORY(LogF5PlayerFactory,                          0x8BCA138, All)
DECLARE_GAME_LOG_CATEGORY(LogF5PlayerStats,                            0x8BC8F20, All)
DECLARE_GAME_LOG_CATEGORY(LogFacialAnimSystem,                         0x8BBBD50, All)
DECLARE_GAME_LOG_CATEGORY(LogFileAttributesParser,                     0x8E2E068, All)
DECLARE_GAME_LOG_CATEGORY(LogFileManager,                              0x8CEF2E0, All)
DECLARE_GAME_LOG_CATEGORY(LogFileOperationTracker,                     0x8E2E1B0, All)
DECLARE_GAME_LOG_CATEGORY(LogFillFloorLTM,                             0x8C0A910, All)
DECLARE_GAME_LOG_CATEGORY(LogFloatPacker,                              0x8CF77E0, All)
DECLARE_GAME_LOG_CATEGORY(LogFontFace,                                 0x8E74C40, All)
DECLARE_GAME_LOG_CATEGORY(LogFort,                                     0x8C22A28, All)
DECLARE_GAME_LOG_CATEGORY(LogFortAbility,                              0x8C22B78, All)
DECLARE_GAME_LOG_CATEGORY(LogFortAI,                                   0x8C22A78, All)
DECLARE_GAME_LOG_CATEGORY(LogFortAIAssetLoader,                        0x8BE5E18, All)
DECLARE_GAME_LOG_CATEGORY(LogFortAIDebug,                              0x8BE5E60, All)
DECLARE_GAME_LOG_CATEGORY(LogFortAIDirector,                           0x8C22AB8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortAIGoalSelection,                      0x8C22A88, All)
DECLARE_GAME_LOG_CATEGORY(LogFortAnalytics,                            0x8BEC5F0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortAnalyticsEvent,                       0x8C22C18, All)
DECLARE_GAME_LOG_CATEGORY(LogFortAnimInstance,                         0x8BF4F38, All)
DECLARE_GAME_LOG_CATEGORY(LogFortAthenaAILOD,                          0x8BF9310, All)
DECLARE_GAME_LOG_CATEGORY(LogFortAthenaCustomTimeDilationManager,      0x8C029D0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortAthenaMutator_PerkSystemMutator,      0x8C05480, All)
DECLARE_GAME_LOG_CATEGORY(LogFortAthenaMutatorUtilities,               0x8C06D20, All)
DECLARE_GAME_LOG_CATEGORY(LogFortAwardManager,                         0x8C085C0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortBaseAnimInstance,                     0x8BF7468, All)
DECLARE_GAME_LOG_CATEGORY(LogFortBench,                                0x8C44D78, Log)
DECLARE_GAME_LOG_CATEGORY(LogFortBotMission,                           0x8BE9A40, All)
DECLARE_GAME_LOG_CATEGORY(LogFortBotTestState,                         0x8C0F560, All)
DECLARE_GAME_LOG_CATEGORY(LogFortBuilding,                             0x8C22A58, All)
DECLARE_GAME_LOG_CATEGORY(LogFortCharm,                                0x8BFF158, All)
DECLARE_GAME_LOG_CATEGORY(LogFortChat,                                 0x8C2F300, All)
DECLARE_GAME_LOG_CATEGORY(LogFortCheat,                                0x8C388C8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortCheckPointSplineActor,                0x8C079A8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortClientBot,                            0x8C1A5D0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortClientSmokeBot,                       0x8C1A618, All)
DECLARE_GAME_LOG_CATEGORY(LogFortCollectionBook,                       0x8C22C98, All)
DECLARE_GAME_LOG_CATEGORY(LogFortCollectionsComponent,                 0x8C104F0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortControllerComponent_Inspector,        0x8C11000, All)
DECLARE_GAME_LOG_CATEGORY(LogFortControllerComponent_PerkSystem,       0x8C06D30, All)
DECLARE_GAME_LOG_CATEGORY(LogFortControllerComponent_Portal,           0x8C113B0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortCosmeticLoadoutOverride,              0x8C04370, All)
DECLARE_GAME_LOG_CATEGORY(LogFortCosmetics,                            0x8C22B48, All)
DECLARE_GAME_LOG_CATEGORY(LogFortCreativeIslandExporter,               0x8C13E20, All)
DECLARE_GAME_LOG_CATEGORY(LogFortCreativeTags,                         0x8C13F58, All)
DECLARE_GAME_LOG_CATEGORY(LogFortCurieBuildingGrid,                    0x8C15D60, All)
DECLARE_GAME_LOG_CATEGORY(LogFortCurieComponent,                       0x8BB53D0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortCurieElectricityPropagationManager,   0x8C14FD0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortCurieFirePropagationManager,          0x8C159E0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortCurieGrassFireReplicator,             0x8C14A30, All)
DECLARE_GAME_LOG_CATEGORY(LogFortCurieGrassGrid,                       0x8C15D70, All)
DECLARE_GAME_LOG_CATEGORY(LogFortCurieSpatialManager,                  0x8C15D50, All)
DECLARE_GAME_LOG_CATEGORY(LogFortCustomCharacterParts,                 0x8C04428, All)
DECLARE_GAME_LOG_CATEGORY(LogFortCustomization,                        0x8C22B38, All)
DECLARE_GAME_LOG_CATEGORY(LogFortDamage,                               0x8C22C38, All)
DECLARE_GAME_LOG_CATEGORY(LogFortDayNight,                             0x8C22B18, All)
DECLARE_GAME_LOG_CATEGORY(LogFortDownloadLocalizedOverlays,            0x8C1BA50, All)
DECLARE_GAME_LOG_CATEGORY(LogFortEQS,                                  0x8C22AC8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortFactionCustomCharacterParts,          0x8C045D0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortFeedback,                             0x8C22B68, All)
DECLARE_GAME_LOG_CATEGORY(LogFortGameDifficulty,                       0x8C22AD8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortGameFeedback,                         0x8ECD5F8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortGameInstanceEmbeddedCommunication,    0x8C1BDD0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortGameplayTagData,                      0x8C1D068, All)
DECLARE_GAME_LOG_CATEGORY(LogFortGameUI,                               0x8C22AE8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortHealing,                              0x8C22C48, All)
DECLARE_GAME_LOG_CATEGORY(LogFortHero,                                 0x8C22AF8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortHitches,                              0x8C1D1B0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortHomebase,                             0x8C42000, All)
DECLARE_GAME_LOG_CATEGORY(LogFortHUDConverter,                         0x8C2DB68, All)
DECLARE_GAME_LOG_CATEGORY(LogFortHUDLayoutTool,                        0x8C22CD8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortHUDLayoutToolContext,                 0x8ECCA20, All)
DECLARE_GAME_LOG_CATEGORY(LogFortHUDMessage,                           0x8C13B00, All)
DECLARE_GAME_LOG_CATEGORY(LogFortIdle,                                 0x8C22CE8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortInfiltration,                         0x8C05120, All)
DECLARE_GAME_LOG_CATEGORY(LogFortInput,                                0x8C22C28, All)
DECLARE_GAME_LOG_CATEGORY(LogFortInventory,                            0x8C22A38, All)
DECLARE_GAME_LOG_CATEGORY(LogFortInventoryUI,                          0x8C22A48, All)
DECLARE_GAME_LOG_CATEGORY(LogFortItemsForEvent,                        0x8C22D18, All)
DECLARE_GAME_LOG_CATEGORY(LogFortKeyLockComponent,                     0x8C13E50, All)
DECLARE_GAME_LOG_CATEGORY(LogFortKnockback,                            0x8C22C58, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLevelSaveComponent,                   0x8C21AB0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLimitedLives,                         0x8C038D8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLinkDataManager,                      0x8C21EF0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLoadingScreen,                        0x8C22B08, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLocalization,                         0x8C22940, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLockDevice,                           0x8C13E40, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLogin,                                0x8ED2DB0, Verbose)
DECLARE_GAME_LOG_CATEGORY(LogFortLoot,                                 0x8C22BF8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLootQuota,                            0x8C22BE8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLTM,                                  0x8C045E0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLTMBismuth,                           0x8C038E8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLTMChrome,                            0x8C038F8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLTMCobalt,                            0x8C03908, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLTMCobaltCountdownWidget,             0x8EC7110, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLTMCobaltUI,                          0x8EC7120, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLTMNitrogen,                          0x8BB5C30, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLTMNitrogenCheat,                     0x8BB5C40, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLTMNitrogenPickup,                    0x8BB5C80, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLTMPayback,                           0x8C05470, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLTMRespawnAndSpectateSelectUI,        0x8EC7EB0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLTMTitanium,                          0x8C06120, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLTMUranium,                           0x8C06C80, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLTMUraniumUI,                         0x8EC7EC0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortLTMZinc,                              0x8C06D10, All)
DECLARE_GAME_LOG_CATEGORY(LogFortManifestHelper,                       0x8BBF7D0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortMantis,                               0x8BB89C0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortMemory,                               0x8C125E8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortMinigameManager,                      0x8C2C0D0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortMission,                              0x8C22B88, All)
DECLARE_GAME_LOG_CATEGORY(LogFortMissionGeneration,                    0x8C22B98, All)
DECLARE_GAME_LOG_CATEGORY(LogFortMissionRewards,                       0x8C22BA8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortMovementCompGroundSpline,             0x8C2E540, All)
DECLARE_GAME_LOG_CATEGORY(LogFortMovie,                                0x8ECFEC0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortMusic,                                0x8C22B28, All)
DECLARE_GAME_LOG_CATEGORY(LogFortMutatorCustomStorm,                   0x8C04438, All)
DECLARE_GAME_LOG_CATEGORY(LogFortMutatorInventoryOverride,             0x8C05180, All)
DECLARE_GAME_LOG_CATEGORY(LogFortNavigation,                           0x8ED0B30, All)
DECLARE_GAME_LOG_CATEGORY(LogFortniteAutomationHelper,                 0x8C45BC0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortniteBackgroundDownloadHelper,         0x8BBF8E8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortniteClientAutomationHelper,           0x8C44D88, All)
DECLARE_GAME_LOG_CATEGORY(LogFortniteCollctionBookAutomationTests,     0x8C43C90, All)
DECLARE_GAME_LOG_CATEGORY(LogFortniteCraftingAutomationTests,          0x8C43D90, All)
DECLARE_GAME_LOG_CATEGORY(LogFortniteCrowdControlAutomationTests,      0x8C43DA0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortniteDestructibleAutomationTests,      0x8C43DB0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortniteEarlyStartupPatcher,              0x8BBF8D8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortniteEngineLoadingScreen,              0x8CAA880, All)
DECLARE_GAME_LOG_CATEGORY(LogFortniteInventoryAutomationTests,         0x8C43DC0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortniteLlamaStoreAutomationTests,        0x8C43D60, All)
DECLARE_GAME_LOG_CATEGORY(LogFortnitePartyHubPreloadScreen,            0x8BBFBD8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortniteReplay,                           0x8EA8AF8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortniteTransmogAutomationTests,          0x8C43D70, All)
DECLARE_GAME_LOG_CATEGORY(LogFortniteVaultAutomationTests,             0x8C43D80, All)
DECLARE_GAME_LOG_CATEGORY(LogFortNitrogenFare,                         0x8BB5C50, All)
DECLARE_GAME_LOG_CATEGORY(LogFortOutpost,                              0x8C22BB8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortParachute,                            0x8C002D0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPawnScriptedBehavior,                 0x8C22D78, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPerformance,                          0x8C129D0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPet,                                  0x8C006F8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPhysics,                              0x8C22D28, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPickupManagement,                     0x8C11590, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPlacement,                            0x8C22BC8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPlayerFeedback,                       0x8ECFE90, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPlayerFeedbackModal,                  0x8ED7780, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPlayerPawn,                           0x8C370E0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPlayerPawnAthena,                     0x8C00408, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPlayerPawnAthenaMovement,             0x8BFEFD8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPlayerRegistration,                   0x8C22CA8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPlayerStartupController,              0x8C22E70, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPlayerSurvey,                         0x8ED85E0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPlayerSurveyCondition,                0x8ED85F0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPlaylistOptions,                      0x8C22CF8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPoiTracker,                           0x8C016A8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPortalComponent,                      0x8C24D50, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPostGamePlacementOverlay,             0x8EC6DC0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPostScreenRoot,                       0x8EC62E8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPrioSwitcher,                         0x8EC9118, All)
DECLARE_GAME_LOG_CATEGORY(LogFortProjectileAthena,                     0x8C016B8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPropertyOverrides,                    0x8C22D08, All)
DECLARE_GAME_LOG_CATEGORY(LogFortPunchCard,                            0x8EC5238, All)
DECLARE_GAME_LOG_CATEGORY(LogFortQuest,                                0x8C22BD8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortRechargeWeapons,                      0x8C113C0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortReplaySequence,                       0x8C3ED58, All)
DECLARE_GAME_LOG_CATEGORY(LogFortReplicationGraph,                     0x8C34260, All)
DECLARE_GAME_LOG_CATEGORY(LogFortRespawnAndSpectateSelect,             0x8C05F10, All)
DECLARE_GAME_LOG_CATEGORY(LogFortSaveGameReplay,                       0x8EA8C08, All)
DECLARE_GAME_LOG_CATEGORY(LogFortScreenshotForModal,                   0x8EDE1D0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortServerBanning,                        0x8C22CB8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortServerDebug,                          0x8C34550, All)
DECLARE_GAME_LOG_CATEGORY(LogFortSettings,                             0x8C1A780, All)
DECLARE_GAME_LOG_CATEGORY(LogFortShowPlacard,                          0x8C05F90, All)
DECLARE_GAME_LOG_CATEGORY(LogFortSignificance,                         0x8BE30D0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortSocial,                               0x8C34C80, All)
DECLARE_GAME_LOG_CATEGORY(LogFortSocialFlow,                           0x8ED2BF0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortSpectateBeforeRespawn,                0x8C05FD0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortSpectating,                           0x8C3B2D0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortSplineGroundPath,                     0x8C18608, All)
DECLARE_GAME_LOG_CATEGORY(LogFortSquadSlotManager,                     0x8EDD350, All)
DECLARE_GAME_LOG_CATEGORY(LogFortStore,                                0x8C22C68, All)
DECLARE_GAME_LOG_CATEGORY(LogFortStreamedMedia,                        0x8C24E98, All)
DECLARE_GAME_LOG_CATEGORY(LogFortSynchronizedTeleport,                 0x8C06010, All)
DECLARE_GAME_LOG_CATEGORY(LogFortTeamMemberUI,                         0x8EC4660, All)
DECLARE_GAME_LOG_CATEGORY(LogFortTeams,                                0x8C22C08, All)
DECLARE_GAME_LOG_CATEGORY(LogFortThreatVisuals,                        0x8C22A98, All)
DECLARE_GAME_LOG_CATEGORY(LogFortTooltip,                              0x8C22C78, All)
DECLARE_GAME_LOG_CATEGORY(LogFortTouchControls,                        0x8C22CC8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortUI,                                   0x8ECFED0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortUICameraFraming,                      0x8C4DCB0, All)
DECLARE_GAME_LOG_CATEGORY(LogFortUpdateManager,                        0x8C35A50, All)
DECLARE_GAME_LOG_CATEGORY(LogFortUserCloudHelper,                      0x8C258F8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortVoiceIndicators,                      0x8ED7770, All)
DECLARE_GAME_LOG_CATEGORY(LogFortVoicePopup,                           0x8ED0D50, All)
DECLARE_GAME_LOG_CATEGORY(LogFortVolumeManager,                        0x8C0E418, All)
DECLARE_GAME_LOG_CATEGORY(LogFortWeaponVariants,                       0x8C22B58, All)
DECLARE_GAME_LOG_CATEGORY(LogFortWind,                                 0x8C22AA8, All)
DECLARE_GAME_LOG_CATEGORY(LogFortWorld,                                0x8C22A68, All)
DECLARE_GAME_LOG_CATEGORY(LogFortWorldSettings,                        0x8C270A0, All)
DECLARE_GAME_LOG_CATEGORY(LogFoundationStreamingEventTracker,          0x8C0E430, All)
DECLARE_GAME_LOG_CATEGORY(LogFPhysScene_ChaosSolver,                   0x8E86F70, All)
DECLARE_GAME_LOG_CATEGORY(LogFramePacer,                               0x8D62608, All)
DECLARE_GAME_LOG_CATEGORY(LogGameFeedback,                             0x8BBA790, All)
DECLARE_GAME_LOG_CATEGORY(LogGameHeartbeat,                            0x8C40318, All)
DECLARE_GAME_LOG_CATEGORY(LogGameMode,                                 0x8E75FA0, All)
DECLARE_GAME_LOG_CATEGORY(LogGameNetworkManager,                       0x8E75FB0, All)
DECLARE_GAME_LOG_CATEGORY(LogGameplayBehavior,                         0x8BCC6A0, All)
DECLARE_GAME_LOG_CATEGORY(LogGameplayCueTranslator,                    0x8B9C928, All)
DECLARE_GAME_LOG_CATEGORY(LogGameplayEffects,                          0x8B9C808, All)
DECLARE_GAME_LOG_CATEGORY(LogGameplayTags,                             0x8E6BDF0, All)
DECLARE_GAME_LOG_CATEGORY(LogGameplayTasks,                            0x8EBD900, All)
DECLARE_GAME_LOG_CATEGORY(LogGameSession,                              0x8E75FE0, All)
DECLARE_GAME_LOG_CATEGORY(LogGameStartCountdown,                       0x8C045F0, All)
DECLARE_GAME_LOG_CATEGORY(LogGameState,                                0x8E76008, All)
DECLARE_GAME_LOG_CATEGORY(LogGameStatEvents,                           0x8C19240, All)
DECLARE_GAME_LOG_CATEGORY(LogGameStats,                                0x8C40368, All)
DECLARE_GAME_LOG_CATEGORY(LogGameSubCatalog,                           0x8BBA878, All)
DECLARE_GAME_LOG_CATEGORY(LogGarbage,                                  0x8D5C8C0, All)
DECLARE_GAME_LOG_CATEGORY(LogGatherArchiveObject,                      0x8CF4B00, All)
DECLARE_GAME_LOG_CATEGORY(LogGauntlet,                                 0x8BBFEA8, All)
DECLARE_GAME_LOG_CATEGORY(LogGenericOctree,                            0x8CF77F0, All)
DECLARE_GAME_LOG_CATEGORY(LogGenericPlatformMisc,                      0x8CEE9A0, All)
DECLARE_GAME_LOG_CATEGORY(LogGenericPlatformString,                    0x8CEE9D0, All)
DECLARE_GAME_LOG_CATEGORY(LogGenericPlatformSymbolication,             0x8CEE9E0, All)
DECLARE_GAME_LOG_CATEGORY(LogGenericPlatformTime,                      0x8CEE9F0, All)
DECLARE_GAME_LOG_CATEGORY(LogGenericPlatformWindow,                    0x8D625F8, All)
DECLARE_GAME_LOG_CATEGORY(LogGeoemtryCollectionClean,                  0x8E61388, All)
DECLARE_GAME_LOG_CATEGORY(LogGeometryCollectionAdapter,                0x8BD0760, All)
DECLARE_GAME_LOG_CATEGORY(LogGeometryCollectionCache,                  0x8E60138, All)
DECLARE_GAME_LOG_CATEGORY(LogGeometryCollectionClean,                  0x8E600F0, All)
DECLARE_GAME_LOG_CATEGORY(LogGeometryCollectionDebugDraw,              0x8E61378, All)
DECLARE_GAME_LOG_CATEGORY(LogGeometryCollectionDebugDrawActor,         0x8E60F90, All)
DECLARE_GAME_LOG_CATEGORY(LogGeomTools,                                0x8E765E8, All)
DECLARE_GAME_LOG_CATEGORY(LogGfeSDK,                                   0x8BC1930, All)
DECLARE_GAME_LOG_CATEGORY(LogGooglePAD,                                0x8BD3338, All)
DECLARE_GAME_LOG_CATEGORY(LogGooseLTM,                                 0x8C04608, All)
DECLARE_GAME_LOG_CATEGORY(LogGoosePickup,                              0x8BF9E90, All)
DECLARE_GAME_LOG_CATEGORY(LogGPUSort,                                  0x8E78010, All)
DECLARE_GAME_LOG_CATEGORY(LogGrass,                                    0x8E58590, All)
DECLARE_GAME_LOG_CATEGORY(LogGroomBuilder,                             0x8BCD260, All)
DECLARE_GAME_LOG_CATEGORY(LogHairRendering,                            0x8DAF998, All)
DECLARE_GAME_LOG_CATEGORY(LogHairStrands,                              0x8BCD208, All)
DECLARE_GAME_LOG_CATEGORY(LogHAL,                                      0x8D26AD8, All)
DECLARE_GAME_LOG_CATEGORY(LogHandshake,                                0x8E82CC0, All)
DECLARE_GAME_LOG_CATEGORY(LogHapticTest,                               0x8E75F60, All)
DECLARE_GAME_LOG_CATEGORY(LogHDRLoader,                                0x8E78628, All)
DECLARE_GAME_LOG_CATEGORY(LogHeadMountedDisplayCommand,                0x8E2C710, All)
DECLARE_GAME_LOG_CATEGORY(LogHealthSnapshot,                           0x8E882C0, All)
DECLARE_GAME_LOG_CATEGORY(LogHermesTest,                               0x8C45BE0, All)
DECLARE_GAME_LOG_CATEGORY(LogHighResScreenshot,                        0x8E7CCD0, All)
DECLARE_GAME_LOG_CATEGORY(LogHistograms,                               0x8D286F8, All)
DECLARE_GAME_LOG_CATEGORY(LogHittestDebug,                             0x8D63050, All)
DECLARE_GAME_LOG_CATEGORY(LogHLOD,                                     0x8E7DEF8, All)
DECLARE_GAME_LOG_CATEGORY(LogHMD,                                      0x8E2C850, All)
DECLARE_GAME_LOG_CATEGORY(LogHotfixManager,                            0x8BA1700, All)
DECLARE_GAME_LOG_CATEGORY(LogHttp,                                     0x8E2CF40, All)
DECLARE_GAME_LOG_CATEGORY(LogHttpConnection,                           0x8EA8F08, All)
DECLARE_GAME_LOG_CATEGORY(LogHttpConnectionResponseWriteContext,       0x8EA8F18, All)
DECLARE_GAME_LOG_CATEGORY(LogHttpListener,                             0x8EA8F28, All)
DECLARE_GAME_LOG_CATEGORY(LogHttpReplay,                               0x8E6DCA8, All)
DECLARE_GAME_LOG_CATEGORY(LogHttpSerialization,                        0x8BA3338, All)
DECLARE_GAME_LOG_CATEGORY(LogHttpServerConfig,                         0x8EA8F38, All)
DECLARE_GAME_LOG_CATEGORY(LogHttpServerModule,                         0x8EA8F48, All)
DECLARE_GAME_LOG_CATEGORY(LogHUD,                                      0x8E7CD00, All)
DECLARE_GAME_LOG_CATEGORY(LogHUDLayoutToolButtonLayer,                 0x8EC8D50, All)
DECLARE_GAME_LOG_CATEGORY(LogHUDLayoutToolSaving,                      0x8EC8D60, All)
DECLARE_GAME_LOG_CATEGORY(LogHUDWidgetAssetRegistryBuilder,            0x8ECAA80, All)
DECLARE_GAME_LOG_CATEGORY(LogIcmp,                                     0x8EA9638, All)
DECLARE_GAME_LOG_CATEGORY(LogICUInternationalization,                  0x8CF4AD8, All)
DECLARE_GAME_LOG_CATEGORY(LogIGMWidgets,                               0x8EC6220, All)
DECLARE_GAME_LOG_CATEGORY(LogImageDownload,                            0x8EA7560, All)
DECLARE_GAME_LOG_CATEGORY(LogImagePlateFileSequence,                   0x8BBFF68, Warning)
DECLARE_GAME_LOG_CATEGORY(LogImageUtils,                               0x8E7CD78, All)
DECLARE_GAME_LOG_CATEGORY(LogImageWrapper,                             0x8D6C7C0, All)
DECLARE_GAME_LOG_CATEGORY(LogImageWriteQueue,                          0x8D6F1D0, Warning)
DECLARE_GAME_LOG_CATEGORY(LogInfiltrationPerkSelectContainerWidget,    0x8EC7CC0, All)
DECLARE_GAME_LOG_CATEGORY(LogInfiltrationPerkSelectionPanel,           0x8EC7CE0, All)
DECLARE_GAME_LOG_CATEGORY(LogInfiltrationSpawnPoint,                   0x8C028B8, All)
DECLARE_GAME_LOG_CATEGORY(LogInGamePerformanceTracker,                 0x8E7CD88, All)
DECLARE_GAME_LOG_CATEGORY(LogInGameRewards,                            0x8ECFEF0, All)
DECLARE_GAME_LOG_CATEGORY(LogInit,                                     0x8D27250, All)
DECLARE_GAME_LOG_CATEGORY(LogInput,                                    0x8D5FF78, All)
DECLARE_GAME_LOG_CATEGORY(LogInstallBundleManager,                     0x8EA63B0, All)
DECLARE_GAME_LOG_CATEGORY(LogInstallChunkSource,                       0x8E2E180, All)
DECLARE_GAME_LOG_CATEGORY(LogInstanceCount,                            0x8D28708, All)
DECLARE_GAME_LOG_CATEGORY(LogInstancedFoliage,                         0x8E569C8, All)
DECLARE_GAME_LOG_CATEGORY(LogInteractiveProcess,                       0x8D277E0, All)
DECLARE_GAME_LOG_CATEGORY(LogInternationalizationManifestObject,       0x8CF4B10, All)
DECLARE_GAME_LOG_CATEGORY(LogInternationalizationMetadata,             0x8CF4B20, All)
DECLARE_GAME_LOG_CATEGORY(LogInterpToMovementComponent,                0x8E70750, All)
DECLARE_GAME_LOG_CATEGORY(LogIoDispatcher,                             0x8CF6BF0, All)
DECLARE_GAME_LOG_CATEGORY(LogIslandInspector,                          0x8C44D18, All)
DECLARE_GAME_LOG_CATEGORY(LogItemWrapOverride,                         0x8C115D8, All)
DECLARE_GAME_LOG_CATEGORY(LogJson,                                     0x8D5B5F0, All)
DECLARE_GAME_LOG_CATEGORY(LogKairos,                                   0x8BBB8D0, All)
DECLARE_GAME_LOG_CATEGORY(LogKairosAnalytics,                          0x8EA8AC8, All)
DECLARE_GAME_LOG_CATEGORY(LogKairosHeartbeat,                          0x8C40328, All)
DECLARE_GAME_LOG_CATEGORY(LogKeychainHelper,                           0x8BBA9E8, All)
DECLARE_GAME_LOG_CATEGORY(LogLandscape,                                0x8E56D90, All)
DECLARE_GAME_LOG_CATEGORY(LogLandscapeBP,                              0x8E56DA0, All)
DECLARE_GAME_LOG_CATEGORY(LogLatencyMarkers,                           0x8BC00D8, All)
DECLARE_GAME_LOG_CATEGORY(LogLauncherCheck,                            0x8EA6638, All)
DECLARE_GAME_LOG_CATEGORY(LogLauncherPlatform,                         0x8EA6628, All)
DECLARE_GAME_LOG_CATEGORY(LogLauncherSocial,                           0x8BC56C8, All)
DECLARE_GAME_LOG_CATEGORY(LogLauncherSocialChat,                       0x8BC56D8, All)
DECLARE_GAME_LOG_CATEGORY(LogLaunchWindows,                            0x8B9B3E0, All)
DECLARE_GAME_LOG_CATEGORY(LogLayoutUV,                                 0x8E6C5E0, All)
DECLARE_GAME_LOG_CATEGORY(LogLeakDetector,                             0x8E882F0, All)
DECLARE_GAME_LOG_CATEGORY(LogLevel,                                    0x8E7D9E0, All)
DECLARE_GAME_LOG_CATEGORY(LogLevelActorContainer,                      0x8E7DB00, All)
DECLARE_GAME_LOG_CATEGORY(LogLevelSaveRecord,                          0x8C2AFC0, All)
DECLARE_GAME_LOG_CATEGORY(LogLevelSequence,                            0x8E6D430, All)
DECLARE_GAME_LOG_CATEGORY(LogLevelStreaming,                           0x8E7DB10, All)
DECLARE_GAME_LOG_CATEGORY(LogLightMap,                                 0x8E7DB98, All)
DECLARE_GAME_LOG_CATEGORY(LogLinker,                                   0x8D5CCA0, All)
DECLARE_GAME_LOG_CATEGORY(LogLiveLink,                                 0x8BBBF50, All)
DECLARE_GAME_LOG_CATEGORY(LogLiveLinkComponents,                       0x8BBC700, All)
DECLARE_GAME_LOG_CATEGORY(LogLiveLinkCurveDebugUI,                     0x8BC0138, All)
DECLARE_GAME_LOG_CATEGORY(LogLiveLinkMessageBus,                       0x8EA94E8, All)
DECLARE_GAME_LOG_CATEGORY(LogLiveLinkMovieScene,                       0x8BBC7E0, All)
DECLARE_GAME_LOG_CATEGORY(LogLiveLinkRoles,                            0x8EA8FF0, All)
DECLARE_GAME_LOG_CATEGORY(LogLiveStreamAnimation,                      0x8BC0560, All)
DECLARE_GAME_LOG_CATEGORY(LogLiveStreamVoice,                          0x8BC03A8, All)
DECLARE_GAME_LOG_CATEGORY(LogLiveStreamVoiceEngine,                    0x8BC0398, All)
DECLARE_GAME_LOG_CATEGORY(LogLivingWorldManager,                       0x8BB59A0, All)
DECLARE_GAME_LOG_CATEGORY(LogLoad,                                     0x8D272C0, All)
DECLARE_GAME_LOG_CATEGORY(LogLoadingDev,                               0x8D5BD18, All)
DECLARE_GAME_LOG_CATEGORY(LogLoadingSplash,                            0x8E2C860, All)
DECLARE_GAME_LOG_CATEGORY(LogLobby,                                    0x8BA1310, All)
DECLARE_GAME_LOG_CATEGORY(LogLobbyBeacon,                              0x8BA1300, All)
DECLARE_GAME_LOG_CATEGORY(LogLocalFileReplay,                          0x8E6DC08, All)
DECLARE_GAME_LOG_CATEGORY(LogLocalization,                             0x8D27290, All)
DECLARE_GAME_LOG_CATEGORY(LogLocalNotificationModule,                  0x8EC1F48, All)
DECLARE_GAME_LOG_CATEGORY(LogLockFreeList,                             0x8CAB020, All)
DECLARE_GAME_LOG_CATEGORY(LogLoginFlow,                                0x8BA2028, All)
DECLARE_GAME_LOG_CATEGORY(LogLogitechLED,                              0x8BC0BD0, All)
DECLARE_GAME_LOG_CATEGORY(LogLongPackageNames,                         0x8D272A0, All)
DECLARE_GAME_LOG_CATEGORY(LogMagicLeapARPin,                           0x8BD2E40, All)
DECLARE_GAME_LOG_CATEGORY(LogMagicLeapSharedWorld,                     0x8BD3010, All)
DECLARE_GAME_LOG_CATEGORY(LogMallocFrameProfiler,                      0x8CF35D0, All)
DECLARE_GAME_LOG_CATEGORY(LogManagerInstanceTracker,                   0x8EBDB60, All)
DECLARE_GAME_LOG_CATEGORY(LogManifestBuilder,                          0x8E2E078, All)
DECLARE_GAME_LOG_CATEGORY(LogManifestBuildStreamer,                    0x8E2E088, All)
DECLARE_GAME_LOG_CATEGORY(LogManifestData,                             0x8E2DBB8, All)
DECLARE_GAME_LOG_CATEGORY(LogManifestUObject,                          0x8E2DFD8, All)
DECLARE_GAME_LOG_CATEGORY(LogMarkerComponent,                          0x8C028D8, All)
DECLARE_GAME_LOG_CATEGORY(LogMatchAnalytics,                           0x8C30D30, All)
DECLARE_GAME_LOG_CATEGORY(LogMatchmakingServiceClient,                 0x8BC0C98, All)
DECLARE_GAME_LOG_CATEGORY(LogMatchmakingServiceDedicatedServer,        0x8BC0D98, All)
DECLARE_GAME_LOG_CATEGORY(LogMaterial,                                 0x8E80900, Verbose)
DECLARE_GAME_LOG_CATEGORY(LogMaterialParameter,                        0x8E70770, All)
DECLARE_GAME_LOG_CATEGORY(LogMatinee,                                  0x8E7D0B0, All)
DECLARE_GAME_LOG_CATEGORY(LogMcpContext,                               0x8C322D0, All)
DECLARE_GAME_LOG_CATEGORY(LogMedia,                                    0x8ED7608, All)
DECLARE_GAME_LOG_CATEGORY(LogMediaAssets,                              0x8E6CDC8, All)
DECLARE_GAME_LOG_CATEGORY(LogMediaTimeSource,                          0x8E6CD38, All)
DECLARE_GAME_LOG_CATEGORY(LogMediaUtils,                               0x8E6CD48, All)
DECLARE_GAME_LOG_CATEGORY(LogMemory,                                   0x8D26B28, All)
DECLARE_GAME_LOG_CATEGORY(LogMemoryImage,                              0x8D29958, All)
DECLARE_GAME_LOG_CATEGORY(LogMergeManifests,                           0x8E2DB00, All)
DECLARE_GAME_LOG_CATEGORY(LogMeshBeacon,                               0x8BC0DF0, All)
DECLARE_GAME_LOG_CATEGORY(LogMeshDescription,                          0x8E6C260, All)
DECLARE_GAME_LOG_CATEGORY(LogMeshNetwork,                              0x8BC0E20, All)
DECLARE_GAME_LOG_CATEGORY(LogMetaData,                                 0x8D5CEF8, All)
DECLARE_GAME_LOG_CATEGORY(LogMFAModal,                                 0x8ED2CD0, All)
DECLARE_GAME_LOG_CATEGORY(LogMfMediaFactory,                           0x8BC16B8, All)
DECLARE_GAME_LOG_CATEGORY(LogMIDIDevice,                               0x8BC8150, All)
DECLARE_GAME_LOG_CATEGORY(LogMinigame,                                 0x8C2BED8, All)
DECLARE_GAME_LOG_CATEGORY(LogMobileInteraction,                        0x8C11320, All)
DECLARE_GAME_LOG_CATEGORY(LogMockMeshDataTracker,                      0x8E2B940, All)
DECLARE_GAME_LOG_CATEGORY(LogModel,                                    0x8E82188, All)
DECLARE_GAME_LOG_CATEGORY(LogModelComponent,                           0x8E821C0, All)
DECLARE_GAME_LOG_CATEGORY(LogModularGameplay,                          0x8BA8408, All)
DECLARE_GAME_LOG_CATEGORY(LogModuleManager,                            0x8D284E0, All)
DECLARE_GAME_LOG_CATEGORY(LogMotionControllerComponent,                0x8E2C890, All)
DECLARE_GAME_LOG_CATEGORY(LogMotionDelayBuffer,                        0x8E2C8E8, All)
DECLARE_GAME_LOG_CATEGORY(LogMotionTracking,                           0x8E2C908, All)
DECLARE_GAME_LOG_CATEGORY(LogMovement,                                 0x8E70780, All)
DECLARE_GAME_LOG_CATEGORY(LogMovieCapture,                             0x8E6CCA8, All)
DECLARE_GAME_LOG_CATEGORY(LogMoviePlayer,                              0x8E6DB48, All)
DECLARE_GAME_LOG_CATEGORY(LogMovieScene,                               0x8E5E7D8, All)
DECLARE_GAME_LOG_CATEGORY(LogMovieSceneCapture,                        0x8E6D7C0, All)
DECLARE_GAME_LOG_CATEGORY(LogMovieSceneRepl,                           0x8E5E800, All)
DECLARE_GAME_LOG_CATEGORY(LogMrMesh,                                   0x8E2B950, All)
DECLARE_GAME_LOG_CATEGORY(LogMultichannelTCP,                          0x8D6E168, All)
DECLARE_GAME_LOG_CATEGORY(LogMultiView,                                0x8E8AB18, All)
DECLARE_GAME_LOG_CATEGORY(LogMutatorAI,                                0x8C03490, Warning)
DECLARE_GAME_LOG_CATEGORY(LogMutatorGibson,                            0x8BA8E30, All)
DECLARE_GAME_LOG_CATEGORY(LogNavigation,                               0x8E6E430, All)
DECLARE_GAME_LOG_CATEGORY(LogNavigationDataBuild,                      0x8E6E440, All)
DECLARE_GAME_LOG_CATEGORY(LogNavigationDirtyArea,                      0x8EA7C40, All)
DECLARE_GAME_LOG_CATEGORY(LogNavigationPoint,                          0x8E821E0, All)
DECLARE_GAME_LOG_CATEGORY(LogNavLink,                                  0x8E6E450, All)
DECLARE_GAME_LOG_CATEGORY(LogNavMeshMovement,                          0x8E70260, All)
DECLARE_GAME_LOG_CATEGORY(LogNavOctree,                                0x8EA7C30, All)
DECLARE_GAME_LOG_CATEGORY(LogNavSysModule,                             0x8EA7D30, All)
DECLARE_GAME_LOG_CATEGORY(LogNeowiz,                                   0x8BCC328, All)
DECLARE_GAME_LOG_CATEGORY(LogNet,                                      0x8E72128, All)
DECLARE_GAME_LOG_CATEGORY(LogNetCore,                                  0x8E6B8B0, All)
DECLARE_GAME_LOG_CATEGORY(LogNetDormancy,                              0x8E72178, All)
DECLARE_GAME_LOG_CATEGORY(LogNetFastTArray,                            0x8E72188, All)
DECLARE_GAME_LOG_CATEGORY(LogNetPackageMap,                            0x8D26B08, All)
DECLARE_GAME_LOG_CATEGORY(LogNetPartialBunch,                          0x8E721A8, All)
DECLARE_GAME_LOG_CATEGORY(LogNetPlayerMovement,                        0x8E72148, All)
DECLARE_GAME_LOG_CATEGORY(LogNetSerialization,                         0x8D26B18, All)
DECLARE_GAME_LOG_CATEGORY(LogNetTraffic,                               0x8E72158, All)
DECLARE_GAME_LOG_CATEGORY(LogNetVersion,                               0x8D27BC0, All)
DECLARE_GAME_LOG_CATEGORY(LogNiagara,                                  0x8BAA100, Verbose)
DECLARE_GAME_LOG_CATEGORY(LogNiagaraShaderCompiler,                    0x8BB1DA0, All)
DECLARE_GAME_LOG_CATEGORY(LogNitrogenFareBonusWidget,                  0x8BB62B8, All)
DECLARE_GAME_LOG_CATEGORY(LogNitrogenFareMeterWidget,                  0x8BB62C8, All)
DECLARE_GAME_LOG_CATEGORY(LogNitrogenFareValueIndicator,               0x8BB5C60, All)
DECLARE_GAME_LOG_CATEGORY(LogNitrogenLeaderboardWidget,                0x8BB62D8, All)
DECLARE_GAME_LOG_CATEGORY(LogNitrogenMatchNearEndWidget,               0x8BB62E8, All)
DECLARE_GAME_LOG_CATEGORY(LogNitrogenObjectiveWidget,                  0x8BB62F8, All)
DECLARE_GAME_LOG_CATEGORY(LogNitrogenPassengerPawnComponent,           0x8BB5C70, All)
DECLARE_GAME_LOG_CATEGORY(LogNitrogenPlacementWidget,                  0x8BB6308, All)
DECLARE_GAME_LOG_CATEGORY(LogNitrogenTraversePoint,                    0x8BB5C90, All)
DECLARE_GAME_LOG_CATEGORY(LogNitrogenVehicleComponent,                 0x8BB5CA0, All)
DECLARE_GAME_LOG_CATEGORY(LogNitrogenWidgetBase,                       0x8BB6318, All)
DECLARE_GAME_LOG_CATEGORY(LogNullReplay,                               0x8E6DBF8, All)
DECLARE_GAME_LOG_CATEGORY(LogObj,                                      0x8D5CF08, All)
DECLARE_GAME_LOG_CATEGORY(LogOnline,                                   0x8BA2110, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineAccount,                            0x8BB90F0, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineAchievements,                       0x8BA21C0, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineChannels,                           0x8BA3348, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineChat,                               0x8BA2130, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineCloud,                              0x8BA21E0, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineContentControlsService,             0x8BA3358, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineDataStorageService,                 0x8BA6B08, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineEntitlement,                        0x8BA2200, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineEula,                               0x8BA6DE8, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineEvents,                             0x8BA2210, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineEventsService,                      0x8BA6E48, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineExternalUI,                         0x8BA21B0, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineFriend,                             0x8BA2180, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineGame,                               0x8BA2120, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineHandover,                           0x8BA7158, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineIdentity,                           0x8BA2190, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineInteractions,                       0x8BA7D58, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineLeaderboard,                        0x8BA21D0, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineLinks,                              0x8BA7168, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineMesh,                               0x8BA7178, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineMessageStorageService,              0x8BA74F8, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineParty,                              0x8BA2058, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlinePresence,                           0x8BA21A0, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlinePurchase,                           0x8BA2250, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineRtcp,                               0x8BA7608, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineSession,                            0x8BA2160, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineSharing,                            0x8BA2220, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineSocialBanService,                   0x8BA7618, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineStats,                              0x8BA2270, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineStore,                              0x8BA2230, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineStoreV2,                            0x8BA2240, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineTitleFile,                          0x8BA21F0, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineTournament,                         0x8BA2260, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineUser,                               0x8BA2170, All)
DECLARE_GAME_LOG_CATEGORY(LogOnlineVoice,                              0x8BA2150, All)
DECLARE_GAME_LOG_CATEGORY(LogOpenGL,                                   0x8E32C88, All)
DECLARE_GAME_LOG_CATEGORY(LogOptimisedDelta,                           0x8E2E190, All)
DECLARE_GAME_LOG_CATEGORY(LogOptionalProperty,                         0x8BCB528, All)
DECLARE_GAME_LOG_CATEGORY(LogOutputDevice,                             0x8D27BF0, All)
DECLARE_GAME_LOG_CATEGORY(LogOverlay,                                  0x8EA6018, All)
DECLARE_GAME_LOG_CATEGORY(LogPackageChunkData,                         0x8E2E098, All)
DECLARE_GAME_LOG_CATEGORY(LogPackageLocalizationCache,                 0x8D5BC40, All)
DECLARE_GAME_LOG_CATEGORY(LogPackageLocalizationManager,               0x8D5BC50, All)
DECLARE_GAME_LOG_CATEGORY(LogPackageName,                              0x8D5BC60, All)
DECLARE_GAME_LOG_CATEGORY(LogPakFile,                                  0x8E6C790, All)
DECLARE_GAME_LOG_CATEGORY(LogPaper2D,                                  0x8BA84C8, All)
DECLARE_GAME_LOG_CATEGORY(LogParticles,                                0x8E82F98, All)
DECLARE_GAME_LOG_CATEGORY(LogParty,                                    0x8BA0B18, All)
DECLARE_GAME_LOG_CATEGORY(LogPartyBeacon,                              0x8BA2750, All)
DECLARE_GAME_LOG_CATEGORY(LogPatchCheck,                               0x8BA1630, All)
DECLARE_GAME_LOG_CATEGORY(LogPatchGeneration,                          0x8E2DAF0, All)
DECLARE_GAME_LOG_CATEGORY(LogPath,                                     0x8E71D60, All)
DECLARE_GAME_LOG_CATEGORY(LogPathFollowing,                            0x8EBDFB0, All)
DECLARE_GAME_LOG_CATEGORY(LogPaths,                                    0x8D27C20, All)
DECLARE_GAME_LOG_CATEGORY(LogPawn,                                     0x8E869F8, All)
DECLARE_GAME_LOG_CATEGORY(LogPawnAction,                               0x8EBDAD0, All)
DECLARE_GAME_LOG_CATEGORY(LogPawnMovementComponent,                    0x8E707C8, All)
DECLARE_GAME_LOG_CATEGORY(LogPaybackHUDWidgetBase,                     0x8EC7E40, All)
DECLARE_GAME_LOG_CATEGORY(LogPBAC,                                     0x8BC77E0, All)
DECLARE_GAME_LOG_CATEGORY(LogPBDRigidsSolver,                          0x8E60618, All)
DECLARE_GAME_LOG_CATEGORY(LogPegasusDriver,                            0x8C3EB28, All)
DECLARE_GAME_LOG_CATEGORY(LogPerfCounters,                             0x8EA8F78, All)
DECLARE_GAME_LOG_CATEGORY(LogPerfReplayTest,                           0x8C44B38, All)
DECLARE_GAME_LOG_CATEGORY(LogPerkAvailableAlertBaseWidget,             0x8EC7E50, All)
DECLARE_GAME_LOG_CATEGORY(LogPerkIntroScreenWidget,                    0x8EC7E60, All)
DECLARE_GAME_LOG_CATEGORY(LogPerkSelectionEntryWidget,                 0x8EC7E70, All)
DECLARE_GAME_LOG_CATEGORY(LogPhysicalMaterialMask,                     0x8E87740, All)
DECLARE_GAME_LOG_CATEGORY(LogPhysics,                                  0x8E86D90, All)
DECLARE_GAME_LOG_CATEGORY(LogPhysicsAsset,                             0x8BCEE68, All)
DECLARE_GAME_LOG_CATEGORY(LogPhysicsCore,                              0x8E2B7A0, All)
DECLARE_GAME_LOG_CATEGORY(LogPhysicsPawn,                              0x8BD50C0, All)
DECLARE_GAME_LOG_CATEGORY(LogPlanarCut,                                0x8BD06C0, All)
DECLARE_GAME_LOG_CATEGORY(LogPlatformCryptoOpenSSL,                    0x8BBB2D8, All)
DECLARE_GAME_LOG_CATEGORY(LogPlatformFile,                             0x8CF2A28, All)
DECLARE_GAME_LOG_CATEGORY(LogPlatformFileManagedStorage,               0x8CF2A38, All)
DECLARE_GAME_LOG_CATEGORY(LogPlayerCameraManager,                      0x8E87980, All)
DECLARE_GAME_LOG_CATEGORY(LogPlayerController,                         0x8E87A40, All)
DECLARE_GAME_LOG_CATEGORY(LogPlayerManagement,                         0x8E7DD58, All)
DECLARE_GAME_LOG_CATEGORY(LogPlayerQuestProgress,                      0x8BFF338, All)
DECLARE_GAME_LOG_CATEGORY(LogPlaysetLevelStream,                       0x8C3C5A0, All)
DECLARE_GAME_LOG_CATEGORY(LogPlayspaces,                               0x8BC4770, All)
DECLARE_GAME_LOG_CATEGORY(LogPlayTimeLimit,                            0x8BA1CE0, All)
DECLARE_GAME_LOG_CATEGORY(LogPluginCommandlet,                         0x8E70130, All)
DECLARE_GAME_LOG_CATEGORY(LogPluginManager,                            0x8D5B630, All)
DECLARE_GAME_LOG_CATEGORY(LogPolygon,                                  0x8E87CD0, All)
DECLARE_GAME_LOG_CATEGORY(LogPostmaster,                               0x8BA7F88, All)
DECLARE_GAME_LOG_CATEGORY(LogPreLoadScreenManager,                     0x8E2E588, All)
DECLARE_GAME_LOG_CATEGORY(LogPrerequisites,                            0x8E2E1A0, All)
DECLARE_GAME_LOG_CATEGORY(LogPressureGrid,                             0x8BCEFB0, All)
DECLARE_GAME_LOG_CATEGORY(LogPrimitiveComponent,                       0x8E70888, All)
DECLARE_GAME_LOG_CATEGORY(LogProcedural,                               0x8BCC950, All)
DECLARE_GAME_LOG_CATEGORY(LogProceduralComponent,                      0x8BC4D70, All)
DECLARE_GAME_LOG_CATEGORY(LogProcess,                                  0x8D272B0, All)
DECLARE_GAME_LOG_CATEGORY(LogProfileMocking,                           0x8BBA3D0, All)
DECLARE_GAME_LOG_CATEGORY(LogProfileSys,                               0x8BBA3E0, All)
DECLARE_GAME_LOG_CATEGORY(LogProfileUtils,                             0x8C45C00, All)
DECLARE_GAME_LOG_CATEGORY(LogProfilingBlueprintLibrary,                0x8C3C6A8, All)
DECLARE_GAME_LOG_CATEGORY(LogProfilingDebugging,                       0x8D26B38, All)
DECLARE_GAME_LOG_CATEGORY(LogProjectileMovement,                       0x8E70B28, All)
DECLARE_GAME_LOG_CATEGORY(LogProjectManager,                           0x8D5B640, All)
DECLARE_GAME_LOG_CATEGORY(LogProperty,                                 0x8D5D208, All)
DECLARE_GAME_LOG_CATEGORY(LogProtoLem,                                 0x8EC0268, All)
DECLARE_GAME_LOG_CATEGORY(LogPurchaseFlow,                             0x8BC4F48, All)
DECLARE_GAME_LOG_CATEGORY(LogQos,                                      0x8BA0770, All)
DECLARE_GAME_LOG_CATEGORY(LogQuadTree,                                 0x8E76298, Warning)
DECLARE_GAME_LOG_CATEGORY(LogRazerChroma,                              0x8BC5350, All)
DECLARE_GAME_LOG_CATEGORY(LogRBAN,                                     0x8E61AE0, Warning)
DECLARE_GAME_LOG_CATEGORY(LogRecast,                                   0x8EA7B98, All)
DECLARE_GAME_LOG_CATEGORY(LogReferenceChain,                           0x8D5D430, All)
DECLARE_GAME_LOG_CATEGORY(LogReflectionCaptureComponent,               0x8E70BF0, All)
DECLARE_GAME_LOG_CATEGORY(LogRegisterWithMutatorComponent,             0x8C078B0, All)
DECLARE_GAME_LOG_CATEGORY(LogRenderer,                                 0x8DFC558, All)
DECLARE_GAME_LOG_CATEGORY(LogRendererCore,                             0x8E47C98, All)
DECLARE_GAME_LOG_CATEGORY(LogRenderTargetPool,                         0x8E481C8, All)
DECLARE_GAME_LOG_CATEGORY(LogRep,                                      0x8E72138, All)
DECLARE_GAME_LOG_CATEGORY(LogReplayExtraction,                         0x8C3EC08, All)
DECLARE_GAME_LOG_CATEGORY(LogReplicationGraph,                         0x8BC13E8, All)
DECLARE_GAME_LOG_CATEGORY(LogRepProperties,                            0x8E89370, Warning)
DECLARE_GAME_LOG_CATEGORY(LogRepPropertiesBackCompat,                  0x8E89380, Warning)
DECLARE_GAME_LOG_CATEGORY(LogRepTraffic,                               0x8E72168, All)
DECLARE_GAME_LOG_CATEGORY(LogResonanceAudio,                           0x8BCBF40, All)
DECLARE_GAME_LOG_CATEGORY(LogRespawnAndSpectateSelectButtonWidgetBase, 0x8EC7E90, All)
DECLARE_GAME_LOG_CATEGORY(LogRespawnAndSpectateSelectContainerWidget,  0x8EC7EA0, All)
DECLARE_GAME_LOG_CATEGORY(LogRespawnStateGooseLTM,                     0x8C04618, All)
DECLARE_GAME_LOG_CATEGORY(LogRHI,                                      0x8E367F0, All)
DECLARE_GAME_LOG_CATEGORY(LogRigUnit,                                  0x8BBCD50, All)
DECLARE_GAME_LOG_CATEGORY(LogRigVM,                                    0x8F0A268, All)
DECLARE_GAME_LOG_CATEGORY(LogRootMotion,                               0x8E6E620, All)
DECLARE_GAME_LOG_CATEGORY(LogRoundHelper,                              0x8C05F30, All)
DECLARE_GAME_LOG_CATEGORY(LogRudderControl,                            0x8BD0200, All)
DECLARE_GAME_LOG_CATEGORY(LogRuntimeOptionsBase,                       0x8E8A758, All)
DECLARE_GAME_LOG_CATEGORY(LogSaveGameReplay,                           0x8EA8B48, All)
DECLARE_GAME_LOG_CATEGORY(LogSavePackage,                              0x8D5D440, All)
DECLARE_GAME_LOG_CATEGORY(LogSceneComponent,                           0x8E70CB0, All)
DECLARE_GAME_LOG_CATEGORY(LogScheduledEvents,                          0x8BBACC8, All)
DECLARE_GAME_LOG_CATEGORY(LogScript,                                   0x8D27280, All)
DECLARE_GAME_LOG_CATEGORY(LogScriptCore,                               0x8D5F098, All)
DECLARE_GAME_LOG_CATEGORY(LogScriptFrame,                              0x8D5F088, All)
DECLARE_GAME_LOG_CATEGORY(LogScriptPlatformInterface,                  0x8E8B688, All)
DECLARE_GAME_LOG_CATEGORY(LogScriptSerialization,                      0x8D5C618, All)
DECLARE_GAME_LOG_CATEGORY(LogScriptStackTracker,                       0x8D5F160, All)
DECLARE_GAME_LOG_CATEGORY(LogSecureHash,                               0x8D284A0, All)
DECLARE_GAME_LOG_CATEGORY(LogSecurity,                                 0x8E72198, All)
DECLARE_GAME_LOG_CATEGORY(LogSelection,                                0x8E8B698, All)
DECLARE_GAME_LOG_CATEGORY(LogSerialization,                            0x8D26AE8, All)
DECLARE_GAME_LOG_CATEGORY(LogSHA,                                      0x8D27220, All)
DECLARE_GAME_LOG_CATEGORY(LogShaderCompilers,                          0x8E8B6A8, All)
DECLARE_GAME_LOG_CATEGORY(LogShaderLibrary,                            0x8E528A0, All)
DECLARE_GAME_LOG_CATEGORY(LogShaders,                                  0x8E52858, All)
DECLARE_GAME_LOG_CATEGORY(LogShaderWarnings,                           0x8E52868, Log)
DECLARE_GAME_LOG_CATEGORY(LogSidecar,                                  0x8BBAFE8, All)
DECLARE_GAME_LOG_CATEGORY(LogSidecarDss,                               0x8BBAD70, All)
DECLARE_GAME_LOG_CATEGORY(LogSidecarInventory,                         0x8BBAE78, All)
DECLARE_GAME_LOG_CATEGORY(LogSignalProcessing,                         0x8E6ACB0, All)
DECLARE_GAME_LOG_CATEGORY(LogSignificance,                             0x8BBB7D0, All)
DECLARE_GAME_LOG_CATEGORY(LogSkeletalControl,                          0x8E6F810, All)
DECLARE_GAME_LOG_CATEGORY(LogSkeletalGPUSkinMesh,                      0x8E8E0A0, All)
DECLARE_GAME_LOG_CATEGORY(LogSkeletalMesh,                             0x8E8CD00, All)
DECLARE_GAME_LOG_CATEGORY(LogSkeletalMeshEditorData,                   0x8E8E038, All)
DECLARE_GAME_LOG_CATEGORY(LogSkeletalMeshLODSettings,                  0x8E8E058, All)
DECLARE_GAME_LOG_CATEGORY(LogSkinCache,                                0x8E765F8, All)
DECLARE_GAME_LOG_CATEGORY(LogSkinnedMeshComp,                          0x8E70F20, All)
DECLARE_GAME_LOG_CATEGORY(LogSkinWeightProfileManager,                 0x8E6EE80, Display)
DECLARE_GAME_LOG_CATEGORY(LogSlate,                                    0x8D664E0, All)
DECLARE_GAME_LOG_CATEGORY(LogSlateInvalidationPanel,                   0x8D6DD10, All)
DECLARE_GAME_LOG_CATEGORY(LogSlateSoundDevice,                         0x8E8EA20, All)
DECLARE_GAME_LOG_CATEGORY(LogSlateStyle,                               0x8D665D8, All)
DECLARE_GAME_LOG_CATEGORY(LogSlateStyles,                              0x8D664F0, All)
DECLARE_GAME_LOG_CATEGORY(LogSmartObject,                              0x8BCC3E8, All)
DECLARE_GAME_LOG_CATEGORY(LogSmartObjectManager,                       0x8BCC3C8, All)
DECLARE_GAME_LOG_CATEGORY(LogSnippetView,                              0x8EC0278, All)
DECLARE_GAME_LOG_CATEGORY(LogSocialUMG,                                0x8BC6858, All)
DECLARE_GAME_LOG_CATEGORY(LogSockets,                                  0x8D6E178, All)
DECLARE_GAME_LOG_CATEGORY(LogSolaris,                                  0x8BCAE38, All)
DECLARE_GAME_LOG_CATEGORY(LogSolarisIde,                               0x8BCAE28, All)
DECLARE_GAME_LOG_CATEGORY(LogSolarisInterop,                           0x8BCB538, All)
DECLARE_GAME_LOG_CATEGORY(LogSolarisRuntime,                           0x8BCAE48, All)
DECLARE_GAME_LOG_CATEGORY(LogSolarisUi,                                0x8BCB640, All)
DECLARE_GAME_LOG_CATEGORY(LogSolGeneratedClass,                        0x8BCAE18, All)
DECLARE_GAME_LOG_CATEGORY(LogSoundNodeGroupControl,                    0x8E8EB50, All)
DECLARE_GAME_LOG_CATEGORY(LogSpawn,                                    0x8E94240, All)
DECLARE_GAME_LOG_CATEGORY(LogSpecialEvent,                             0x8C05FA0, All)
DECLARE_GAME_LOG_CATEGORY(LogSpectatorBeacon,                          0x8BA2778, All)
DECLARE_GAME_LOG_CATEGORY(LogSpeechGraphics,                           0x8BBBE80, All)
DECLARE_GAME_LOG_CATEGORY(LogSpeechGraphicsCore,                       0x8BBBE90, All)
DECLARE_GAME_LOG_CATEGORY(LogSpyRumble,                                0x8C05FF0, All)
DECLARE_GAME_LOG_CATEGORY(LogSpyTechGrantItem,                         0x8BE4FD8, All)
DECLARE_GAME_LOG_CATEGORY(LogSpyTechMysteryGun,                        0x8BE4FE8, All)
DECLARE_GAME_LOG_CATEGORY(LogSquadStart,                               0x8C07A28, All)
DECLARE_GAME_LOG_CATEGORY(LogSsl,                                      0x8E2CED0, All)
DECLARE_GAME_LOG_CATEGORY(LogStackTracker,                             0x8CB3188, All)
DECLARE_GAME_LOG_CATEGORY(LogStaticMesh,                               0x8E8ED00, All)
DECLARE_GAME_LOG_CATEGORY(LogStaticMeshOperations,                     0x8E6C698, All)
DECLARE_GAME_LOG_CATEGORY(LogStats,                                    0x8D27230, All)
DECLARE_GAME_LOG_CATEGORY(LogStomp,                                    0x8EA7480, All)
DECLARE_GAME_LOG_CATEGORY(LogStreamableManager,                        0x8E8F358, All)
DECLARE_GAME_LOG_CATEGORY(LogStreaming,                                0x8D27240, All)
DECLARE_GAME_LOG_CATEGORY(LogStreamingFileCache,                       0x8CB3740, All)
DECLARE_GAME_LOG_CATEGORY(LogStreamingRadioPlayer,                     0x8BB8DD0, All)
DECLARE_GAME_LOG_CATEGORY(LogStringTable,                              0x8CF6AB8, All)
DECLARE_GAME_LOG_CATEGORY(LogStringUtility,                            0x8D284B0, All)
DECLARE_GAME_LOG_CATEGORY(LogStructQuery,                              0x8BE2570, All)
DECLARE_GAME_LOG_CATEGORY(LogSubsurfaceProfile,                        0x8E89328, All)
DECLARE_GAME_LOG_CATEGORY(LogSubtitle,                                 0x8E8FBD0, All)
DECLARE_GAME_LOG_CATEGORY(LogSynthBenchmark,                           0x8D625E8, All)
DECLARE_GAME_LOG_CATEGORY(LogSynthesis,                                0x8BB7090, All)
DECLARE_GAME_LOG_CATEGORY(LogSystemSettings,                           0x8E8FBE0, All)
DECLARE_GAME_LOG_CATEGORY(LogTabManager,                               0x8D6CCA8, All)
DECLARE_GAME_LOG_CATEGORY(LogTapDelay,                                 0x8BB7080, All)
DECLARE_GAME_LOG_CATEGORY(LogTaskGraph,                                0x8CAAE00, All)
DECLARE_GAME_LOG_CATEGORY(LogTeamPedestal,                             0x8ED1CC0, Verbose)
DECLARE_GAME_LOG_CATEGORY(LogTemp,                                     0x8D26B48, All)
DECLARE_GAME_LOG_CATEGORY(LogTestAI,                                   0x8EBDB10, All)
DECLARE_GAME_LOG_CATEGORY(LogText,                                     0x8CF6AC8, All)
DECLARE_GAME_LOG_CATEGORY(LogTextFormatter,                            0x8CF6AD8, All)
DECLARE_GAME_LOG_CATEGORY(LogTextHistory,                              0x8CF6AE8, All)
DECLARE_GAME_LOG_CATEGORY(LogTextKey,                                  0x8CF6AF8, All)
DECLARE_GAME_LOG_CATEGORY(LogTextLocalizationManager,                  0x8CF6B08, All)
DECLARE_GAME_LOG_CATEGORY(LogTextLocalizationResource,                 0x8CF6B18, All)
DECLARE_GAME_LOG_CATEGORY(LogTextStoreACP,                             0x8D62628, All)
DECLARE_GAME_LOG_CATEGORY(LogTexture,                                  0x8E8FC38, All)
DECLARE_GAME_LOG_CATEGORY(LogTheaterMap,                               0x8C43E98, All)
DECLARE_GAME_LOG_CATEGORY(LogThreadingWindows,                         0x8D59D80, All)
DECLARE_GAME_LOG_CATEGORY(LogTick,                                     0x8E8FDE8, All)
DECLARE_GAME_LOG_CATEGORY(LogTileMenu,                                 0x8EC0258, All)
DECLARE_GAME_LOG_CATEGORY(LogTiles,                                    0x8EC0178, All)
DECLARE_GAME_LOG_CATEGORY(LogTimeGuard,                                0x8D284C0, All)
DECLARE_GAME_LOG_CATEGORY(LogTimeline,                                 0x8E8FFC0, All)
DECLARE_GAME_LOG_CATEGORY(LogTimeSynth,                                0x8BC7E70, All)
DECLARE_GAME_LOG_CATEGORY(LogTitaniumPlayerComponent,                  0x8C028C8, All)
DECLARE_GAME_LOG_CATEGORY(LogTournamentDataCache,                      0x8C35750, All)
DECLARE_GAME_LOG_CATEGORY(LogTournamentElimExtraction,                 0x8C1BDC0, All)
DECLARE_GAME_LOG_CATEGORY(LogTournamentManager,                        0x8C35910, All)
DECLARE_GAME_LOG_CATEGORY(LogTournamentManagerTypes,                   0x8C35940, All)
DECLARE_GAME_LOG_CATEGORY(LogTournamentStats,                          0x8BFD050, All)
DECLARE_GAME_LOG_CATEGORY(LogTransform,                                0x8CF7800, All)
DECLARE_GAME_LOG_CATEGORY(LogTraverseLTM,                              0x8C06C70, All)
DECLARE_GAME_LOG_CATEGORY(LogType,                                     0x8D5F1A8, All)
DECLARE_GAME_LOG_CATEGORY(LogUAC,                                      0x8BC7BF0, All)
DECLARE_GAME_LOG_CATEGORY(LogUACNet,                                   0x8BC7CD0, All)
DECLARE_GAME_LOG_CATEGORY(LogUdpMessaging,                             0x8BC7D78, All)
DECLARE_GAME_LOG_CATEGORY(LogUHeadMountedDisplay,                      0x8E2C840, All)
DECLARE_GAME_LOG_CATEGORY(LogUIActionRouter,                           0x8B9EFC0, All)
DECLARE_GAME_LOG_CATEGORY(LogUIManagerSplit,                           0x8EDE2C8, All)
DECLARE_GAME_LOG_CATEGORY(LogUMG,                                      0x8E67790, All)
DECLARE_GAME_LOG_CATEGORY(LogUnicornDriver,                            0x8C48F48, All)
DECLARE_GAME_LOG_CATEGORY(LogUnicornLegacy,                            0x8C1BDB0, All)
DECLARE_GAME_LOG_CATEGORY(LogUnicornLogStream,                         0x8C491A0, All)
DECLARE_GAME_LOG_CATEGORY(LogUnicornPawnSampler,                       0x8C48F38, All)
DECLARE_GAME_LOG_CATEGORY(LogUnicornWebAPIComponent,                   0x8C49DA0, All)
DECLARE_GAME_LOG_CATEGORY(LogUnion,                                    0x8CB36A0, All)
DECLARE_GAME_LOG_CATEGORY(LogUnrealMath,                               0x8CF78D0, All)
DECLARE_GAME_LOG_CATEGORY(LogUnrealNames,                              0x8D33300, All)
DECLARE_GAME_LOG_CATEGORY(LogUObjectAllocator,                         0x8D5F1B8, All)
DECLARE_GAME_LOG_CATEGORY(LogUObjectArray,                             0x8D5F1E8, All)
DECLARE_GAME_LOG_CATEGORY(LogUObjectBase,                              0x8D5F220, All)
DECLARE_GAME_LOG_CATEGORY(LogUObjectBootstrap,                         0x8D5F230, Display)
DECLARE_GAME_LOG_CATEGORY(LogUObjectGlobals,                           0x8D5F2B0, All)
DECLARE_GAME_LOG_CATEGORY(LogUObjectHash,                              0x8D5F520, All)
DECLARE_GAME_LOG_CATEGORY(LogUObjectLinker,                            0x8D5FA78, All)
DECLARE_GAME_LOG_CATEGORY(LogUObjectThreadContext,                     0x8D5FA88, All)
DECLARE_GAME_LOG_CATEGORY(LogUProjectInfo,                             0x8D284D0, All)
DECLARE_GAME_LOG_CATEGORY(LogVectorField,                              0x8E916C0, Error)
DECLARE_GAME_LOG_CATEGORY(LogVectorVM,                                 0x8EBCD28, All)
DECLARE_GAME_LOG_CATEGORY(LogVehiclePhysics,                           0x8C26340, All)
DECLARE_GAME_LOG_CATEGORY(LogVehicles,                                 0x8BC43E0, All)
DECLARE_GAME_LOG_CATEGORY(LogVehicleTricks,                            0x8C3C4A0, All)
DECLARE_GAME_LOG_CATEGORY(LogVelocityGrid,                             0x8BCF0F8, All)
DECLARE_GAME_LOG_CATEGORY(LogVerifier,                                 0x8E2E1C0, All)
DECLARE_GAME_LOG_CATEGORY(LogVerifyChunkData,                          0x8E2DB98, All)
DECLARE_GAME_LOG_CATEGORY(LogVideoExtractionBootstrapper,              0x8C3ECD0, All)
DECLARE_GAME_LOG_CATEGORY(LogViewport,                                 0x8E8E348, All)
DECLARE_GAME_LOG_CATEGORY(LogVirtualTextureSpace,                      0x8E21048, All)
DECLARE_GAME_LOG_CATEGORY(LogVirtualTexturing,                         0x8E56708, All)
DECLARE_GAME_LOG_CATEGORY(LogVisual,                                   0x8E93CA0, All)
DECLARE_GAME_LOG_CATEGORY(LogVivoxVoiceChat,                           0x8BC8920, All)
DECLARE_GAME_LOG_CATEGORY(LogVoice,                                    0x8EA71F0, All)
DECLARE_GAME_LOG_CATEGORY(LogVoiceCapture,                             0x8EA7220, All)
DECLARE_GAME_LOG_CATEGORY(LogVoiceChannelView,                         0x8ED2C20, All)
DECLARE_GAME_LOG_CATEGORY(LogVoiceChatManager,                         0x8BC8420, All)
DECLARE_GAME_LOG_CATEGORY(LogVoiceDecode,                              0x8EA7210, All)
DECLARE_GAME_LOG_CATEGORY(LogVoiceEncode,                              0x8EA7200, All)
DECLARE_GAME_LOG_CATEGORY(LogVoiceEngine,                              0x8BA2140, All)
DECLARE_GAME_LOG_CATEGORY(LogVolume,                                   0x8E93DB0, All)
DECLARE_GAME_LOG_CATEGORY(LogVoting,                                   0x8BC4AB8, All)
DECLARE_GAME_LOG_CATEGORY(LogVulkan,                                   0x8E2EBE0, All)
DECLARE_GAME_LOG_CATEGORY(LogVulkanRHI,                                0x8E2E7B8, All)
DECLARE_GAME_LOG_CATEGORY(LogWater,                                    0x8BA91F0, All)
DECLARE_GAME_LOG_CATEGORY(LogWeakObjectPtr,                            0x8D5FA98, All)
DECLARE_GAME_LOG_CATEGORY(LogWebAuth,                                  0x8BA8230, All)
DECLARE_GAME_LOG_CATEGORY(LogWebBrowser,                               0x8EA87D0, All)
DECLARE_GAME_LOG_CATEGORY(LogWebSockets,                               0x8EA72E0, All)
DECLARE_GAME_LOG_CATEGORY(LogWindows,                                  0x8D26B58, All)
DECLARE_GAME_LOG_CATEGORY(LogWindowsDesktop,                           0x8D62638, All)
DECLARE_GAME_LOG_CATEGORY(LogWindowsTextInputMethodSystem,             0x8D62708, All)
DECLARE_GAME_LOG_CATEGORY(LogWinDualShock,                             0x8BC7DE8, All)
DECLARE_GAME_LOG_CATEGORY(LogWinHttp,                                  0x8E2CFE0, All)
DECLARE_GAME_LOG_CATEGORY(LogWmfMedia,                                 0x8BCA908, All)
DECLARE_GAME_LOG_CATEGORY(LogWmfMediaFactory,                          0x8BCADC8, All)
DECLARE_GAME_LOG_CATEGORY(LogWorld,                                    0x8E94230, All)
DECLARE_GAME_LOG_CATEGORY(LogWorldComposition,                         0x8E94690, All)
DECLARE_GAME_LOG_CATEGORY(LogWorldSettings,                            0x8E946C0, All)
DECLARE_GAME_LOG_CATEGORY(LogXAudio2,                                  0x8EA7130, All)
DECLARE_GAME_LOG_CATEGORY(LogXmpp,                                     0x8EA7328, All)
DECLARE_GAME_LOG_CATEGORY(LogZeroLoad,                                 0x8EA8FB8, All)
DECLARE_GAME_LOG_CATEGORY(LSR_LOG,                                     0x8E613A8, All)
DECLARE_GAME_LOG_CATEGORY(MP4,                                         0x8EA6B78, All)
DECLARE_GAME_LOG_CATEGORY(OodleCompression,                            0x8BC4378, All)
DECLARE_GAME_LOG_CATEGORY(OodleHandlerComponentLog,                    0x8BC1F90, All)
DECLARE_GAME_LOG_CATEGORY(PacketHandlerLog,                            0x8E6C180, All)
DECLARE_GAME_LOG_CATEGORY(SandboxFile,                                 0x8EA5F30, All)
DECLARE_GAME_LOG_CATEGORY(SoundCueTemplates,                           0x8BC7100, All)
DECLARE_GAME_LOG_CATEGORY(TextureStreamingBuild,                       0x8E8F870, All)
DECLARE_GAME_LOG_CATEGORY(UGCC_LOG,                                    0x8E606F8, All)
DECLARE_GAME_LOG_CATEGORY(UGeometryCollectionLogging,                  0x8E61398, All)
DECLARE_GAME_LOG_CATEGORY(UManagedArrayLogging,                        0x8BD05C0, All)
DECLARE_GAME_LOG_CATEGORY(USkeletalMeshSimulationComponentLogging,     0x8E60570, All)
DECLARE_GAME_LOG_CATEGORY(UStaticMeshSimulationComponentLogging,       0x8E61460, All)
DECLARE_GAME_LOG_CATEGORY(VLogAbilitySystem,                           0x8B9C7F8, All)
DECLARE_GAME_LOG_CATEGORY(WindowsVideoRecordingSystem,                 0x8EA6B50, All)
DECLARE_GAME_LOG_CATEGORY(WMF,                                         0x8EA6AE0, All)
DECLARE_GAME_LOG_CATEGORY(WmfRingBuffer,                               0x8EA6B88, All)

namespace OutputDeviceColor
{
	const TCHAR* const COLOR_BLACK			= TEXT("0000");

	const TCHAR* const COLOR_DARK_RED		= TEXT("1000");
	const TCHAR* const COLOR_DARK_GREEN		= TEXT("0100");
	const TCHAR* const COLOR_DARK_BLUE		= TEXT("0010");
	const TCHAR* const COLOR_DARK_YELLOW	= TEXT("1100");
	const TCHAR* const COLOR_DARK_CYAN		= TEXT("0110");
	const TCHAR* const COLOR_DARK_PURPLE	= TEXT("1010");
	const TCHAR* const COLOR_DARK_WHITE		= TEXT("1110");
	const TCHAR* const COLOR_GRAY			= COLOR_DARK_WHITE;

	const TCHAR* const COLOR_RED			= TEXT("1001");
	const TCHAR* const COLOR_GREEN			= TEXT("0101");
	const TCHAR* const COLOR_BLUE			= TEXT("0011");
	const TCHAR* const COLOR_YELLOW			= TEXT("1101");
	const TCHAR* const COLOR_CYAN			= TEXT("0111");
	const TCHAR* const COLOR_PURPLE			= TEXT("1011");
	const TCHAR* const COLOR_WHITE			= TEXT("1111");

	const TCHAR* const COLOR_NONE			= TEXT("");
}

using namespace OutputDeviceColor;

#define SET_WARN_COLOR(Color) \
	UE_LOG(LogHAL, SetColor, TEXT("%s"), Color);
#define SET_WARN_COLOR_AND_BACKGROUND(Color, Bkgrnd) \
	UE_LOG(LogHAL, SetColor, TEXT("%s%s"), Color, Bkgrnd);

#define CLEAR_WARN_COLOR() \
	UE_LOG(LogHAL, SetColor, TEXT("%s"), COLOR_NONE);

namespace ESecurityEvent
{
	enum Type
	{
		Malformed_Packet = 0,
		Invalid_Data = 1,
		Closed = 2
	};

	inline const TCHAR* ToString(const ESecurityEvent::Type EnumVal)
	{
		switch (EnumVal)
		{
			case Malformed_Packet:
			{
				return TEXT("Malformed_Packet");
			}
			case Invalid_Data:
			{
				return TEXT("Invalid_Data");
			}
			case Closed:
			{
				return TEXT("Closed");
			}
		}
		return TEXT("");
	}
}

#define NOTIFY_CLIENT_OF_SECURITY_EVENT_IF_NOT_SHIPPING(NetConnection, SecurityPrint) ;

#define CLOSE_CONNECTION_DUE_TO_SECURITY_VIOLATION_INNER(NetConnection, SecurityEventType, Format, ...) \
{ \
	FString SecurityPrint = FString::Printf(UE_LOG_TEXT(Format), ##__VA_ARGS__); \
	UE_SECURITY_LOG(NetConnection, SecurityEventType, Format, ##__VA_ARGS__); \
	UE_SECURITY_LOG(NetConnection, ESecurityEvent::Closed, TEXT("Connection closed")); \
	SecurityPrint.Free(); \
	NetConnection->Close(); \
}

#define CLOSE_CONNECTION_DUE_TO_SECURITY_VIOLATION(NetConnection, SecurityEventType, Format, ...) \
	CLOSE_CONNECTION_DUE_TO_SECURITY_VIOLATION_INNER(NetConnection, SecurityEventType, Format, ##__VA_ARGS__)

#define logOrEnsureNanError(_FormatString_, ...) \
	{ \
		static bool OnceOnly = false; \
		if (!OnceOnly) \
		{ \
			UE_LOG(LogCore, Error, _FormatString_, ##__VA_ARGS__); \
			OnceOnly = true; \
		} \
	}
