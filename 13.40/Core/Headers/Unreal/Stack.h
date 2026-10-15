#pragma once
#include "pch.h"

class FFrame : public FOutputDevice
{
public:
	UFunction* Node;
	UObject* Object;
	uint8* Code;
	uint8* Locals;
	FProperty* MostRecentProperty;
	uint8* MostRecentPropertyAddress;
	uint8 FlowStack[0x30];
	FFrame* PreviousFrame;
	void* OutParms;
	FField* PropertyChainForCompiledIn;
	UFunction* CurrentNativeFunction;
	bool bArrayContextFailed;

public:
	void Step(UObject* Context, void* const Result)
	{
		reinterpret_cast<void(*)(FFrame*, UObject*, void* const)>(ImageBase + 0x3531130)(this, Context, Result);
	}

	void StepExplicitProperty(void* const Result, FField* Property)
	{
		reinterpret_cast<void(*)(FFrame*, void* const, FField*)>(ImageBase + 0x3531160)(this, Result, Property);
	}

	void StepCompiledIn(void* const Result)
	{
		if (Code)
		{
			Step(Object, Result);
		}
		else
		{
			FField* Property = PropertyChainForCompiledIn;
			PropertyChainForCompiledIn = Property->Next;
			StepExplicitProperty(Result, Property);
		}
	}

	template <typename T>
	T StepCompiledInRef()
	{
		T TempVal{};
		MostRecentPropertyAddress = nullptr;

		StepCompiledIn(&TempVal);

		return MostRecentPropertyAddress ? *reinterpret_cast<T*>(MostRecentPropertyAddress) : TempVal;
	}

	void IncrementCode()
	{
		Code += !!Code;
	}
};
