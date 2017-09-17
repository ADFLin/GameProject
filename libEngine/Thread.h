#ifndef Thread_h__
#define Thread_h__

#include "PlatformConfig.h"
#include "CompilerConfig.h"

#include "IntegerType.h"

uint32 const WAIT_TIME_INFINITE = 0xffffffff;

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#include <process.h>

class WinThread
{
public:
	typedef unsigned (_stdcall *ThreadFunc)(void*);

	WinThread()
		:mPriorityLevel( THREAD_PRIORITY_NORMAL )
		,mhThread(0),mbRunning(false){}
	~WinThread(){  detach(); }

	static uint32 GetCurrentThreadId()
	{
		return ::GetCurrentThreadId();
	}

	bool create( ThreadFunc fun , void* ptr , uint32 stackSize = 0 );
	void detach()
	{
		if ( mhThread )
		{
			CloseHandle(mhThread);
			mhThread = NULL;
			mbRunning = false;
		}
	}

	bool     kill();
	bool     suspend();
	bool     resume();
	void     join(){ ::WaitForSingleObject( mhThread , INFINITE ); }

	DWORD    getPriorityLevel() { return mPriorityLevel; }
	bool     setPriorityLevel( DWORD level);
	DWORD    getSuspendTimes(){ return mSupendTimes; }
	unsigned getID(){ return mThreadID; }
	bool     isRunning(){ return mbRunning; }
	void     setDisplayName(char const* name)
	{
		SetThreadName(mThreadID, name);
	}

	static void SetThreadName(uint32 ThreadID, LPCSTR ThreadName)
	{
		/**
		* Code setting the thread name for use in the debugger.
		*
		* http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
		*/
		const uint32 MS_VC_EXCEPTION = 0x406D1388;

		struct THREADNAME_INFO
		{
			uint32 dwType;		// Must be 0x1000.
			LPCSTR szName;		// Pointer to name (in user addr space).
			uint32 dwThreadID;	// Thread ID (-1=caller thread).
			uint32 dwFlags;		// Reserved for future use, must be zero.
		};

		// on the xbox setting thread names messes up the XDK COM API that UnrealConsole uses so check to see if they have been
		// explicitly enabled
		Sleep(10);
		THREADNAME_INFO ThreadNameInfo;
		ThreadNameInfo.dwType = 0x1000;
		ThreadNameInfo.szName = ThreadName;
		ThreadNameInfo.dwThreadID = ThreadID;
		ThreadNameInfo.dwFlags = 0;

		__try
		{
			RaiseException(MS_VC_EXCEPTION, 0, sizeof(ThreadNameInfo) / sizeof(ULONG_PTR), (ULONG_PTR*)&ThreadNameInfo);
		}
		__except( EXCEPTION_EXECUTE_HANDLER )
		{
		}
	}
protected:
	template< class T >
	static unsigned _stdcall RunnableProcess( void* t )
	{
		unsigned result = static_cast< T* >( t )->execRunPrivate();
		static_cast< T* >(t)->mbRunning = false;
		::_endthreadex(result);
		return result;
	}

	DWORD    mSupendTimes;
	DWORD    mPriorityLevel;
	HANDLE   mhThread;
	unsigned mThreadID;
	bool     mbRunning;
};


class WinMutex
{
public:
	WinMutex()   { ::InitializeCriticalSection( &mCS ); }
	~WinMutex()  { ::DeleteCriticalSection( &mCS ); }
	void lock()  { ::EnterCriticalSection( &mCS ); }
	void unlock(){ ::LeaveCriticalSection( &mCS ); }
private: 
	friend class WinCondition;
	CRITICAL_SECTION mCS;
};


class WinCondition
{
public:
	WinCondition()
	{
		InitializeConditionVariable( &mCV );
	}

	~WinCondition()
	{


	}
	void notify()
	{
		WakeConditionVariable( &mCV );
	}

	void notifyAll()
	{
		WakeAllConditionVariable( &mCV );
	}
protected:

	template< class Fun >
	bool doWait( WinMutex& mutex , Fun fun )
	{
		bool result = true;
		while( !fun() )
		{
			result = SleepConditionVariableCS( &mCV , &mutex.mCS , INFINITE ) !=0;
		}
		return result;
	}
	bool doWaitTime( WinMutex& mutex , uint32 time = INFINITE )
	{ 
		return SleepConditionVariableCS( &mCV , &mutex.mCS , time ) !=0;
	}
private:
	CONDITION_VARIABLE mCV;
};


typedef WinThread PlatformThread;
typedef WinMutex PlatformMutex;
typedef WinCondition PlatformCondition;


#elif SYS_PLATFORM_LINUX 

class PosixThread
{
public:
	typedef void* (*ThreadFunc)(void*);

	static uint32 GetCurrentThreadId()
	{
		static_assert(sizeof(uint32) == sizeof(pthread_t), "pthread_t cannot be converted to uint32 one to one");
		return static_cast<uint32>(pthread_self());
	}

	bool create( ThreadFunc fun , void* ptr , uint32 stackSize)
	{
		if ( pthread_create(&mHandle,NULL,fun,ptr) == 0 )
			return true;

		return false;
	}
	void detach()
	{
	}

	bool     kill()
	{
		pthread_cancel(mHandle);
	}
	bool     suspend();
	bool     resume();
	void     join(){ pthread_join(mHandle,0); }


	template< class T >
	static void* RunnableProcess( void* t )
	{
		return static_cast< T* >( t )->execRun();
	}

	pthread_t mHandle;
};

class PosixMutex
{
public:
	PostfixMutex()  { ::pthread_mutex_init(&mMutex,NULL); }
	~PostfixMutex() { :: pthread_mutex_destroy(&mMutex); }
	void lock()  { ::pthread_mutex_lock(&mMutex); }
	void unlock(){ ::pthread_mutex_unlock(&mMutex); }
private: 
	friend class Condition;
	pthread_mutex_t mMutex;
};

class PosixCondition
{


};

typedef PosixThread PlatformThread;
typedef PosixMutex PlatformMutex;
typedef PosixCondition PlatformCondition;

#else
#error "Thread Not Support!"
#endif


template< class T >
class RunnableThreadT : public PlatformThread
{
	T* _this(){ return static_cast< T* >( this ); }
	typedef RunnableThreadT< T > RannableThread;
public:

	//~
	unsigned run() { return 0; }
	bool init() { return true; }
	void exit() {}
	//

	bool start(uint32 stackSize = 0)
	{ 
		if ( !_this()->init() )
			return false;
		return create( PlatformThread::RunnableProcess< T > , this , stackSize );
	}
	void stop()
	{
		PlatformThread::kill();
		_this()->exit();
	}

	unsigned execRunPrivate()
	{
		T* impl = _this();
		unsigned reault = impl->run();
		impl->exit();
		
		return reault;
	}
};


template< class T >
class TFunctionThread : public RunnableThreadT< TFunctionThread<T> >
{
public:
	TFunctionThread( T fun ):mFunction( fun ){}
	unsigned run(){  (mFunction)(); return 0; }
private:
	T mFunction;
};

class Mutex : public PlatformMutex
{
public:
	class Locker
	{
	public:
		Locker( Mutex& mutex ):mMutex( mutex ){ mMutex.lock(); }
		~Locker(){ mMutex.unlock(); }
	private:
		friend class Condition;
		Mutex& mMutex;
	};
private: 
	friend class Condition;
};

class Condition : public PlatformCondition
{
public:
	Condition(){}
	~Condition(){}

	template< class Fun >
	bool wait( Mutex::Locker& locker , Fun fun )
	{
		return PlatformCondition::doWait( locker.mMutex , fun );
	}
	bool waitTime( Mutex::Locker& locker , uint32 time = WAIT_TIME_INFINITE )
	{ 
		return PlatformCondition::doWaitTime( locker.mMutex , time );
	}
};

template< class T >
struct TLockedObjectHandle
{
public:
	TLockedObjectHandle(T& obj, Mutex* m) :object(&obj), mutex(m) {}
private:
	Mutex* mutex;
	T*     object;
	template< class T >
	friend class TLockedObject;
};

template < class T >
auto MakeLockedObjectHandle(T& obj, Mutex* mutex)
{
	return TLockedObjectHandle< T >(obj, mutex);
}

template< class T >
class TLockedObject
{
public:
	TLockedObject( TLockedObjectHandle<T> const& handle )
		:mHandle(handle){ if ( mHandle.mutex ) mHandle.mutex->lock(); }
	~TLockedObject(){ if ( mHandle.mutex ) mHandle.mutex->unlock(); }
	T* operator->(){ return mHandle.object; }
	operator T* () { return mHandle.object; }
	T& operator *(){ return *mHandle.object; }

private:
	TLockedObject(TLockedObject const& other) = delete;
	TLockedObject& operator = (TLockedObject const& other) = delete;
	TLockedObjectHandle<T>   mHandle;
};


#endif // Thread_h__
