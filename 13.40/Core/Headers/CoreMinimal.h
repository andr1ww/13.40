#pragma once
#include "pch.h"
#include "Core.h"

#ifndef UE_SERVER
	#define UE_SERVER 0
#endif

#ifndef UE_GAME
	#define UE_GAME 1
#endif

#ifndef UE_BUILD_SHIPPING
	#define UE_BUILD_SHIPPING 1
#endif

#ifndef NUM_TFUNCTION_INLINE_BYTES
	#define NUM_TFUNCTION_INLINE_BYTES 32
#endif

#ifndef PLATFORM_COMPILER_HAS_IF_CONSTEXPR
	#define PLATFORM_COMPILER_HAS_IF_CONSTEXPR 1
#endif

typedef decltype(nullptr) TYPE_OF_NULLPTR;

#ifndef CORE_API
	#define CORE_API
#endif

#ifndef ENGINE_API
	#define ENGINE_API
#endif

#ifndef USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME
	#define USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME 0
#endif

#ifndef DO_TIMEGUARD
	#define DO_TIMEGUARD 0
#endif

#ifndef UNLIKELY
	#define UNLIKELY(x)			(x)
#endif

typedef wchar_t WIDECHAR;

#define LINE_TERMINATOR TEXT("\r\n")

#define checkCode(...)
#define check(expr)					{ CA_ASSUME(expr); }
#define checkf(expr, format, ...)	{ CA_ASSUME(expr); }
#define checkNoEntry()
#define checkNoReentry()
#define checkNoRecursion()
#define verify(expr)				{ if(UNLIKELY(!(expr))){ CA_ASSUME(false); } }
#define verifyf(expr, format, ...)	{ if(UNLIKELY(!(expr))){ CA_ASSUME(false); } }
#define unimplemented()				{ CA_ASSUME(false); }

#define checkSlow(expr)					{ CA_ASSUME(expr); }
#define checkfSlow(expr, format, ...)	{ CA_ASSUME(expr); }
#define verifySlow(expr)				{ if(UNLIKELY(!(expr))) { CA_ASSUME(false); } }

#define ensure(           InExpression                ) (!!(InExpression))
#define ensureMsgf(       InExpression, InFormat, ... ) (!!(InExpression))
#define ensureAlways(     InExpression                ) (!!(InExpression))
#define ensureAlwaysMsgf( InExpression, InFormat, ... ) (!!(InExpression))

#define SCOPED_NAMED_EVENT(...)
#define SCOPED_NAMED_EVENT_FSTRING(...)
#define SCOPED_NAMED_EVENT_TCHAR(...)
#define SCOPED_NAMED_EVENT_TEXT(...)
#define SCOPED_NAMED_EVENT_F(...)
#define SCOPED_PROFILER_COLOR(...)

#define GFrameCounter (*reinterpret_cast<uint64*>(ImageBase + 0x8D26620))

/**
 * Check to see if this executable is running a dedicated server
 * Editor can run as a game with -server, but it won't be a dedicated server
 */
FORCEINLINE bool IsRunningDedicatedServer()
{
#if UE_SERVER
	return true;
#elif UE_GAME
	return false;
#else
	extern CORE_API int32 StaticDedicatedServerCheck();
	return (StaticDedicatedServerCheck() == 1);
#endif
}

/**
* Windows implementation of the Math OS functions
**/
struct FWindowsPlatformMath
{
	static FORCEINLINE int32 TruncToInt(float F)
	{
		return _mm_cvtt_ss2si(_mm_set_ss(F));
	}
};

typedef FWindowsPlatformMath FPlatformMath;

/**
 * Structure for all math helper functions, inherits from platform math to pick up platform-specific implementations
 * Check GenericPlatformMath.h for additional math functions
 */
struct FMath : public FPlatformMath
{
};

/**
 * Enum to indicate whether a string is null-terminated
 */
namespace ENullTerminatedString
{
	enum Type
	{
		No  = 0,
		Yes = 1
	};
}

#define DEFAULT_STRING_CONVERSION_SIZE 128u
#define UNICODE_BOGUS_CHAR_CODEPOINT '?'

struct FGenericPlatformString
{
	template <typename DestEncoding, typename SourceEncoding>
	static DestEncoding* Convert(DestEncoding* Dest, int32 DestSize, const SourceEncoding* Src, int32 SrcSize, DestEncoding BogusChar = (DestEncoding)'?');

	template <typename DestEncoding, typename SourceEncoding>
	static int32 ConvertedLength(const SourceEncoding* Src, int32 SrcSize)
	{
		return SrcSize;
	}
};

template <>
inline WIDECHAR* FGenericPlatformString::Convert<WIDECHAR, ANSICHAR>(WIDECHAR* Dest, int32 DestSize, const ANSICHAR* Src, int32 SrcSize, WIDECHAR BogusChar)
{
	return reinterpret_cast<WIDECHAR*(*)(WIDECHAR*, int32, const ANSICHAR*, int32, WIDECHAR)>(ImageBase + 0x92E4C0)(Dest, DestSize, Src, SrcSize, BogusChar);
}

typedef FGenericPlatformString FPlatformString;

template <typename From, typename To>
class TStringConvert
{
public:
	typedef From FromType;
	typedef To   ToType;

	FORCEINLINE static void Convert(To* Dest, int32 DestLen, const From* Source, int32 SourceLen)
	{
		To* Result = FPlatformString::Convert(Dest, DestLen, Source, SourceLen, (To)UNICODE_BOGUS_CHAR_CODEPOINT);
		check(Result);
	}

	static int32 ConvertedLength(const From* Source, int32 SourceLen)
	{
		return FPlatformString::ConvertedLength<To>(Source, SourceLen);
	}
};

/**
 * Class takes one type of string and converts it to another. The class includes a
 * chunk of presized memory of the destination type. If the presized array is
 * too small, it mallocs the memory needed and frees on the class going out of
 * scope.
 */
template<typename Converter, int32 DefaultConversionSize = DEFAULT_STRING_CONVERSION_SIZE>
class TStringConversion : private Converter, private TInlineAllocator<DefaultConversionSize>::template ForElementType<typename Converter::ToType>
{
	typedef typename TInlineAllocator<DefaultConversionSize>::template ForElementType<typename Converter::ToType> AllocatorType;

	typedef typename Converter::FromType FromType;
	typedef typename Converter::ToType   ToType;

	/**
	 * Converts the data by using the Convert() method on the base class
	 */
	void Init(const FromType* Source, int32 SourceLen, ENullTerminatedString::Type NullTerminated)
	{
		StringLength = Converter::ConvertedLength(Source, SourceLen);

		int32 BufferSize = StringLength + NullTerminated;

		AllocatorType::ResizeAllocation(0, BufferSize, sizeof(ToType));

		Ptr = (ToType*)AllocatorType::GetAllocation();
		Converter::Convert(Ptr, BufferSize, Source, SourceLen + NullTerminated);
	}

public:
	explicit TStringConversion(const FromType* Source)
	{
		if (Source)
		{
			Init(Source, static_cast<int32>(strlen(Source)), ENullTerminatedString::Yes);
		}
		else
		{
			Ptr = nullptr;
			StringLength = 0;
		}
	}

	/**
	 * Accessor for the converted string.
	 * @note The string may not be null-terminated if constructed from an explicitly sized buffer that didn't include the null-terminator.
	 *
	 * @return A const pointer to the converted string.
	 */
	FORCEINLINE const ToType* Get() const
	{
		return Ptr;
	}

	/**
	 * Length of the converted string.
	 *
	 * @return The number of characters in the converted string, excluding any null terminator.
	 */
	FORCEINLINE int32 Length() const
	{
		return StringLength;
	}

private:
	// Non-copyable
	TStringConversion(const TStringConversion&) = delete;
	TStringConversion& operator=(const TStringConversion&) = delete;

	ToType* Ptr;
	int32   StringLength;
};

/**
 * Creates an object which acts as a source of a given string type.  See example above.
 *
 * @param Str The null-terminated source string to convert.
 */
template <typename To, typename From>
FORCEINLINE TStringConversion<TStringConvert<From, To>> StringCast(const From* Str)
{
	return TStringConversion<TStringConvert<From, To>>(Str);
}

#define ANSI_TO_TCHAR(str) (TCHAR*)StringCast<TCHAR>(static_cast<const ANSICHAR*>(str)).Get()

//
// String output device.
//
class FStringOutputDevice : public FString, public FOutputDevice
{
public:
	FStringOutputDevice( const TCHAR* OutputDeviceName=TEXT("") ):
		FString( OutputDeviceName )
	{
		bAutoEmitLineTerminator = false;
	}
	virtual void Serialize( const TCHAR* InData, ELogVerbosity::Type Verbosity, const class FName& Category ) override
	{
		FString::operator+=((TCHAR*)InData);
		if(bAutoEmitLineTerminator)
		{
			*this += LINE_TERMINATOR;
		}
	}

	FStringOutputDevice(FStringOutputDevice&&) = default;
	FStringOutputDevice(const FStringOutputDevice&) = default;
	FStringOutputDevice& operator=(FStringOutputDevice&&) = default;
	FStringOutputDevice& operator=(const FStringOutputDevice&) = default;

	// Make += operator virtual.
	virtual FString& operator+=(const FString& Other)
	{
		return FString::operator+=(Other);
	}
};

#include "Templates/Function.h"
