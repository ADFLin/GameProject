//#include "MiscTestPCH.h"
#include "Clock.h"
#include "FastDelegate/FastDelegate.h"


uint64 GInt = 0;
class Foo
{
public:
	void foo(int n) 
	{
		//LogMsg("Foo!");
		GInt += n;
	}

	void foo2(int n)
	{
		//LogMsg("Foo2!");
		GInt += n;
	}
	
	static void sfoo(int n)
	{
		//LogMsg("SFoo");
		GInt += n;
	}
};


template< class T >
class TDelegate {};

template< uint32 StorageSize >
class SingleInlineAllocator
{
public:
	void* allocate(uint32 allocSize)
	{
		if( allocSize > StorageSize )
		{
			void** pPtr = *reinterpret_cast<void**>(mData);
			*pPtr = ::malloc(allocSize);
			return *pPtr;
		}
		return mData;
	}
	void  deallocate(uint32 allocSize)
	{
		if( allocSize > StorageSize )
		{
			void** pPtr = *reinterpret_cast<void**>(mData);
			::free(*ptr);
		}
	}

	void* getData()
	{
		return mData;
	}

	void* getAllocatedPtr(uint32 allocSize)
	{
		if( allocSize > StorageSize )
		{
			return *reinterpret_cast<void**>(mData);
		}
		return mData;
	}

	uint8 mData[StorageSize];
};

template< class RetType , class ...Args >
class TDelegate< RetType (Args...) >
{
public:
	TDelegate()
	{
		::memset( this , 0 , sizeof(*this));
	}

	~TDelegate()
	{
		cleanup();
	}

	void cleanup()
	{
		if( mTraits && mTraits->DestroyFunc )
		{
			mTraits->DestroyFunc(mStorage.getData());
		}
			
	}
	struct FuncTraits
	{
		uint32 allocSize;
		RetType(*InvokeFunc)(void*, Args...);
		RetType(*DestroyFunc)(void*);
		RetType(*CopyFunc)(void*, void*);
		RetType(*EqualFunc)(void*, void*);
	};
	FuncTraits* mTraits;
	SingleInlineAllocator<16> mStorage;

	template< class T >
	struct  MemberFuncData
	{
		T* ptr;
		typedef RetType(T::*MemberFuncType)(Args...);
		MemberFuncType memberFunc;
	};


	template< class T, RetType (Foo::*Func)(Args...) >
	void bind(T* ptr)
	{
		cleanup();
		mTraits = &GetFixedMemberFuncTraits<T, Func>();
		reinterpret_cast<T*>( mStorage.getData()) = ptr;
	}

	template< RetType(*Func)(Args...) >
	void bind()
	{
		cleanup();
		mTraits = &GetFixedFuncTraits<Func>();
		reinterpret_cast<void*>(mStorage.getData()) = Func;
	}

	void bind(RetType(*Func)(Args...))
	{
		cleanup();
		mTraits = &GetNormalFuncTraits<T>();
		reinterpret_cast<void*>( mStorage.getData() ) = Func;
	}

	template< class Q , class T >
	void bind(Q* ptr, RetType(T::*MemberFunc)(Args...))
	{
		cleanup();
		mTraits = &GetMemberFuncTraits<T>();
		MemberFuncData<T>* data = reinterpret_cast<MemberFuncData<T>*>(mStorage.getData());
		data->ptr = static_cast<T*>(ptr);
		data->memberFunc = MemberFunc;
	}

	template< class LambdaObject >
	void bind(LambdaObject&& obj)
	{
		cleanup();
		mTraits = &GetLamdbaTraits<LambdaObject>();
		static_assert(sizeof(mStorage) >= sizeof(LambdaObject) , "Delegate' storage is not enough!!");
		new (mStorage.getData()) LambdaObject(std::forward<LambdaObject>(obj));
	}

	template< class LambdaObject >
	TDelegate& operator = (LambdaObject obj) { bind(std::forward<LambdaObject>(obj)); return *this; }

	TDelegate& operator = (TDelegate const& other)
	{
		cleanup();
		mTraits = other.mTraits;
		if( mTraits && mTraits->CopyFunc )
		{
			mTraits->CopyFunc(mStorage.getData(), other.mStorage.getData());
		}
		return *this;
	}

 	FORCEINLINE RetType operator()( Args... args ) { return mTraits->InvokeFunc(mStorage.getData(), std::forward<Args>(args)...); }


	template< class LambdaObject >
	static RetType InvokeLamdba(void* handle , Args... args)
	{
		return (*reinterpret_cast<LambdaObject*>(handle))(std::forward<Args>(args)...);
	}

	template< class LambdaObject >
	static RetType DestoryLamdba(void* handle)
	{
		(reinterpret_cast<LambdaObject*>(handle))->~LambdaObject();
	}
	template< class LambdaObject >
	static RetType CopyLamdba(void* handle , void* otherHandle)
	{
		new (handle) LambdaObject(*reinterpret_cast<LambdaObject*>(otherHandle));
	}
	template< class LambdaObject >
	FuncTraits& GetLamdbaTraits()
	{
		static FuncTraits result{ sizeof(LambdaObject) , &InvokeLamdba<LambdaObject > , &DestoryLamdba<LambdaObject> , &CopyLamdba<LambdaObject> };
		return result;
	}

	template< class T >
	static RetType InvokeMemberFunc(void* handle, Args... args)
	{
		MemberFuncData<T>* data = reinterpret_cast<MemberFuncData<T>*>(handle);
		return ((data->ptr)->*data->memberFunc)(std::forward<Args>(args)...);
	}

	template< class T >
	FuncTraits& GetMemberFuncTraits()
	{
		static FuncTraits result{ 0 , &InvokeMemberFunc<T> , nullptr };
		return result;
	}

	template< class T, RetType (T::*Func)(Args...) >
	static RetType InvokeFixedMemberFunc(void* handle, Args... args)
	{
		return (reinterpret_cast<T*>(handle)->*Func)(std::forward<Args>(args)...);
	}

	template< class T, RetType(T::*Func)(Args...) >
	FuncTraits& GetFixedMemberFuncTraits()
	{
		static FuncTraits result{ 0 , &InvokeFixedMemberFunc<T,Func> , nullptr };
		return result;
	}

	template< RetType (*Func)(Args...) >
	static RetType InvokeFixedFunc(void* handle, Args... args)
	{
		return (*Func)(std::forward<Args>(args)...);
	}

	template< RetType(*Func)(Args...) >
	FuncTraits& GetFixedFuncTraits()
	{
		static FuncTraits result{ 0 , &InvokeFixedFunc<Func> , nullptr };
		return result;
	}

	static RetType InvokeNormalFunc(void* handle, Args... args)
	{
		return (*reinterpret_cast<RetType (*)(Args...)>(handle) )(std::forward<Args>(args)...);
	}

	FuncTraits& GetNormalFuncTraits()
	{
		static FuncTraits result{ 0 , &InvokeNormalFunc , nullptr };
		return result;
	}
};




thread_local TDelegate< void(int) > delegate;
thread_local std::function< void(int) > delegate2;
thread_local fastdelegate::FastDelegate< void(int) > delegate3;

template< class Fun , class ...Args >
void RunTest(char const* name , HighResClock& clock , Fun& fun , Args... args )
{
	GInt = 0;
	for( int i = 0; i < 10000000; ++i )
	{
		fun(i, std::forward<Args>(args)...);
	}

	GInt = 0;
	clock.reset();
	for( int i = 0; i < 50000000; ++i )
	{
		fun( i , std::forward<Args>(args)...);
	}
	LogMsg("%s = %.3f  %ld", name , clock.getTimeMicroseconds() / 1000.0f , GInt );
}

void FooBind(int a, int b)
{
	GInt += a;
	GInt -= b;
	LogMsg("%ld", GInt);
}

static void DelegateTest()
{
	int j = 100;

	GInt = 0;
	delegate.bind([=](int i) { GInt += i; GInt += j; LogMsg("%d", GInt); });
	delegate(10);
	Foo f;
	GInt = 0;
	delegate = std::bind(&FooBind, 10 , std::placeholders::_1);
	delegate(10);


	HighResClock clock;

	if( rand() % 2 )
		delegate3.bind(&f, &Foo::foo);
	else
		delegate3.bind(&f, &Foo::foo2);

	RunTest("FastDelegate", clock, delegate3);
#if 0

	if ( rand() % 2 )
		delegate.bind<Foo,&Foo::foo>(&f);
	else
		delegate.bind<Foo,&Foo::foo2>(&f);
#else
	if( rand() % 2 )
		delegate.bind(&f, &Foo::foo);
	else
		delegate.bind(&f, &Foo::foo2);
#endif

	RunTest("TDeletge" , clock, delegate);

	if( rand() % 2 )
		delegate2 = std::bind(&Foo::foo, &f, std::placeholders::_1);
	else
		delegate2 = std::bind(&Foo::foo2, &f, std::placeholders::_1);

	RunTest("std::function" , clock, delegate2);

}

REGISTER_MISC_TEST("Delegate Test", DelegateTest);