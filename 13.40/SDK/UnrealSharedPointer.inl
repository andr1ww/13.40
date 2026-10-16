#pragma once

enum class ESPMode : uint8
{
	NotThreadSafe = 0,
	Fast = NotThreadSafe,
	ThreadSafe = 1
};

template<typename ObjectType, ESPMode Mode = ESPMode::Fast> class TSharedRef;
template<typename ObjectType, ESPMode Mode = ESPMode::Fast> class TSharedPtr;
template<typename ObjectType, ESPMode Mode = ESPMode::Fast> class TWeakPtr;
template<typename ObjectType, ESPMode Mode = ESPMode::Fast> class TSharedFromThis;

namespace SharedPointerInternals
{
	class FReferenceControllerBase
	{
	public:
		FReferenceControllerBase()
			: SharedReferenceCount(1), WeakReferenceCount(1)
		{
		}

		FReferenceControllerBase(const FReferenceControllerBase&) = delete;
		FReferenceControllerBase& operator=(const FReferenceControllerBase&) = delete;

		virtual void DestroyObject() = 0;
		virtual ~FReferenceControllerBase() = default;

		static void* operator new(size_t Size)
		{
			return FMemory::Realloc(nullptr, Size);
		}

		static void operator delete(void* Ptr)
		{
			FMemory::Free(Ptr);
		}

	public:
		int32 SharedReferenceCount;
		int32 WeakReferenceCount;
	};

	template<typename ObjectType, typename DeleterType>
	class TReferenceControllerWithDeleter : public FReferenceControllerBase
	{
	public:
		template<typename InDeleterType>
		explicit TReferenceControllerWithDeleter(ObjectType* InObject, InDeleterType&& InDeleter)
			: Object(InObject), Deleter(std::forward<InDeleterType>(InDeleter))
		{
		}

		void DestroyObject() override
		{
			Deleter(Object);
		}

	private:
		ObjectType* Object;
		DeleterType Deleter;
	};

	template<typename ObjectType>
	class TIntrusiveReferenceController : public FReferenceControllerBase
	{
	public:
		template<typename... ArgTypes>
		explicit TIntrusiveReferenceController(ArgTypes&&... Args)
		{
			new (GetObjectPtr()) ObjectType(std::forward<ArgTypes>(Args)...);
		}

		ObjectType* GetObjectPtr() const
		{
			return reinterpret_cast<ObjectType*>(const_cast<uint8*>(ObjectStorage));
		}

		void DestroyObject() override
		{
			GetObjectPtr()->~ObjectType();
		}

	private:
		alignas(ObjectType) uint8 ObjectStorage[sizeof(ObjectType)];
	};

	template<typename Type>
	struct DefaultDeleter
	{
		void operator()(Type* Object) const
		{
			delete Object;
		}
	};

	template<typename ObjectType>
	FReferenceControllerBase* NewDefaultReferenceController(ObjectType* Object)
	{
		return new TReferenceControllerWithDeleter<ObjectType, DefaultDeleter<ObjectType>>(Object, DefaultDeleter<ObjectType>());
	}

	template<typename ObjectType, typename DeleterType>
	FReferenceControllerBase* NewCustomReferenceController(ObjectType* Object, DeleterType&& Deleter)
	{
		return new TReferenceControllerWithDeleter<ObjectType, std::remove_reference_t<DeleterType>>(Object, std::forward<DeleterType>(Deleter));
	}

	template<ESPMode Mode>
	struct FReferenceControllerOps;

	template<>
	struct FReferenceControllerOps<ESPMode::ThreadSafe>
	{
		static int32 GetSharedReferenceCount(const FReferenceControllerBase* ReferenceController)
		{
			return static_cast<const volatile int32&>(ReferenceController->SharedReferenceCount);
		}

		static void AddSharedReference(FReferenceControllerBase* ReferenceController)
		{
			_InterlockedIncrement(reinterpret_cast<volatile long*>(&ReferenceController->SharedReferenceCount));
		}

		static bool ConditionallyAddSharedReference(FReferenceControllerBase* ReferenceController)
		{
			for (;;)
			{
				const int32 OriginalCount = static_cast<const volatile int32&>(ReferenceController->SharedReferenceCount);

				if (OriginalCount == 0)
					return false;

				if (_InterlockedCompareExchange(reinterpret_cast<volatile long*>(&ReferenceController->SharedReferenceCount), OriginalCount + 1, OriginalCount) == OriginalCount)
					return true;
			}
		}

		static void ReleaseSharedReference(FReferenceControllerBase* ReferenceController)
		{
			if (_InterlockedDecrement(reinterpret_cast<volatile long*>(&ReferenceController->SharedReferenceCount)) == 0)
			{
				ReferenceController->DestroyObject();
				ReleaseWeakReference(ReferenceController);
			}
		}

		static void AddWeakReference(FReferenceControllerBase* ReferenceController)
		{
			_InterlockedIncrement(reinterpret_cast<volatile long*>(&ReferenceController->WeakReferenceCount));
		}

		static void ReleaseWeakReference(FReferenceControllerBase* ReferenceController)
		{
			if (_InterlockedDecrement(reinterpret_cast<volatile long*>(&ReferenceController->WeakReferenceCount)) == 0)
				delete ReferenceController;
		}
	};

	template<>
	struct FReferenceControllerOps<ESPMode::NotThreadSafe>
	{
		static int32 GetSharedReferenceCount(const FReferenceControllerBase* ReferenceController)
		{
			return ReferenceController->SharedReferenceCount;
		}

		static void AddSharedReference(FReferenceControllerBase* ReferenceController)
		{
			++ReferenceController->SharedReferenceCount;
		}

		static bool ConditionallyAddSharedReference(FReferenceControllerBase* ReferenceController)
		{
			if (ReferenceController->SharedReferenceCount == 0)
				return false;

			++ReferenceController->SharedReferenceCount;
			return true;
		}

		static void ReleaseSharedReference(FReferenceControllerBase* ReferenceController)
		{
			if (--ReferenceController->SharedReferenceCount == 0)
			{
				ReferenceController->DestroyObject();
				ReleaseWeakReference(ReferenceController);
			}
		}

		static void AddWeakReference(FReferenceControllerBase* ReferenceController)
		{
			++ReferenceController->WeakReferenceCount;
		}

		static void ReleaseWeakReference(FReferenceControllerBase* ReferenceController)
		{
			if (--ReferenceController->WeakReferenceCount == 0)
				delete ReferenceController;
		}
	};

	template<ESPMode Mode>
	class FWeakReferencer;

	template<ESPMode Mode>
	class FSharedReferencer
	{
		using TOps = FReferenceControllerOps<Mode>;

	public:
		FSharedReferencer()
			: ReferenceController(nullptr)
		{
		}

		explicit FSharedReferencer(FReferenceControllerBase* InReferenceController)
			: ReferenceController(InReferenceController)
		{
		}

		FSharedReferencer(const FSharedReferencer& InSharedReference)
			: ReferenceController(InSharedReference.ReferenceController)
		{
			if (ReferenceController)
				TOps::AddSharedReference(ReferenceController);
		}

		FSharedReferencer(FSharedReferencer&& InSharedReference)
			: ReferenceController(InSharedReference.ReferenceController)
		{
			InSharedReference.ReferenceController = nullptr;
		}

		FSharedReferencer(const FWeakReferencer<Mode>& InWeakReference)
			: ReferenceController(InWeakReference.ReferenceController)
		{
			if (ReferenceController && !TOps::ConditionallyAddSharedReference(ReferenceController))
				ReferenceController = nullptr;
		}

		~FSharedReferencer()
		{
			if (ReferenceController)
				TOps::ReleaseSharedReference(ReferenceController);
		}

		FSharedReferencer& operator=(const FSharedReferencer& InSharedReference)
		{
			FReferenceControllerBase* NewReferenceController = InSharedReference.ReferenceController;

			if (NewReferenceController != ReferenceController)
			{
				if (NewReferenceController)
					TOps::AddSharedReference(NewReferenceController);

				if (ReferenceController)
					TOps::ReleaseSharedReference(ReferenceController);

				ReferenceController = NewReferenceController;
			}

			return *this;
		}

		FSharedReferencer& operator=(FSharedReferencer&& InSharedReference)
		{
			FReferenceControllerBase* NewReferenceController = InSharedReference.ReferenceController;
			FReferenceControllerBase* OldReferenceController = ReferenceController;

			if (NewReferenceController != OldReferenceController)
			{
				InSharedReference.ReferenceController = nullptr;
				ReferenceController = NewReferenceController;

				if (OldReferenceController)
					TOps::ReleaseSharedReference(OldReferenceController);
			}

			return *this;
		}

		bool IsValid() const
		{
			return ReferenceController != nullptr;
		}

		int32 GetSharedReferenceCount() const
		{
			return ReferenceController ? TOps::GetSharedReferenceCount(ReferenceController) : 0;
		}

		bool IsUnique() const
		{
			return GetSharedReferenceCount() == 1;
		}

	private:
		friend class FWeakReferencer<Mode>;

		FReferenceControllerBase* ReferenceController;
	};

	template<ESPMode Mode>
	class FWeakReferencer
	{
		using TOps = FReferenceControllerOps<Mode>;

	public:
		FWeakReferencer()
			: ReferenceController(nullptr)
		{
		}

		FWeakReferencer(const FWeakReferencer& InWeakRefCountPointer)
			: ReferenceController(InWeakRefCountPointer.ReferenceController)
		{
			if (ReferenceController)
				TOps::AddWeakReference(ReferenceController);
		}

		FWeakReferencer(FWeakReferencer&& InWeakRefCountPointer)
			: ReferenceController(InWeakRefCountPointer.ReferenceController)
		{
			InWeakRefCountPointer.ReferenceController = nullptr;
		}

		FWeakReferencer(const FSharedReferencer<Mode>& InSharedRefCountPointer)
			: ReferenceController(InSharedRefCountPointer.ReferenceController)
		{
			if (ReferenceController)
				TOps::AddWeakReference(ReferenceController);
		}

		~FWeakReferencer()
		{
			if (ReferenceController)
				TOps::ReleaseWeakReference(ReferenceController);
		}

		FWeakReferencer& operator=(const FWeakReferencer& InWeakReference)
		{
			AssignReferenceController(InWeakReference.ReferenceController);
			return *this;
		}

		FWeakReferencer& operator=(FWeakReferencer&& InWeakReference)
		{
			FReferenceControllerBase* OldReferenceController = ReferenceController;
			ReferenceController = InWeakReference.ReferenceController;
			InWeakReference.ReferenceController = nullptr;

			if (OldReferenceController)
				TOps::ReleaseWeakReference(OldReferenceController);

			return *this;
		}

		FWeakReferencer& operator=(const FSharedReferencer<Mode>& InSharedReference)
		{
			AssignReferenceController(InSharedReference.ReferenceController);
			return *this;
		}

		bool IsValid() const
		{
			return ReferenceController && TOps::GetSharedReferenceCount(ReferenceController) > 0;
		}

	private:
		void AssignReferenceController(FReferenceControllerBase* NewReferenceController)
		{
			if (NewReferenceController != ReferenceController)
			{
				if (NewReferenceController)
					TOps::AddWeakReference(NewReferenceController);

				if (ReferenceController)
					TOps::ReleaseWeakReference(ReferenceController);

				ReferenceController = NewReferenceController;
			}
		}

		friend class FSharedReferencer<Mode>;

		FReferenceControllerBase* ReferenceController;
	};

	template<typename ObjectType>
	struct FRawPtrProxy
	{
		ObjectType* Object;
		FReferenceControllerBase* ReferenceController;

		FRawPtrProxy(std::nullptr_t)
			: Object(nullptr), ReferenceController(nullptr)
		{
		}

		explicit FRawPtrProxy(ObjectType* InObject)
			: Object(InObject), ReferenceController(NewDefaultReferenceController(InObject))
		{
		}

		template<typename DeleterType>
		explicit FRawPtrProxy(ObjectType* InObject, DeleterType&& Deleter)
			: Object(InObject), ReferenceController(NewCustomReferenceController(InObject, std::forward<DeleterType>(Deleter)))
		{
		}
	};

	template<typename SharedPtrType, typename OtherType, typename ObjectType, ESPMode Mode>
	void EnableSharedFromThis(const TSharedPtr<SharedPtrType, Mode>* InSharedPtr, const OtherType* InObject, const TSharedFromThis<ObjectType, Mode>* InShareable)
	{
		if (InShareable)
			InShareable->UpdateWeakReferenceInternal(InSharedPtr, const_cast<OtherType*>(InObject));
	}

	template<typename SharedRefType, typename OtherType, typename ObjectType, ESPMode Mode>
	void EnableSharedFromThis(const TSharedRef<SharedRefType, Mode>* InSharedRef, const OtherType* InObject, const TSharedFromThis<ObjectType, Mode>* InShareable)
	{
		if (InShareable)
			InShareable->UpdateWeakReferenceInternal(InSharedRef, const_cast<OtherType*>(InObject));
	}

	inline void EnableSharedFromThis(...)
	{
	}
}

template<typename ObjectType, ESPMode Mode>
class TSharedRef
{
public:
	using ElementType = ObjectType;

	template<typename OtherType>
	explicit TSharedRef(OtherType* InObject)
		: Object(InObject), SharedReferenceCount(SharedPointerInternals::NewDefaultReferenceController(InObject))
	{
		SharedPointerInternals::EnableSharedFromThis(this, InObject, InObject);
	}

	template<typename OtherType, typename DeleterType>
	TSharedRef(OtherType* InObject, DeleterType&& InDeleter)
		: Object(InObject), SharedReferenceCount(SharedPointerInternals::NewCustomReferenceController(InObject, std::forward<DeleterType>(InDeleter)))
	{
		SharedPointerInternals::EnableSharedFromThis(this, InObject, InObject);
	}

	template<typename OtherType>
	TSharedRef(const SharedPointerInternals::FRawPtrProxy<OtherType>& InRawPtrProxy)
		: Object(InRawPtrProxy.Object), SharedReferenceCount(InRawPtrProxy.ReferenceController)
	{
		SharedPointerInternals::EnableSharedFromThis(this, InRawPtrProxy.Object, InRawPtrProxy.Object);
	}

	TSharedRef(ObjectType* InObject, SharedPointerInternals::FReferenceControllerBase* InSharedReferenceCount)
		: Object(InObject), SharedReferenceCount(InSharedReferenceCount)
	{
		SharedPointerInternals::EnableSharedFromThis(this, InObject, InObject);
	}

	template<typename OtherType>
	TSharedRef(const TSharedRef<OtherType, Mode>& InSharedRef)
		: Object(InSharedRef.Object), SharedReferenceCount(InSharedRef.SharedReferenceCount)
	{
	}

	template<typename OtherType>
	TSharedRef(const TSharedRef<OtherType, Mode>& OtherSharedRef, ObjectType* InObject)
		: Object(InObject), SharedReferenceCount(OtherSharedRef.SharedReferenceCount)
	{
	}

	TSharedRef(const TSharedRef& InSharedRef) = default;

	TSharedRef(TSharedRef&& InSharedRef)
		: Object(InSharedRef.Object), SharedReferenceCount(InSharedRef.SharedReferenceCount)
	{
	}

	TSharedRef& operator=(const TSharedRef& InSharedRef) = default;

	TSharedRef& operator=(TSharedRef&& InSharedRef)
	{
		std::swap(Object, InSharedRef.Object);
		std::swap(SharedReferenceCount, InSharedRef.SharedReferenceCount);
		return *this;
	}

	template<typename OtherType>
	TSharedRef& operator=(const SharedPointerInternals::FRawPtrProxy<OtherType>& InRawPtrProxy)
	{
		*this = TSharedRef<ObjectType, Mode>(InRawPtrProxy);
		return *this;
	}

public:
	TSharedPtr<ObjectType, Mode> ToSharedPtr() const
	{
		return TSharedPtr<ObjectType, Mode>(*this);
	}

	ObjectType* Get() const
	{
		return Object;
	}

	std::add_lvalue_reference_t<ObjectType> operator*() const
	{
		return *Object;
	}

	ObjectType* operator->() const
	{
		return Object;
	}

	int32 GetSharedReferenceCount() const
	{
		return SharedReferenceCount.GetSharedReferenceCount();
	}

	bool IsUnique() const
	{
		return SharedReferenceCount.IsUnique();
	}

private:
	template<typename OtherType>
	explicit TSharedRef(const TSharedPtr<OtherType, Mode>& InSharedPtr)
		: Object(InSharedPtr.Object), SharedReferenceCount(InSharedPtr.SharedReferenceCount)
	{
	}

	template<typename OtherType>
	explicit TSharedRef(TSharedPtr<OtherType, Mode>&& InSharedPtr)
		: Object(InSharedPtr.Object), SharedReferenceCount(std::move(InSharedPtr.SharedReferenceCount))
	{
		InSharedPtr.Object = nullptr;
	}

	template<typename OtherType, ESPMode OtherMode> friend class TSharedRef;
	template<typename OtherType, ESPMode OtherMode> friend class TSharedPtr;
	template<typename OtherType, ESPMode OtherMode> friend class TWeakPtr;

	ObjectType* Object;
	SharedPointerInternals::FSharedReferencer<Mode> SharedReferenceCount;
};

template<typename ObjectType, ESPMode Mode>
class TSharedPtr
{
public:
	using ElementType = ObjectType;

	TSharedPtr(std::nullptr_t = nullptr)
		: Object(nullptr), SharedReferenceCount()
	{
	}

	template<typename OtherType>
	explicit TSharedPtr(OtherType* InObject)
		: Object(InObject), SharedReferenceCount(SharedPointerInternals::NewDefaultReferenceController(InObject))
	{
		SharedPointerInternals::EnableSharedFromThis(this, InObject, InObject);
	}

	template<typename OtherType, typename DeleterType>
	TSharedPtr(OtherType* InObject, DeleterType&& InDeleter)
		: Object(InObject), SharedReferenceCount(SharedPointerInternals::NewCustomReferenceController(InObject, std::forward<DeleterType>(InDeleter)))
	{
		SharedPointerInternals::EnableSharedFromThis(this, InObject, InObject);
	}

	template<typename OtherType>
	TSharedPtr(const SharedPointerInternals::FRawPtrProxy<OtherType>& InRawPtrProxy)
		: Object(InRawPtrProxy.Object), SharedReferenceCount(InRawPtrProxy.ReferenceController)
	{
		SharedPointerInternals::EnableSharedFromThis(this, InRawPtrProxy.Object, InRawPtrProxy.Object);
	}

	template<typename OtherType>
	TSharedPtr(const TSharedPtr<OtherType, Mode>& InSharedPtr)
		: Object(InSharedPtr.Object), SharedReferenceCount(InSharedPtr.SharedReferenceCount)
	{
	}

	template<typename OtherType>
	TSharedPtr(TSharedPtr<OtherType, Mode>&& InSharedPtr)
		: Object(InSharedPtr.Object), SharedReferenceCount(std::move(InSharedPtr.SharedReferenceCount))
	{
		InSharedPtr.Object = nullptr;
	}

	template<typename OtherType>
	TSharedPtr(const TSharedRef<OtherType, Mode>& InSharedRef)
		: Object(InSharedRef.Object), SharedReferenceCount(InSharedRef.SharedReferenceCount)
	{
	}

	template<typename OtherType>
	TSharedPtr(const TSharedPtr<OtherType, Mode>& OtherSharedPtr, ObjectType* InObject)
		: Object(InObject), SharedReferenceCount(OtherSharedPtr.SharedReferenceCount)
	{
	}

	template<typename OtherType>
	TSharedPtr(const TSharedRef<OtherType, Mode>& OtherSharedRef, ObjectType* InObject)
		: Object(InObject), SharedReferenceCount(OtherSharedRef.SharedReferenceCount)
	{
	}

	TSharedPtr(const TSharedPtr& InSharedPtr) = default;

	TSharedPtr(TSharedPtr&& InSharedPtr)
		: Object(InSharedPtr.Object), SharedReferenceCount(std::move(InSharedPtr.SharedReferenceCount))
	{
		InSharedPtr.Object = nullptr;
	}

	TSharedPtr& operator=(std::nullptr_t)
	{
		Reset();
		return *this;
	}

	TSharedPtr& operator=(const TSharedPtr& InSharedPtr) = default;

	TSharedPtr& operator=(TSharedPtr&& InSharedPtr)
	{
		if (this != &InSharedPtr)
		{
			Object = InSharedPtr.Object;
			InSharedPtr.Object = nullptr;
			SharedReferenceCount = std::move(InSharedPtr.SharedReferenceCount);
		}

		return *this;
	}

	template<typename OtherType>
	TSharedPtr& operator=(const SharedPointerInternals::FRawPtrProxy<OtherType>& InRawPtrProxy)
	{
		*this = TSharedPtr<ObjectType, Mode>(InRawPtrProxy);
		return *this;
	}

public:
	TSharedRef<ObjectType, Mode> ToSharedRef() const
	{
		return TSharedRef<ObjectType, Mode>(*this);
	}

	ObjectType* Get() const
	{
		return Object;
	}

	explicit operator bool() const
	{
		return Object != nullptr;
	}

	bool IsValid() const
	{
		return Object != nullptr;
	}

	std::add_lvalue_reference_t<ObjectType> operator*() const
	{
		return *Object;
	}

	ObjectType* operator->() const
	{
		return Object;
	}

	void Reset()
	{
		*this = TSharedPtr<ObjectType, Mode>();
	}

	int32 GetSharedReferenceCount() const
	{
		return SharedReferenceCount.GetSharedReferenceCount();
	}

	bool IsUnique() const
	{
		return SharedReferenceCount.IsUnique();
	}

private:
	template<typename OtherType>
	explicit TSharedPtr(const TWeakPtr<OtherType, Mode>& InWeakPtr)
		: Object(nullptr), SharedReferenceCount(InWeakPtr.WeakReferenceCount)
	{
		if (SharedReferenceCount.IsValid())
			Object = InWeakPtr.Object;
	}

	template<typename OtherType, ESPMode OtherMode> friend class TSharedPtr;
	template<typename OtherType, ESPMode OtherMode> friend class TSharedRef;
	template<typename OtherType, ESPMode OtherMode> friend class TWeakPtr;

	ObjectType* Object;
	SharedPointerInternals::FSharedReferencer<Mode> SharedReferenceCount;
};

template<typename ObjectType, ESPMode Mode>
class TWeakPtr
{
public:
	using ElementType = ObjectType;

	TWeakPtr(std::nullptr_t = nullptr)
		: Object(nullptr)
	{
	}

	template<typename OtherType>
	TWeakPtr(const TSharedRef<OtherType, Mode>& InSharedRef)
		: Object(InSharedRef.Object), WeakReferenceCount(InSharedRef.SharedReferenceCount)
	{
	}

	template<typename OtherType>
	TWeakPtr(const TSharedPtr<OtherType, Mode>& InSharedPtr)
		: Object(InSharedPtr.Object), WeakReferenceCount(InSharedPtr.SharedReferenceCount)
	{
	}

	template<typename OtherType>
	TWeakPtr(const TWeakPtr<OtherType, Mode>& InWeakPtr)
		: Object(InWeakPtr.Object), WeakReferenceCount(InWeakPtr.WeakReferenceCount)
	{
	}

	template<typename OtherType>
	TWeakPtr(TWeakPtr<OtherType, Mode>&& InWeakPtr)
		: Object(InWeakPtr.Object), WeakReferenceCount(std::move(InWeakPtr.WeakReferenceCount))
	{
		InWeakPtr.Object = nullptr;
	}

	TWeakPtr(const TWeakPtr& InWeakPtr) = default;

	TWeakPtr(TWeakPtr&& InWeakPtr)
		: Object(InWeakPtr.Object), WeakReferenceCount(std::move(InWeakPtr.WeakReferenceCount))
	{
		InWeakPtr.Object = nullptr;
	}

	TWeakPtr& operator=(std::nullptr_t)
	{
		Reset();
		return *this;
	}

	TWeakPtr& operator=(const TWeakPtr& InWeakPtr) = default;

	TWeakPtr& operator=(TWeakPtr&& InWeakPtr)
	{
		if (this != &InWeakPtr)
		{
			Object = InWeakPtr.Object;
			InWeakPtr.Object = nullptr;
			WeakReferenceCount = std::move(InWeakPtr.WeakReferenceCount);
		}

		return *this;
	}

	template<typename OtherType>
	TWeakPtr& operator=(const TWeakPtr<OtherType, Mode>& InWeakPtr)
	{
		Object = InWeakPtr.Pin().Get();
		WeakReferenceCount = InWeakPtr.WeakReferenceCount;
		return *this;
	}

	template<typename OtherType>
	TWeakPtr& operator=(const TSharedRef<OtherType, Mode>& InSharedRef)
	{
		Object = InSharedRef.Object;
		WeakReferenceCount = InSharedRef.SharedReferenceCount;
		return *this;
	}

	template<typename OtherType>
	TWeakPtr& operator=(const TSharedPtr<OtherType, Mode>& InSharedPtr)
	{
		Object = InSharedPtr.Object;
		WeakReferenceCount = InSharedPtr.SharedReferenceCount;
		return *this;
	}

public:
	TSharedPtr<ObjectType, Mode> Pin() const
	{
		return TSharedPtr<ObjectType, Mode>(*this);
	}

	bool IsValid() const
	{
		return Object != nullptr && WeakReferenceCount.IsValid();
	}

	void Reset()
	{
		*this = TWeakPtr<ObjectType, Mode>();
	}

	bool HasSameObject(const void* InOtherPtr) const
	{
		return Pin().Get() == InOtherPtr;
	}

private:
	template<typename OtherType, ESPMode OtherMode> friend class TWeakPtr;
	template<typename OtherType, ESPMode OtherMode> friend class TSharedPtr;

	ObjectType* Object;
	SharedPointerInternals::FWeakReferencer<Mode> WeakReferenceCount;
};

template<typename CastToType, typename CastFromType, ESPMode Mode>
TSharedRef<CastToType, Mode> StaticCastSharedRef(const TSharedRef<CastFromType, Mode>& InSharedRef)
{
	return TSharedRef<CastToType, Mode>(InSharedRef, static_cast<CastToType*>(InSharedRef.Get()));
}

template<typename CastToType, typename CastFromType, ESPMode Mode>
TSharedPtr<CastToType, Mode> StaticCastSharedPtr(const TSharedPtr<CastFromType, Mode>& InSharedPtr)
{
	return TSharedPtr<CastToType, Mode>(InSharedPtr, static_cast<CastToType*>(InSharedPtr.Get()));
}

template<typename CastToType, typename CastFromType, ESPMode Mode>
TSharedRef<CastToType, Mode> ConstCastSharedRef(const TSharedRef<CastFromType, Mode>& InSharedRef)
{
	return TSharedRef<CastToType, Mode>(InSharedRef, const_cast<CastToType*>(InSharedRef.Get()));
}

template<typename CastToType, typename CastFromType, ESPMode Mode>
TSharedPtr<CastToType, Mode> ConstCastSharedPtr(const TSharedPtr<CastFromType, Mode>& InSharedPtr)
{
	return TSharedPtr<CastToType, Mode>(InSharedPtr, const_cast<CastToType*>(InSharedPtr.Get()));
}

template<typename ObjectType, ESPMode Mode>
class TSharedFromThis
{
public:
	TSharedRef<ObjectType, Mode> AsShared()
	{
		return WeakThis.Pin().ToSharedRef();
	}

	TSharedRef<const ObjectType, Mode> AsShared() const
	{
		return TSharedPtr<const ObjectType, Mode>(WeakThis.Pin()).ToSharedRef();
	}

	template<typename SharedPtrType, typename OtherType>
	void UpdateWeakReferenceInternal(const TSharedPtr<SharedPtrType, Mode>* InSharedPtr, OtherType* InObject) const
	{
		if (!WeakThis.IsValid())
			WeakThis = TSharedPtr<ObjectType, Mode>(*InSharedPtr, InObject);
	}

	template<typename SharedRefType, typename OtherType>
	void UpdateWeakReferenceInternal(const TSharedRef<SharedRefType, Mode>* InSharedRef, OtherType* InObject) const
	{
		if (!WeakThis.IsValid())
			WeakThis = TSharedRef<ObjectType, Mode>(*InSharedRef, InObject);
	}

	bool DoesSharedInstanceExist() const
	{
		return WeakThis.IsValid();
	}

protected:
	TSharedFromThis() = default;

	TSharedFromThis(const TSharedFromThis&)
	{
	}

	TSharedFromThis& operator=(const TSharedFromThis&)
	{
		return *this;
	}

	~TSharedFromThis() = default;

	template<typename OtherType>
	static TSharedRef<OtherType, Mode> SharedThis(OtherType* ThisPtr)
	{
		return StaticCastSharedRef<OtherType>(ThisPtr->AsShared());
	}

	template<typename OtherType>
	static TSharedRef<const OtherType, Mode> SharedThis(const OtherType* ThisPtr)
	{
		return StaticCastSharedRef<const OtherType>(ThisPtr->AsShared());
	}

private:
	mutable TWeakPtr<ObjectType, Mode> WeakThis;
};

template<typename ObjectTypeA, typename ObjectTypeB, ESPMode Mode>
bool operator==(const TSharedRef<ObjectTypeA, Mode>& InSharedRefA, const TSharedRef<ObjectTypeB, Mode>& InSharedRefB)
{
	return InSharedRefA.Get() == InSharedRefB.Get();
}

template<typename ObjectTypeA, typename ObjectTypeB, ESPMode Mode>
bool operator==(const TSharedPtr<ObjectTypeA, Mode>& InSharedPtrA, const TSharedPtr<ObjectTypeB, Mode>& InSharedPtrB)
{
	return InSharedPtrA.Get() == InSharedPtrB.Get();
}

template<typename ObjectTypeA, typename ObjectTypeB, ESPMode Mode>
bool operator==(const TSharedPtr<ObjectTypeA, Mode>& InSharedPtrA, const TSharedRef<ObjectTypeB, Mode>& InSharedRefB)
{
	return InSharedPtrA.Get() == InSharedRefB.Get();
}

template<typename ObjectType, ESPMode Mode>
bool operator==(const TSharedPtr<ObjectType, Mode>& InSharedPtr, std::nullptr_t)
{
	return !InSharedPtr.IsValid();
}

template<typename ObjectTypeA, typename ObjectTypeB, ESPMode Mode>
bool operator==(const TWeakPtr<ObjectTypeA, Mode>& InWeakPtrA, const TWeakPtr<ObjectTypeB, Mode>& InWeakPtrB)
{
	return InWeakPtrA.Pin().Get() == InWeakPtrB.Pin().Get();
}

template<typename ObjectTypeA, typename ObjectTypeB, ESPMode Mode>
bool operator==(const TWeakPtr<ObjectTypeA, Mode>& InWeakPtrA, const TSharedPtr<ObjectTypeB, Mode>& InSharedPtrB)
{
	return InWeakPtrA.Pin().Get() == InSharedPtrB.Get();
}

template<typename ObjectTypeA, typename ObjectTypeB, ESPMode Mode>
bool operator==(const TWeakPtr<ObjectTypeA, Mode>& InWeakPtrA, const TSharedRef<ObjectTypeB, Mode>& InSharedRefB)
{
	return InWeakPtrA.Pin().Get() == InSharedRefB.Get();
}

template<typename ObjectType, ESPMode Mode>
bool operator==(const TWeakPtr<ObjectType, Mode>& InWeakPtr, std::nullptr_t)
{
	return !InWeakPtr.IsValid();
}

template<typename ObjectType>
SharedPointerInternals::FRawPtrProxy<ObjectType> MakeShareable(ObjectType* InObject)
{
	return SharedPointerInternals::FRawPtrProxy<ObjectType>(InObject);
}

template<typename ObjectType, typename DeleterType>
SharedPointerInternals::FRawPtrProxy<ObjectType> MakeShareable(ObjectType* InObject, DeleterType&& InDeleter)
{
	return SharedPointerInternals::FRawPtrProxy<ObjectType>(InObject, std::forward<DeleterType>(InDeleter));
}

template<typename ObjectType, ESPMode Mode = ESPMode::Fast, typename... ArgTypes>
TSharedRef<ObjectType, Mode> MakeShared(ArgTypes&&... Args)
{
	SharedPointerInternals::TIntrusiveReferenceController<ObjectType>* Controller = new SharedPointerInternals::TIntrusiveReferenceController<ObjectType>(std::forward<ArgTypes>(Args)...);
	return TSharedRef<ObjectType, Mode>(Controller->GetObjectPtr(), static_cast<SharedPointerInternals::FReferenceControllerBase*>(Controller));
}
