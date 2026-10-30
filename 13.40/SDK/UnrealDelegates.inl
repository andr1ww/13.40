#pragma once

namespace DelegatePrivate
{
	inline class UFunction* FindFunction(const class UObject* Object, FName FunctionName)
	{
		return InSDKUtils::CallGameFunction(reinterpret_cast<class UFunction*(*)(const class UObject*, FName)>(InSDKUtils::GetImageBase() + 0x351D590), Object, FunctionName);
	}

	inline void ProcessEvent(const class UObject* Object, class UFunction* Function, void* Parms)
	{
		InSDKUtils::CallGameFunction(InSDKUtils::GetVirtualFunction<void(*)(const class UObject*, class UFunction*, void*)>(Object, Offsets::ProcessEventIdx), Object, Function, Parms);
	}

	template<typename... Types>
	struct TParmsLayout
	{
		static constexpr uint64 Alignment = []
		{
			uint64 Result = 1;
			((Result = alignof(Types) > Result ? alignof(Types) : Result), ...);
			return Result;
		}();

		static constexpr std::array<uint64, sizeof...(Types) + 1> Offsets = []
		{
			std::array<uint64, sizeof...(Types) + 1> Result{};
			uint64 Offset = 0;
			uint64 Index = 0;
			((Offset = (Offset + alignof(Types) - 1) & ~(alignof(Types) - 1), Result[Index++] = Offset, Offset += sizeof(Types)), ...);
			Result[Index] = Offset;
			return Result;
		}();

		static constexpr uint64 Size = Offsets[sizeof...(Types)] ? (Offsets[sizeof...(Types)] + Alignment - 1) & ~(Alignment - 1) : 1;
	};

	template<typename ParamType>
	inline void CopyOut(ParamType& Param, uint8* Value)
	{
		if constexpr (std::is_lvalue_reference_v<ParamType> && !std::is_const_v<std::remove_reference_t<ParamType>>)
			Param = *reinterpret_cast<std::remove_reference_t<ParamType>*>(Value);
	}

	template<typename Type>
	inline void Destroy(uint8* Value)
	{
		reinterpret_cast<Type*>(Value)->~Type();
	}

	template<typename Ret, typename... ParamTypes>
	struct TParms
	{
		using FLayout = std::conditional_t<std::is_void_v<Ret>, TParmsLayout<std::remove_cvref_t<ParamTypes>...>, TParmsLayout<std::remove_cvref_t<ParamTypes>..., Ret>>;

		template<typename CallType>
		static Ret Invoke(CallType&& Call, ParamTypes... Params)
		{
			alignas(FLayout::Alignment) uint8 Parms[FLayout::Size];
			memset(Parms, 0, sizeof(Parms));

			[&]<size_t... Indices>(std::index_sequence<Indices...>)
			{
				(new (Parms + FLayout::Offsets[Indices]) std::remove_cvref_t<ParamTypes>(Params), ...);
			}(std::index_sequence_for<ParamTypes...>{});

			if constexpr (!std::is_void_v<Ret>)
				new (Parms + FLayout::Offsets[sizeof...(ParamTypes)]) Ret();

			Call(static_cast<void*>(Parms));

			[&]<size_t... Indices>(std::index_sequence<Indices...>)
			{
				(CopyOut<ParamTypes>(Params, Parms + FLayout::Offsets[Indices]), ...);
				(Destroy<std::remove_cvref_t<ParamTypes>>(Parms + FLayout::Offsets[Indices]), ...);
			}(std::index_sequence_for<ParamTypes...>{});

			if constexpr (!std::is_void_v<Ret>)
			{
				Ret* ReturnValue = reinterpret_cast<Ret*>(Parms + FLayout::Offsets[sizeof...(ParamTypes)]);
				Ret Result = std::move(*ReturnValue);
				ReturnValue->~Ret();
				return Result;
			}
		}
	};

	template<bool bConst, typename Class, typename FuncType>
	struct TMemFunPtrType;

	template<typename Class, typename Ret, typename... Args>
	struct TMemFunPtrType<false, Class, Ret(Args...)>
	{
		using Type = Ret(Class::*)(Args...);
	};

	template<typename Class, typename Ret, typename... Args>
	struct TMemFunPtrType<true, Class, Ret(Args...)>
	{
		using Type = Ret(Class::*)(Args...) const;
	};
}

class FDelegateHandle
{
public:
	enum EGenerateNewHandleType
	{
		GenerateNewHandle
	};

	FDelegateHandle()
		: ID(0)
	{
	}

	explicit FDelegateHandle(EGenerateNewHandleType)
		: ID(GenerateNewID())
	{
	}

	bool IsValid() const
	{
		return ID != 0;
	}

	void Reset()
	{
		ID = 0;
	}

	bool operator==(const FDelegateHandle& Other) const
	{
		return ID == Other.ID;
	}

	bool operator!=(const FDelegateHandle& Other) const
	{
		return ID != Other.ID;
	}

private:
	static uint64 GenerateNewID()
	{
		return InSDKUtils::CallGameFunction(reinterpret_cast<uint64(*)()>(InSDKUtils::GetImageBase() + 0x31E7BD0));
	}

	uint64 ID;
};

class IDelegateInstance
{
public:
	virtual class UObject* GetUObject() const = 0;
	virtual const void* GetObjectForTimerManager() const = 0;
	virtual uint64 GetBoundProgramCounterForTimerManager() const = 0;
	virtual bool HasSameObject(const void* InUserObject) const = 0;
	virtual bool IsCompactable() const
	{
		return !IsSafeToExecute();
	}
	virtual bool IsSafeToExecute() const = 0;
	virtual FDelegateHandle GetHandle() const = 0;
	virtual ~IDelegateInstance() = default;
};

class FDelegateBase
{
public:
	FDelegateBase()
		: Allocation(nullptr), DelegateSize(0)
	{
	}

	~FDelegateBase()
	{
		Unbind();
	}

	FDelegateBase(FDelegateBase&& Other)
		: Allocation(Other.Allocation), DelegateSize(Other.DelegateSize)
	{
		Other.Allocation = nullptr;
		Other.DelegateSize = 0;
	}

	FDelegateBase& operator=(FDelegateBase&& Other)
	{
		Unbind();
		Allocation = Other.Allocation;
		DelegateSize = Other.DelegateSize;
		Other.Allocation = nullptr;
		Other.DelegateSize = 0;
		return *this;
	}

public:
	void Unbind()
	{
		if (IDelegateInstance* Instance = GetDelegateInstanceProtected())
		{
			Instance->~IDelegateInstance();
			Allocation = FMemory::Realloc(Allocation, 0);
			DelegateSize = 0;
		}
	}

	class UObject* GetUObject() const
	{
		IDelegateInstance* Instance = GetDelegateInstanceProtected();
		return Instance ? Instance->GetUObject() : nullptr;
	}

	FDelegateHandle GetHandle() const
	{
		IDelegateInstance* Instance = GetDelegateInstanceProtected();
		return Instance ? Instance->GetHandle() : FDelegateHandle();
	}

	FORCEINLINE const void* GetObjectForTimerManager() const
	{
		IDelegateInstance* Ptr = GetDelegateInstanceProtected();

		const void* Result = Ptr ? Ptr->GetObjectForTimerManager() : nullptr;
		return Result;
	}

	/**
	 * Returns the address of the method pointer which can be used to learn the address of the function that will be executed.
	 * Returns nullptr if this delegate type does not directly invoke a function pointer.
	 *
	 * Note: Only intended to be used to aid debugging of delegates.
	 *
	 * @return The address of the function pointer that would be executed by this delegate
	 */
	uint64 GetBoundProgramCounterForTimerManager() const
	{
		if (IDelegateInstance* Ptr = GetDelegateInstanceProtected())
		{
			return Ptr->GetBoundProgramCounterForTimerManager();
		}

		return 0;
	}

	IDelegateInstance* GetDelegateInstanceProtected() const
	{
		return DelegateSize ? reinterpret_cast<IDelegateInstance*>(Allocation) : nullptr;
	}

	void* Allocate(int32 Size)
	{
		if (IDelegateInstance* Instance = GetDelegateInstanceProtected())
			Instance->~IDelegateInstance();

		const int32 NewDelegateSize = (Size + 0xF) / 0x10;

		if (DelegateSize != NewDelegateSize)
		{
			Allocation = FMemory::Realloc(Allocation, 0x10ull * NewDelegateSize);
			DelegateSize = NewDelegateSize;
		}

		return Allocation;
	}

private:
	void* Allocation;
	int32 DelegateSize;
};

template<typename Ret, typename... Args>
class IBaseDelegateInstanceCommon : public IDelegateInstance
{
public:
	virtual void CreateCopy(FDelegateBase& Base) = 0;
	virtual Ret Execute(Args...) const = 0;
};

template<typename FuncType>
class IBaseDelegateInstance;

template<typename Ret, typename... Args>
class IBaseDelegateInstance<Ret(Args...)> : public IBaseDelegateInstanceCommon<Ret, Args...>
{
};

template<typename... Args>
class IBaseDelegateInstance<void(Args...)> : public IBaseDelegateInstanceCommon<void, Args...>
{
public:
	virtual bool ExecuteIfSafe(Args...) const = 0;
};

template<bool bConst, typename UserClass, typename FuncType, typename... VarTypes>
class TBaseUObjectMethodDelegateInstance;

template<bool bConst, typename UserClass, typename Ret, typename... Args, typename... VarTypes>
class TBaseUObjectMethodDelegateInstance<bConst, UserClass, Ret(Args...), VarTypes...> : public IBaseDelegateInstance<Ret(Args...)>
{
public:
	using FMethodPtr = typename DelegatePrivate::TMemFunPtrType<bConst, UserClass, Ret(Args..., VarTypes...)>::Type;

	TBaseUObjectMethodDelegateInstance(UserClass* InUserObject, FMethodPtr InMethodPtr, VarTypes... Vars)
		: MethodPtr(InMethodPtr), Payload(Vars...), Handle(FDelegateHandle::GenerateNewHandle)
	{
		UserObject = InUserObject;
	}

	class UObject* GetUObject() const override
	{
		return UserObject.Get();
	}

	const void* GetObjectForTimerManager() const override
	{
		return UserObject.Get();
	}

	uint64 GetBoundProgramCounterForTimerManager() const override
	{
		return *reinterpret_cast<const uint64*>(&MethodPtr);
	}

	bool HasSameObject(const void* InUserObject) const override
	{
		return UserObject.Get() == InUserObject;
	}

	bool IsCompactable() const override
	{
		return !UserObject.Get(true);
	}

	bool IsSafeToExecute() const override
	{
		return !!UserObject.Get();
	}

	FDelegateHandle GetHandle() const override
	{
		return Handle;
	}

	void CreateCopy(FDelegateBase& Base) override
	{
		new (Base.Allocate(sizeof(TBaseUObjectMethodDelegateInstance))) TBaseUObjectMethodDelegateInstance(*this);
	}

	Ret Execute(Args... InArgs) const override
	{
		using MutableUserClass = std::remove_const_t<UserClass>;
		MutableUserClass* MutableUserObject = const_cast<MutableUserClass*>(static_cast<UserClass*>(UserObject.Get()));

		return std::apply([&](const VarTypes&... Vars) -> Ret { return (MutableUserObject->*MethodPtr)(std::forward<Args>(InArgs)..., Vars...); }, Payload);
	}

	bool ExecuteIfSafe(Args... InArgs) const
	{
		if (!UserObject.Get())
			return false;

		Execute(std::forward<Args>(InArgs)...);
		return true;
	}

protected:
	FWeakObjectPtr UserObject;
	FMethodPtr MethodPtr;
	std::tuple<VarTypes...> Payload;
	FDelegateHandle Handle;
};

template<bool bConst, typename UserClass, typename FuncType, typename... VarTypes>
class TBaseRawMethodDelegateInstance;

template<bool bConst, typename UserClass, typename Ret, typename... Args, typename... VarTypes>
class TBaseRawMethodDelegateInstance<bConst, UserClass, Ret(Args...), VarTypes...> : public IBaseDelegateInstance<Ret(Args...)>
{
public:
	using FMethodPtr = typename DelegatePrivate::TMemFunPtrType<bConst, UserClass, Ret(Args..., VarTypes...)>::Type;

	TBaseRawMethodDelegateInstance(UserClass* InUserObject, FMethodPtr InMethodPtr, VarTypes... Vars)
		: UserObject(InUserObject), MethodPtr(InMethodPtr), Payload(Vars...), Handle(FDelegateHandle::GenerateNewHandle)
	{
	}

	class UObject* GetUObject() const override
	{
		return nullptr;
	}

	const void* GetObjectForTimerManager() const override
	{
		return UserObject;
	}

	uint64 GetBoundProgramCounterForTimerManager() const override
	{
		return *reinterpret_cast<const uint64*>(&MethodPtr);
	}

	bool HasSameObject(const void* InUserObject) const override
	{
		return UserObject == InUserObject;
	}

	bool IsSafeToExecute() const override
	{
		return true;
	}

	FDelegateHandle GetHandle() const override
	{
		return Handle;
	}

	void CreateCopy(FDelegateBase& Base) override
	{
		new (Base.Allocate(sizeof(TBaseRawMethodDelegateInstance))) TBaseRawMethodDelegateInstance(*this);
	}

	Ret Execute(Args... InArgs) const override
	{
		using MutableUserClass = std::remove_const_t<UserClass>;
		MutableUserClass* MutableUserObject = const_cast<MutableUserClass*>(UserObject);

		return std::apply([&](const VarTypes&... Vars) -> Ret { return (MutableUserObject->*MethodPtr)(std::forward<Args>(InArgs)..., Vars...); }, Payload);
	}

	bool ExecuteIfSafe(Args... InArgs) const
	{
		Execute(std::forward<Args>(InArgs)...);
		return true;
	}

protected:
	UserClass* UserObject;
	FMethodPtr MethodPtr;
	std::tuple<VarTypes...> Payload;
	FDelegateHandle Handle;
};

template<bool bConst, typename UserClass, ESPMode Mode, typename FuncType, typename... VarTypes>
class TBaseSPMethodDelegateInstance;

template<bool bConst, typename UserClass, ESPMode Mode, typename Ret, typename... Args, typename... VarTypes>
class TBaseSPMethodDelegateInstance<bConst, UserClass, Mode, Ret(Args...), VarTypes...> : public IBaseDelegateInstance<Ret(Args...)>
{
public:
	using FMethodPtr = typename DelegatePrivate::TMemFunPtrType<bConst, UserClass, Ret(Args..., VarTypes...)>::Type;

	TBaseSPMethodDelegateInstance(const TSharedPtr<UserClass, Mode>& InUserObject, FMethodPtr InMethodPtr, VarTypes... Vars)
		: UserObject(InUserObject), MethodPtr(InMethodPtr), Payload(Vars...), Handle(FDelegateHandle::GenerateNewHandle)
	{
	}

	class UObject* GetUObject() const override
	{
		return nullptr;
	}

	const void* GetObjectForTimerManager() const override
	{
		return UserObject.Pin().Get();
	}

	uint64 GetBoundProgramCounterForTimerManager() const override
	{
		return *reinterpret_cast<const uint64*>(&MethodPtr);
	}

	bool HasSameObject(const void* InUserObject) const override
	{
		return UserObject.HasSameObject(InUserObject);
	}

	bool IsSafeToExecute() const override
	{
		return UserObject.IsValid();
	}

	FDelegateHandle GetHandle() const override
	{
		return Handle;
	}

	void CreateCopy(FDelegateBase& Base) override
	{
		new (Base.Allocate(sizeof(TBaseSPMethodDelegateInstance))) TBaseSPMethodDelegateInstance(*this);
	}

	Ret Execute(Args... InArgs) const override
	{
		using MutableUserClass = std::remove_const_t<UserClass>;

		TSharedPtr<UserClass, Mode> SharedUserObject = UserObject.Pin();
		MutableUserClass* MutableUserObject = const_cast<MutableUserClass*>(SharedUserObject.Get());

		return std::apply([&](const VarTypes&... Vars) -> Ret { return (MutableUserObject->*MethodPtr)(std::forward<Args>(InArgs)..., Vars...); }, Payload);
	}

	bool ExecuteIfSafe(Args... InArgs) const
	{
		using MutableUserClass = std::remove_const_t<UserClass>;

		TSharedPtr<UserClass, Mode> SharedUserObject = UserObject.Pin();

		if (!SharedUserObject.IsValid())
			return false;

		MutableUserClass* MutableUserObject = const_cast<MutableUserClass*>(SharedUserObject.Get());

		std::apply([&](const VarTypes&... Vars) { (MutableUserObject->*MethodPtr)(std::forward<Args>(InArgs)..., Vars...); }, Payload);
		return true;
	}

protected:
	TWeakPtr<UserClass, Mode> UserObject;
	FMethodPtr MethodPtr;
	std::tuple<VarTypes...> Payload;
	FDelegateHandle Handle;
};

template<typename FuncType, typename... VarTypes>
class TBaseStaticDelegateInstance;

template<typename Ret, typename... Args, typename... VarTypes>
class TBaseStaticDelegateInstance<Ret(Args...), VarTypes...> : public IBaseDelegateInstance<Ret(Args...)>
{
public:
	using FFuncPtr = Ret(*)(Args..., VarTypes...);

	TBaseStaticDelegateInstance(FFuncPtr InStaticFuncPtr, VarTypes... Vars)
		: StaticFuncPtr(InStaticFuncPtr), Payload(Vars...), Handle(FDelegateHandle::GenerateNewHandle)
	{
	}

	class UObject* GetUObject() const override
	{
		return nullptr;
	}

	const void* GetObjectForTimerManager() const override
	{
		return nullptr;
	}

	uint64 GetBoundProgramCounterForTimerManager() const override
	{
		return reinterpret_cast<uint64>(StaticFuncPtr);
	}

	bool HasSameObject(const void* InUserObject) const override
	{
		return false;
	}

	bool IsSafeToExecute() const override
	{
		return true;
	}

	FDelegateHandle GetHandle() const override
	{
		return Handle;
	}

	void CreateCopy(FDelegateBase& Base) override
	{
		new (Base.Allocate(sizeof(TBaseStaticDelegateInstance))) TBaseStaticDelegateInstance(*this);
	}

	Ret Execute(Args... InArgs) const override
	{
		return std::apply([&](const VarTypes&... Vars) -> Ret { return StaticFuncPtr(std::forward<Args>(InArgs)..., Vars...); }, Payload);
	}

	bool ExecuteIfSafe(Args... InArgs) const
	{
		Execute(std::forward<Args>(InArgs)...);
		return true;
	}

private:
	FFuncPtr StaticFuncPtr;
	std::tuple<VarTypes...> Payload;
	FDelegateHandle Handle;
};

template<typename FuncType, typename FunctorType, typename... VarTypes>
class TBaseFunctorDelegateInstance;

template<typename Ret, typename... Args, typename FunctorType, typename... VarTypes>
class TBaseFunctorDelegateInstance<Ret(Args...), FunctorType, VarTypes...> : public IBaseDelegateInstance<Ret(Args...)>
{
public:
	template<typename InFunctorType>
	TBaseFunctorDelegateInstance(InFunctorType&& InFunctor, VarTypes... Vars)
		: Functor(std::forward<InFunctorType>(InFunctor)), Payload(Vars...), Handle(FDelegateHandle::GenerateNewHandle)
	{
	}

	class UObject* GetUObject() const override
	{
		return nullptr;
	}

	const void* GetObjectForTimerManager() const override
	{
		return nullptr;
	}

	uint64 GetBoundProgramCounterForTimerManager() const override
	{
		return 0;
	}

	bool HasSameObject(const void* InUserObject) const override
	{
		return false;
	}

	bool IsSafeToExecute() const override
	{
		return true;
	}

	FDelegateHandle GetHandle() const override
	{
		return Handle;
	}

	void CreateCopy(FDelegateBase& Base) override
	{
		new (Base.Allocate(sizeof(TBaseFunctorDelegateInstance))) TBaseFunctorDelegateInstance(*this);
	}

	Ret Execute(Args... InArgs) const override
	{
		return std::apply([&](const VarTypes&... Vars) -> Ret { return Functor(std::forward<Args>(InArgs)..., Vars...); }, Payload);
	}

	bool ExecuteIfSafe(Args... InArgs) const
	{
		Execute(std::forward<Args>(InArgs)...);
		return true;
	}

private:
	mutable std::remove_const_t<FunctorType> Functor;
	std::tuple<VarTypes...> Payload;
	FDelegateHandle Handle;
};

template<typename UserClass, typename FuncType, typename FunctorType, typename... VarTypes>
class TWeakBaseFunctorDelegateInstance;

template<typename UserClass, typename Ret, typename... Args, typename FunctorType, typename... VarTypes>
class TWeakBaseFunctorDelegateInstance<UserClass, Ret(Args...), FunctorType, VarTypes...> : public IBaseDelegateInstance<Ret(Args...)>
{
public:
	template<typename InFunctorType>
	TWeakBaseFunctorDelegateInstance(UserClass* InContextObject, InFunctorType&& InFunctor, VarTypes... Vars)
		: Functor(std::forward<InFunctorType>(InFunctor)), Payload(Vars...), Handle(FDelegateHandle::GenerateNewHandle)
	{
		ContextObject = InContextObject;
	}

	class UObject* GetUObject() const override
	{
		return ContextObject.Get();
	}

	const void* GetObjectForTimerManager() const override
	{
		return ContextObject.Get();
	}

	uint64 GetBoundProgramCounterForTimerManager() const override
	{
		return 0;
	}

	bool HasSameObject(const void* InUserObject) const override
	{
		return ContextObject.Get() == InUserObject;
	}

	bool IsCompactable() const override
	{
		return !ContextObject.Get(true);
	}

	bool IsSafeToExecute() const override
	{
		return !!ContextObject.Get();
	}

	FDelegateHandle GetHandle() const override
	{
		return Handle;
	}

	void CreateCopy(FDelegateBase& Base) override
	{
		new (Base.Allocate(sizeof(TWeakBaseFunctorDelegateInstance))) TWeakBaseFunctorDelegateInstance(*this);
	}

	Ret Execute(Args... InArgs) const override
	{
		return std::apply([&](const VarTypes&... Vars) -> Ret { return Functor(std::forward<Args>(InArgs)..., Vars...); }, Payload);
	}

	bool ExecuteIfSafe(Args... InArgs) const
	{
		if (!ContextObject.Get())
			return false;

		Execute(std::forward<Args>(InArgs)...);
		return true;
	}

private:
	FWeakObjectPtr ContextObject;
	mutable std::remove_const_t<FunctorType> Functor;
	std::tuple<VarTypes...> Payload;
	FDelegateHandle Handle;
};

template<typename UserClass, typename FuncType, typename... VarTypes>
class TBaseUFunctionDelegateInstance;

template<typename UserClass, typename Ret, typename... Args, typename... VarTypes>
class TBaseUFunctionDelegateInstance<UserClass, Ret(Args...), VarTypes...> : public IBaseDelegateInstance<Ret(Args...)>
{
public:
	TBaseUFunctionDelegateInstance(UserClass* InUserObject, const FName& InFunctionName, VarTypes... Vars)
		: FunctionName(InFunctionName), CachedFunction(DelegatePrivate::FindFunction(InUserObject, InFunctionName)), Payload(Vars...), Handle(FDelegateHandle::GenerateNewHandle)
	{
		UserObjectPtr = InUserObject;
	}

	class UObject* GetUObject() const override
	{
		return UserObjectPtr.Get();
	}

	const void* GetObjectForTimerManager() const override
	{
		return UserObjectPtr.Get();
	}

	uint64 GetBoundProgramCounterForTimerManager() const override
	{
		return 0;
	}

	bool HasSameObject(const void* InUserObject) const override
	{
		return UserObjectPtr.Get() == InUserObject;
	}

	bool IsCompactable() const override
	{
		return !UserObjectPtr.Get(true);
	}

	bool IsSafeToExecute() const override
	{
		return UserObjectPtr.IsValid();
	}

	FDelegateHandle GetHandle() const override
	{
		return Handle;
	}

	void CreateCopy(FDelegateBase& Base) override
	{
		new (Base.Allocate(sizeof(TBaseUFunctionDelegateInstance))) TBaseUFunctionDelegateInstance(*this);
	}

	Ret Execute(Args... InArgs) const override
	{
		class UObject* Object = UserObjectPtr.Get();

		return std::apply([&](const VarTypes&... Vars) -> Ret
		{
			return DelegatePrivate::TParms<Ret, Args..., VarTypes...>::Invoke([&](void* Parms) { DelegatePrivate::ProcessEvent(Object, CachedFunction, Parms); }, std::forward<Args>(InArgs)..., Vars...);
		}, Payload);
	}

	bool ExecuteIfSafe(Args... InArgs) const
	{
		if (!UserObjectPtr.Get())
			return false;

		Execute(std::forward<Args>(InArgs)...);
		return true;
	}

private:
	FName FunctionName;
	class UFunction* CachedFunction;
	FWeakObjectPtr UserObjectPtr;
	std::tuple<VarTypes...> Payload;
	FDelegateHandle Handle;
};

template<typename FuncType>
class TBaseDelegate;

template<typename Ret, typename... Args>
class TBaseDelegate<Ret(Args...)> : public FDelegateBase
{
	using FDelegateInstanceInterface = IBaseDelegateInstance<Ret(Args...)>;

public:
	template <typename UserClass> struct TUObjectMethodDelegate       : TBaseUObjectMethodDelegateInstance<false, UserClass, Ret(Args...)> { typedef TBaseUObjectMethodDelegateInstance<false, UserClass, Ret(Args...)> Super; TUObjectMethodDelegate      (UserClass* InUserObject, typename Super::FMethodPtr InMethodPtr) : Super(InUserObject, InMethodPtr) {} };
	template <typename UserClass> struct TUObjectMethodDelegate_Const : TBaseUObjectMethodDelegateInstance<true , UserClass, Ret(Args...)> { typedef TBaseUObjectMethodDelegateInstance<true , UserClass, Ret(Args...)> Super; TUObjectMethodDelegate_Const(UserClass* InUserObject, typename Super::FMethodPtr InMethodPtr) : Super(InUserObject, InMethodPtr) {} };

public:
	TBaseDelegate() = default;
	TBaseDelegate(TBaseDelegate&&) = default;
	TBaseDelegate& operator=(TBaseDelegate&&) = default;

	TBaseDelegate(const TBaseDelegate& Other)
	{
		*this = Other;
	}

	TBaseDelegate& operator=(const TBaseDelegate& Other)
	{
		if (&Other != this)
		{
			if (FDelegateInstanceInterface* OtherInstance = Other.GetInstance())
				OtherInstance->CreateCopy(*this);
			else
				Unbind();
		}

		return *this;
	}

public:
	template<typename... VarTypes>
	static TBaseDelegate CreateStatic(typename TBaseStaticDelegateInstance<Ret(Args...), VarTypes...>::FFuncPtr InFunc, VarTypes... Vars)
	{
		TBaseDelegate Result;
		Result.BindStatic(InFunc, Vars...);
		return Result;
	}

	template<typename FunctorType, typename... VarTypes>
	static TBaseDelegate CreateLambda(FunctorType&& InFunctor, VarTypes... Vars)
	{
		TBaseDelegate Result;
		Result.BindLambda(std::forward<FunctorType>(InFunctor), Vars...);
		return Result;
	}

	template<typename UserClass, typename FunctorType, typename... VarTypes>
	static TBaseDelegate CreateWeakLambda(UserClass* InUserObject, FunctorType&& InFunctor, VarTypes... Vars)
	{
		TBaseDelegate Result;
		Result.BindWeakLambda(InUserObject, std::forward<FunctorType>(InFunctor), Vars...);
		return Result;
	}

	template<typename UserClass, typename... VarTypes>
	static TBaseDelegate CreateRaw(UserClass* InUserObject, typename DelegatePrivate::TMemFunPtrType<false, UserClass, Ret(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		TBaseDelegate Result;
		Result.BindRaw(InUserObject, InFunc, Vars...);
		return Result;
	}

	template<typename UserClass, typename... VarTypes>
	static TBaseDelegate CreateRaw(const UserClass* InUserObject, typename DelegatePrivate::TMemFunPtrType<true, UserClass, Ret(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		TBaseDelegate Result;
		Result.BindRaw(InUserObject, InFunc, Vars...);
		return Result;
	}

	template<typename UserClass, ESPMode Mode, typename... VarTypes>
	static TBaseDelegate CreateSP(const TSharedRef<UserClass, Mode>& InUserObjectRef, typename DelegatePrivate::TMemFunPtrType<false, UserClass, Ret(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		TBaseDelegate Result;
		Result.BindSP(InUserObjectRef, InFunc, Vars...);
		return Result;
	}

	template<typename UserClass, ESPMode Mode, typename... VarTypes>
	static TBaseDelegate CreateSP(const TSharedRef<UserClass, Mode>& InUserObjectRef, typename DelegatePrivate::TMemFunPtrType<true, UserClass, Ret(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		TBaseDelegate Result;
		Result.BindSP(InUserObjectRef, InFunc, Vars...);
		return Result;
	}

	template<typename UserClass, typename... VarTypes>
	static TBaseDelegate CreateSP(UserClass* InUserObject, typename DelegatePrivate::TMemFunPtrType<false, UserClass, Ret(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		TBaseDelegate Result;
		Result.BindSP(InUserObject, InFunc, Vars...);
		return Result;
	}

	template<typename UserClass, typename... VarTypes>
	static TBaseDelegate CreateSP(const UserClass* InUserObject, typename DelegatePrivate::TMemFunPtrType<true, UserClass, Ret(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		TBaseDelegate Result;
		Result.BindSP(InUserObject, InFunc, Vars...);
		return Result;
	}

	template<typename UserClass, typename... VarTypes>
	static TBaseDelegate CreateUObject(UserClass* InUserObject, typename DelegatePrivate::TMemFunPtrType<false, UserClass, Ret(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		TBaseDelegate Result;
		Result.BindUObject(InUserObject, InFunc, Vars...);
		return Result;
	}

	template<typename UserClass, typename... VarTypes>
	static TBaseDelegate CreateUObject(const UserClass* InUserObject, typename DelegatePrivate::TMemFunPtrType<true, UserClass, Ret(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		TBaseDelegate Result;
		Result.BindUObject(InUserObject, InFunc, Vars...);
		return Result;
	}

	template<typename UserClass, typename... VarTypes>
	static TBaseDelegate CreateUFunction(UserClass* InUserObject, const FName& InFunctionName, VarTypes... Vars)
	{
		TBaseDelegate Result;
		Result.BindUFunction(InUserObject, InFunctionName, Vars...);
		return Result;
	}

public:
	template<typename... VarTypes>
	void BindStatic(typename TBaseStaticDelegateInstance<Ret(Args...), VarTypes...>::FFuncPtr InFunc, VarTypes... Vars)
	{
		using FInstance = TBaseStaticDelegateInstance<Ret(Args...), VarTypes...>;
		new (Allocate(sizeof(FInstance))) FInstance(InFunc, Vars...);
	}

	template<typename FunctorType, typename... VarTypes>
	void BindLambda(FunctorType&& InFunctor, VarTypes... Vars)
	{
		using FInstance = TBaseFunctorDelegateInstance<Ret(Args...), std::remove_reference_t<FunctorType>, VarTypes...>;
		new (Allocate(sizeof(FInstance))) FInstance(std::forward<FunctorType>(InFunctor), Vars...);
	}

	template<typename UserClass, typename FunctorType, typename... VarTypes>
	void BindWeakLambda(UserClass* InUserObject, FunctorType&& InFunctor, VarTypes... Vars)
	{
		using FInstance = TWeakBaseFunctorDelegateInstance<UserClass, Ret(Args...), std::remove_reference_t<FunctorType>, VarTypes...>;
		new (Allocate(sizeof(FInstance))) FInstance(InUserObject, std::forward<FunctorType>(InFunctor), Vars...);
	}

	template<typename UserClass, typename... VarTypes>
	void BindRaw(UserClass* InUserObject, typename DelegatePrivate::TMemFunPtrType<false, UserClass, Ret(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		using FInstance = TBaseRawMethodDelegateInstance<false, UserClass, Ret(Args...), VarTypes...>;
		new (Allocate(sizeof(FInstance))) FInstance(InUserObject, InFunc, Vars...);
	}

	template<typename UserClass, typename... VarTypes>
	void BindRaw(const UserClass* InUserObject, typename DelegatePrivate::TMemFunPtrType<true, UserClass, Ret(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		using FInstance = TBaseRawMethodDelegateInstance<true, const UserClass, Ret(Args...), VarTypes...>;
		new (Allocate(sizeof(FInstance))) FInstance(InUserObject, InFunc, Vars...);
	}

	template<typename UserClass, ESPMode Mode, typename... VarTypes>
	void BindSP(const TSharedRef<UserClass, Mode>& InUserObjectRef, typename DelegatePrivate::TMemFunPtrType<false, UserClass, Ret(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		using FInstance = TBaseSPMethodDelegateInstance<false, UserClass, Mode, Ret(Args...), VarTypes...>;
		new (Allocate(sizeof(FInstance))) FInstance(InUserObjectRef, InFunc, Vars...);
	}

	template<typename UserClass, ESPMode Mode, typename... VarTypes>
	void BindSP(const TSharedRef<UserClass, Mode>& InUserObjectRef, typename DelegatePrivate::TMemFunPtrType<true, UserClass, Ret(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		using FInstance = TBaseSPMethodDelegateInstance<true, const UserClass, Mode, Ret(Args...), VarTypes...>;
		new (Allocate(sizeof(FInstance))) FInstance(InUserObjectRef, InFunc, Vars...);
	}

	template<typename UserClass, typename... VarTypes>
	void BindSP(UserClass* InUserObject, typename DelegatePrivate::TMemFunPtrType<false, UserClass, Ret(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		BindSP(StaticCastSharedRef<UserClass>(InUserObject->AsShared()), InFunc, Vars...);
	}

	template<typename UserClass, typename... VarTypes>
	void BindSP(const UserClass* InUserObject, typename DelegatePrivate::TMemFunPtrType<true, UserClass, Ret(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		BindSP(StaticCastSharedRef<const UserClass>(InUserObject->AsShared()), InFunc, Vars...);
	}

	template<typename UserClass, typename... VarTypes>
	void BindUObject(UserClass* InUserObject, typename DelegatePrivate::TMemFunPtrType<false, UserClass, Ret(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		using FInstance = TBaseUObjectMethodDelegateInstance<false, UserClass, Ret(Args...), VarTypes...>;
		new (Allocate(sizeof(FInstance))) FInstance(InUserObject, InFunc, Vars...);
	}

	template<typename UserClass, typename... VarTypes>
	void BindUObject(const UserClass* InUserObject, typename DelegatePrivate::TMemFunPtrType<true, UserClass, Ret(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		using FInstance = TBaseUObjectMethodDelegateInstance<true, const UserClass, Ret(Args...), VarTypes...>;
		new (Allocate(sizeof(FInstance))) FInstance(InUserObject, InFunc, Vars...);
	}

	template<typename UserClass, typename... VarTypes>
	void BindUFunction(UserClass* InUserObject, const FName& InFunctionName, VarTypes... Vars)
	{
		using FInstance = TBaseUFunctionDelegateInstance<UserClass, Ret(Args...), VarTypes...>;
		new (Allocate(sizeof(FInstance))) FInstance(InUserObject, InFunctionName, Vars...);
	}

public:
	bool IsBound() const
	{
		IDelegateInstance* Instance = GetDelegateInstanceProtected();
		return Instance && Instance->IsSafeToExecute();
	}

	bool IsBoundToObject(const void* InUserObject) const
	{
		if (!InUserObject)
			return false;

		IDelegateInstance* Instance = GetDelegateInstanceProtected();
		return Instance && Instance->HasSameObject(InUserObject);
	}

	Ret Execute(Args... InArgs) const
	{
		return GetInstance()->Execute(std::forward<Args>(InArgs)...);
	}

	bool ExecuteIfBound(Args... InArgs) const requires std::is_void_v<Ret>
	{
		if (FDelegateInstanceInterface* Instance = GetInstance())
			return Instance->ExecuteIfSafe(std::forward<Args>(InArgs)...);

		return false;
	}

private:
	FDelegateInstanceInterface* GetInstance() const
	{
		return static_cast<FDelegateInstanceInterface*>(GetDelegateInstanceProtected());
	}
};

/**
 * Declares a delegate that can only bind to one native function at a time
 *
 * @note: The last parameter is variadic and is used as the 'template args' for this delegate's classes (__VA_ARGS__)
 * @note: To avoid issues with macro expansion breaking code navigation, make sure the type/class name macro params are unique across all of these macros
 */
#define FUNC_DECLARE_DELEGATE( DelegateName, ReturnType, ... ) \
	typedef TBaseDelegate<ReturnType(__VA_ARGS__)> DelegateName;

#define DECLARE_DELEGATE( DelegateName ) FUNC_DECLARE_DELEGATE( DelegateName, void )

class FMulticastDelegateBase
{
public:
	void Clear()
	{
		for (int32 Index = 0; Index < InvocationList.Num(); Index++)
			InvocationList[Index].Unbind();

		CompactInvocationList(false);
	}

	bool IsBound() const
	{
		for (int32 Index = 0; Index < InvocationList.Num(); Index++)
		{
			if (InvocationList[Index].GetDelegateInstanceProtected())
				return true;
		}

		return false;
	}

	bool IsBoundToObject(const void* InUserObject) const
	{
		for (int32 Index = 0; Index < InvocationList.Num(); Index++)
		{
			IDelegateInstance* Instance = InvocationList[Index].GetDelegateInstanceProtected();

			if (Instance && Instance->HasSameObject(InUserObject))
				return true;
		}

		return false;
	}

	int32 RemoveAll(const void* InUserObject)
	{
		int32 Result = 0;

		if (InvocationListLockCount > 0)
		{
			for (int32 Index = 0; Index < InvocationList.Num(); Index++)
			{
				IDelegateInstance* Instance = InvocationList[Index].GetDelegateInstanceProtected();

				if (Instance && Instance->HasSameObject(InUserObject))
				{
					InvocationList[Index].Unbind();
					Result++;
				}
			}

			if (Result > 0)
				CompactionThreshold = 0;
		}
		else
		{
			for (int32 Index = 0; Index < InvocationList.Num();)
			{
				IDelegateInstance* Instance = InvocationList[Index].GetDelegateInstanceProtected();

				if (!Instance || Instance->HasSameObject(InUserObject) || Instance->IsCompactable())
				{
					InvocationList.RemoveAtSwap(Index);
					Result++;
				}
				else
				{
					Index++;
				}
			}

			CompactionThreshold = InvocationList.Num() * 2 > 2 ? InvocationList.Num() * 2 : 2;
			InvocationList.Shrink();
		}

		return Result;
	}

protected:
	FMulticastDelegateBase()
		: CompactionThreshold(2), InvocationListLockCount(0)
	{
	}

	FDelegateHandle AddDelegateInstance(FDelegateBase&& NewDelegateBaseRef)
	{
		FDelegateHandle Result;

		if (NewDelegateBaseRef.GetDelegateInstanceProtected())
		{
			CompactInvocationList(true);
			Result = NewDelegateBaseRef.GetHandle();
			InvocationList.Add(std::move(NewDelegateBaseRef));
		}

		return Result;
	}

	bool RemoveDelegateInstance(FDelegateHandle Handle)
	{
		for (int32 Index = 0; Index < InvocationList.Num(); Index++)
		{
			IDelegateInstance* Instance = InvocationList[Index].GetDelegateInstanceProtected();

			if (Instance && Instance->GetHandle() == Handle)
			{
				InvocationList[Index].Unbind();
				CompactInvocationList();
				return true;
			}
		}

		return false;
	}

	void CompactInvocationList(bool CheckThreshold = false)
	{
		if (InvocationListLockCount > 0)
			return;

		if (CheckThreshold && --CompactionThreshold > InvocationList.Num())
			return;

		const int32 OldNumItems = InvocationList.Num();

		for (int32 Index = 0; Index < InvocationList.Num();)
		{
			IDelegateInstance* Instance = InvocationList[Index].GetDelegateInstanceProtected();

			if (!Instance || Instance->IsCompactable())
				InvocationList.RemoveAtSwap(Index);
			else
				Index++;
		}

		CompactionThreshold = InvocationList.Num() * 2 > 2 ? InvocationList.Num() * 2 : 2;

		if (OldNumItems > CompactionThreshold)
			InvocationList.Shrink();
	}

	void LockInvocationList() const
	{
		InvocationListLockCount++;
	}

	void UnlockInvocationList() const
	{
		InvocationListLockCount--;
	}

protected:
	TArray<FDelegateBase> InvocationList;
	int32 CompactionThreshold;
	mutable int32 InvocationListLockCount;
};

template<typename FuncType>
class TMulticastDelegate;

template<typename... Args>
class TMulticastDelegate<void(Args...)> : public FMulticastDelegateBase
{
	using FDelegateInstanceInterface = IBaseDelegateInstance<void(Args...)>;

public:
	using FDelegate = TBaseDelegate<void(Args...)>;

	FDelegateHandle Add(FDelegate&& InNewDelegate)
	{
		return AddDelegateInstance(std::move(InNewDelegate));
	}

	FDelegateHandle Add(const FDelegate& InNewDelegate)
	{
		return AddDelegateInstance(FDelegate(InNewDelegate));
	}

	template<typename... VarTypes>
	FDelegateHandle AddStatic(typename TBaseStaticDelegateInstance<void(Args...), VarTypes...>::FFuncPtr InFunc, VarTypes... Vars)
	{
		return AddDelegateInstance(FDelegate::CreateStatic(InFunc, Vars...));
	}

	template<typename FunctorType, typename... VarTypes>
	FDelegateHandle AddLambda(FunctorType&& InFunctor, VarTypes... Vars)
	{
		return AddDelegateInstance(FDelegate::CreateLambda(std::forward<FunctorType>(InFunctor), Vars...));
	}

	template<typename UserClass, typename FunctorType, typename... VarTypes>
	FDelegateHandle AddWeakLambda(UserClass* InUserObject, FunctorType&& InFunctor, VarTypes... Vars)
	{
		return AddDelegateInstance(FDelegate::CreateWeakLambda(InUserObject, std::forward<FunctorType>(InFunctor), Vars...));
	}

	template<typename UserClass, typename... VarTypes>
	FDelegateHandle AddRaw(UserClass* InUserObject, typename DelegatePrivate::TMemFunPtrType<false, UserClass, void(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		return AddDelegateInstance(FDelegate::CreateRaw(InUserObject, InFunc, Vars...));
	}

	template<typename UserClass, typename... VarTypes>
	FDelegateHandle AddRaw(const UserClass* InUserObject, typename DelegatePrivate::TMemFunPtrType<true, UserClass, void(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		return AddDelegateInstance(FDelegate::CreateRaw(InUserObject, InFunc, Vars...));
	}

	template<typename UserClass, ESPMode Mode, typename... VarTypes>
	FDelegateHandle AddSP(const TSharedRef<UserClass, Mode>& InUserObjectRef, typename DelegatePrivate::TMemFunPtrType<false, UserClass, void(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		return AddDelegateInstance(FDelegate::CreateSP(InUserObjectRef, InFunc, Vars...));
	}

	template<typename UserClass, ESPMode Mode, typename... VarTypes>
	FDelegateHandle AddSP(const TSharedRef<UserClass, Mode>& InUserObjectRef, typename DelegatePrivate::TMemFunPtrType<true, UserClass, void(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		return AddDelegateInstance(FDelegate::CreateSP(InUserObjectRef, InFunc, Vars...));
	}

	template<typename UserClass, typename... VarTypes>
	FDelegateHandle AddSP(UserClass* InUserObject, typename DelegatePrivate::TMemFunPtrType<false, UserClass, void(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		return AddDelegateInstance(FDelegate::CreateSP(InUserObject, InFunc, Vars...));
	}

	template<typename UserClass, typename... VarTypes>
	FDelegateHandle AddSP(const UserClass* InUserObject, typename DelegatePrivate::TMemFunPtrType<true, UserClass, void(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		return AddDelegateInstance(FDelegate::CreateSP(InUserObject, InFunc, Vars...));
	}

	template<typename UserClass, typename... VarTypes>
	FDelegateHandle AddUObject(UserClass* InUserObject, typename DelegatePrivate::TMemFunPtrType<false, UserClass, void(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		return AddDelegateInstance(FDelegate::CreateUObject(InUserObject, InFunc, Vars...));
	}

	template<typename UserClass, typename... VarTypes>
	FDelegateHandle AddUObject(const UserClass* InUserObject, typename DelegatePrivate::TMemFunPtrType<true, UserClass, void(Args..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		return AddDelegateInstance(FDelegate::CreateUObject(InUserObject, InFunc, Vars...));
	}

	template<typename UserClass, typename... VarTypes>
	FDelegateHandle AddUFunction(UserClass* InUserObject, const FName& InFunctionName, VarTypes... Vars)
	{
		return AddDelegateInstance(FDelegate::CreateUFunction(InUserObject, InFunctionName, Vars...));
	}

	bool Remove(FDelegateHandle Handle)
	{
		return RemoveDelegateInstance(Handle);
	}

	void Broadcast(Args... InArgs) const
	{
		bool NeedsCompaction = false;

		LockInvocationList();

		for (int32 Index = InvocationList.Num() - 1; Index >= 0; Index--)
		{
			IDelegateInstance* Instance = InvocationList[Index].GetDelegateInstanceProtected();

			if (!Instance || !static_cast<FDelegateInstanceInterface*>(Instance)->ExecuteIfSafe(InArgs...))
				NeedsCompaction = true;
		}

		UnlockInvocationList();

		if (NeedsCompaction)
			const_cast<TMulticastDelegate*>(this)->CompactInvocationList();
	}
};

class FScriptDelegate
{
public:
	FWeakObjectPtr Object;
	FName FunctionName;

public:
	void BindUFunction(class UObject* InObject, const FName& InFunctionName)
	{
		Object = InObject;
		FunctionName = InFunctionName;
	}

	bool IsBound() const
	{
		if (FunctionName.IsNone())
			return false;

		class UObject* ObjectPtr = Object.Get();
		return ObjectPtr && DelegatePrivate::FindFunction(ObjectPtr, FunctionName);
	}

	bool IsBoundToObject(const void* InUserObject) const
	{
		return InUserObject && InUserObject == GetUObject();
	}

	bool IsCompactable() const
	{
		return FunctionName.IsNone() || !Object.Get(true);
	}

	void Unbind()
	{
		Object.Reset();
		FunctionName = FName();
	}

	void Clear()
	{
		Unbind();
	}

	class UObject* GetUObject() const
	{
		return Object.Get();
	}

	FName GetFunctionName() const
	{
		return FunctionName;
	}

	void ProcessDelegate(void* Parameters) const
	{
		class UObject* ObjectPtr = Object.Get();
		DelegatePrivate::ProcessEvent(ObjectPtr, DelegatePrivate::FindFunction(ObjectPtr, FunctionName), Parameters);
	}

	template <class UObjectTemplate>
	void ProcessDelegate( void* Parameters ) const
	{
		// Object was pending kill, so we cannot execute the delegate.  Note that it's important to assert
		// here and not simply continue execution, as memory may be left uninitialized if the delegate is
		// not able to execute, resulting in much harder-to-detect code errors.  Users should always make
		// sure IsBound() returns true before calling ProcessDelegate()!
		UObjectTemplate* ObjectPtr = static_cast< UObjectTemplate* >( Object.Get() );	// Down-cast

		// Object *must* implement the specified function
		UFunction* Function = ObjectPtr->FindFunctionChecked( FunctionName );

		// Execute the delegate!
		ObjectPtr->ProcessEvent(Function, Parameters);
	}

	FORCEINLINE bool operator==( const FScriptDelegate& Other ) const
	{
		return Object == Other.Object && FunctionName == Other.FunctionName;
	}

	bool operator!=(const FScriptDelegate& Other) const
	{
		return !(*this == Other);
	}
};
DUMPER7_ASSERTS_FScriptDelegate;

class FMulticastScriptDelegate
{
public:
	TArray<FScriptDelegate> InvocationList;

public:
	bool IsBound() const
	{
		return InvocationList.Num() > 0;
	}

	bool Contains(const FScriptDelegate& InDelegate) const
	{
		return InvocationList.Contains(InDelegate);
	}

	bool Contains(const class UObject* InObject, FName InFunctionName) const
	{
		for (int32 Index = 0; Index < InvocationList.Num(); Index++)
		{
			if (InvocationList[Index].FunctionName == InFunctionName && InvocationList[Index].IsBoundToObject(InObject))
				return true;
		}

		return false;
	}

	void Add(const FScriptDelegate& InDelegate)
	{
		CompactInvocationList();
		InvocationList.Add(InDelegate);
	}

	void AddUnique(const FScriptDelegate& InDelegate)
	{
		CompactInvocationList();

		if (!InvocationList.Contains(InDelegate))
			InvocationList.Add(InDelegate);
	}

	void Remove(const FScriptDelegate& InDelegate)
	{
		for (int32 Index = 0; Index < InvocationList.Num(); Index++)
		{
			if (InvocationList[Index] == InDelegate)
			{
				InvocationList.RemoveAtSwap(Index);
				break;
			}
		}

		CompactInvocationList();
	}

	void Remove(const class UObject* InObject, FName InFunctionName)
	{
		for (int32 Index = 0; Index < InvocationList.Num(); Index++)
		{
			if (InvocationList[Index].FunctionName == InFunctionName && InvocationList[Index].IsBoundToObject(InObject))
			{
				InvocationList.RemoveAtSwap(Index);
				break;
			}
		}

		CompactInvocationList();
	}

	void RemoveAll(const class UObject* InObject)
	{
		for (int32 Index = InvocationList.Num() - 1; Index >= 0; Index--)
		{
			if (InvocationList[Index].IsBoundToObject(InObject) || InvocationList[Index].IsCompactable())
				InvocationList.RemoveAtSwap(Index);
		}
	}

	void Clear()
	{
		InvocationList.Empty();
	}

	void ProcessMulticastDelegate(void* Parameters) const
	{
		if (InvocationList.Num() <= 0)
			return;

		TArray<FScriptDelegate> InvocationListCopy = InvocationList;

		for (int32 Index = 0; Index < InvocationListCopy.Num(); Index++)
		{
			if (class UObject* ObjectPtr = InvocationListCopy[Index].GetUObject())
				DelegatePrivate::ProcessEvent(ObjectPtr, DelegatePrivate::FindFunction(ObjectPtr, InvocationListCopy[Index].FunctionName), Parameters);
		}
	}

private:
	void CompactInvocationList()
	{
		for (int32 Index = InvocationList.Num() - 1; Index >= 0; Index--)
		{
			if (InvocationList[Index].IsCompactable())
				InvocationList.RemoveAtSwap(Index);
		}
	}
};

template<typename FunctionSignature>
class TDelegate : public FScriptDelegate
{
};

template<typename Ret, typename... Args>
class TDelegate<Ret(Args...)> : public FScriptDelegate
{
public:
	Ret Execute(Args... InArgs) const
	{
		return DelegatePrivate::TParms<Ret, Args...>::Invoke([this](void* Parms) { ProcessDelegate(Parms); }, std::forward<Args>(InArgs)...);
	}

	bool ExecuteIfBound(Args... InArgs) const requires std::is_void_v<Ret>
	{
		if (!IsBound())
			return false;

		Execute(std::forward<Args>(InArgs)...);
		return true;
	}
};

template<typename FunctionSignature>
class TMulticastInlineDelegate : public FMulticastScriptDelegate
{
};

template<typename Ret, typename... Args>
class TMulticastInlineDelegate<Ret(Args...)> : public FMulticastScriptDelegate
{
public:
	void Broadcast(Args... InArgs) const
	{
		DelegatePrivate::TParms<void, Args...>::Invoke([this](void* Parms) { ProcessMulticastDelegate(Parms); }, std::forward<Args>(InArgs)...);
	}
};

class FSparseDelegateStorage
{
public:
	static bool Add(const class UObject* DelegateOwner, FName DelegateName, FScriptDelegate Delegate)
	{
		return InSDKUtils::CallGameFunction(reinterpret_cast<bool(*)(const class UObject*, FName, FScriptDelegate)>(InSDKUtils::GetImageBase() + 0x35147F0), DelegateOwner, DelegateName, Delegate);
	}

	static bool AddUnique(const class UObject* DelegateOwner, FName DelegateName, FScriptDelegate Delegate)
	{
		return InSDKUtils::CallGameFunction(reinterpret_cast<bool(*)(const class UObject*, FName, FScriptDelegate)>(InSDKUtils::GetImageBase() + 0x3515FC0), DelegateOwner, DelegateName, Delegate);
	}

	static bool Contains(const class UObject* DelegateOwner, FName DelegateName, const FScriptDelegate& Delegate)
	{
		FMulticastScriptDelegate* MulticastDelegate = GetMulticastDelegate(DelegateOwner, DelegateName);
		return MulticastDelegate && MulticastDelegate->Contains(Delegate);
	}

	static bool Contains(const class UObject* DelegateOwner, FName DelegateName, const class UObject* InObject, FName InFunctionName)
	{
		return InSDKUtils::CallGameFunction(reinterpret_cast<bool(*)(const class UObject*, FName, const class UObject*, FName)>(InSDKUtils::GetImageBase() + 0x351AFB0), DelegateOwner, DelegateName, InObject, InFunctionName);
	}

	static bool Remove(const class UObject* DelegateOwner, FName DelegateName, const FScriptDelegate& Delegate)
	{
		return InSDKUtils::CallGameFunction(reinterpret_cast<bool(*)(const class UObject*, FName, const FScriptDelegate&)>(InSDKUtils::GetImageBase() + 0x3529FE0), DelegateOwner, DelegateName, Delegate);
	}

	static bool Remove(const class UObject* DelegateOwner, FName DelegateName, const class UObject* InObject, FName InFunctionName)
	{
		return InSDKUtils::CallGameFunction(reinterpret_cast<bool(*)(const class UObject*, FName, const class UObject*, FName)>(InSDKUtils::GetImageBase() + 0x3529D80), DelegateOwner, DelegateName, InObject, InFunctionName);
	}

	static bool RemoveAll(const class UObject* DelegateOwner, FName DelegateName, const class UObject* UserObject)
	{
		return InSDKUtils::CallGameFunction(reinterpret_cast<bool(*)(const class UObject*, FName, const class UObject*)>(InSDKUtils::GetImageBase() + 0x352A180), DelegateOwner, DelegateName, UserObject);
	}

	static void Clear(const class UObject* DelegateOwner, FName DelegateName)
	{
		InSDKUtils::CallGameFunction(reinterpret_cast<void(*)(const class UObject*, FName)>(InSDKUtils::GetImageBase() + 0x3517CC0), DelegateOwner, DelegateName);
	}

	static FMulticastScriptDelegate* GetMulticastDelegate(const class UObject* DelegateOwner, FName DelegateName)
	{
		return InSDKUtils::CallGameFunction(reinterpret_cast<FMulticastScriptDelegate*(*)(const class UObject*, FName)>(InSDKUtils::GetImageBase() + 0x351FCB0), DelegateOwner, DelegateName);
	}

	static TSharedPtr<FMulticastScriptDelegate> GetSharedMulticastDelegate(const class UObject* DelegateOwner, FName DelegateName)
	{
		return InSDKUtils::CallGameFunction(reinterpret_cast<TSharedPtr<FMulticastScriptDelegate>(*)(const class UObject*, FName)>(InSDKUtils::GetImageBase() + 0x3520540), DelegateOwner, DelegateName);
	}

	static void SetMulticastDelegate(const class UObject* DelegateOwner, FName DelegateName, FMulticastScriptDelegate Delegate)
	{
		InSDKUtils::CallGameFunction(reinterpret_cast<void(*)(const class UObject*, FName, FMulticastScriptDelegate)>(InSDKUtils::GetImageBase() + 0x352C2F0), DelegateOwner, DelegateName, std::move(Delegate));
	}
};

class FSparseDelegate
{
public:
	bool bIsBound;

public:
	bool IsBound() const
	{
		return bIsBound;
	}

	FName GetDelegateName(const class UObject* DelegateOwner) const;

	void Add(const class UObject* DelegateOwner, FScriptDelegate InDelegate)
	{
		bIsBound |= FSparseDelegateStorage::Add(DelegateOwner, GetDelegateName(DelegateOwner), InDelegate);
	}

	void AddUnique(const class UObject* DelegateOwner, FScriptDelegate InDelegate)
	{
		bIsBound |= FSparseDelegateStorage::AddUnique(DelegateOwner, GetDelegateName(DelegateOwner), InDelegate);
	}

	bool Contains(const class UObject* DelegateOwner, const FScriptDelegate& InDelegate) const
	{
		return bIsBound && FSparseDelegateStorage::Contains(DelegateOwner, GetDelegateName(DelegateOwner), InDelegate);
	}

	bool Contains(const class UObject* DelegateOwner, const class UObject* InObject, FName InFunctionName) const
	{
		return bIsBound && FSparseDelegateStorage::Contains(DelegateOwner, GetDelegateName(DelegateOwner), InObject, InFunctionName);
	}

	void Remove(const class UObject* DelegateOwner, const FScriptDelegate& InDelegate)
	{
		if (bIsBound)
			bIsBound = FSparseDelegateStorage::Remove(DelegateOwner, GetDelegateName(DelegateOwner), InDelegate);
	}

	void Remove(const class UObject* DelegateOwner, const class UObject* InObject, FName InFunctionName)
	{
		if (bIsBound)
			bIsBound = FSparseDelegateStorage::Remove(DelegateOwner, GetDelegateName(DelegateOwner), InObject, InFunctionName);
	}

	void RemoveAll(const class UObject* DelegateOwner, const class UObject* UserObject)
	{
		if (bIsBound)
			bIsBound = FSparseDelegateStorage::RemoveAll(DelegateOwner, GetDelegateName(DelegateOwner), UserObject);
	}

	void Clear(const class UObject* DelegateOwner)
	{
		if (!bIsBound)
			return;

		FSparseDelegateStorage::Clear(DelegateOwner, GetDelegateName(DelegateOwner));
		bIsBound = false;
	}

	FMulticastScriptDelegate* GetMulticastDelegate(const class UObject* DelegateOwner) const
	{
		return bIsBound ? FSparseDelegateStorage::GetMulticastDelegate(DelegateOwner, GetDelegateName(DelegateOwner)) : nullptr;
	}

	void ProcessMulticastDelegate(const class UObject* DelegateOwner, void* Parameters) const
	{
		if (FMulticastScriptDelegate* MulticastDelegate = GetMulticastDelegate(DelegateOwner))
			MulticastDelegate->ProcessMulticastDelegate(Parameters);
	}
};

class alignas(0x01) FMulticastSparseDelegateProperty_ : public FSparseDelegate
{
};
