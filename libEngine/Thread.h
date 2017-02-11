#ifndef Thread_h__
#define Thread_h__

#include "PlatformConfig.h"
#include "CompilerConfig.h"

#include "IntegerType.h"

uint32 const WAIT_TIME_INFINITE = 0xffffffff;

#if SYS_PLATFORM_WIN
#include "Win32Header.h"
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

	bool create( ThreadFunc fun , void* ptr );
	void detach()
	{
		if ( mhThread )
		{
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

	void     finish( unsigned retValue )
	{ 
		mbRunning = false;
		::_endthreadex( retValue );  
	}
protected:
	template< class T >
	static unsigned _stdcall RunnableProcess( void* t )
	{
		return static_cast< T* >( t )->execRunPrivate();
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
	WinMutex()      { ::InitializeCriticalSection( &mCS ); }
	~WinMutex()     { ::DeleteCriticalSection( &mCS ); }
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
	bool doWaitUntil( WinMutex& mutex , Fun fun )
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

	bool create( ThreadFunc fun , void* ptr )
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
	bool start()
	{ 
		if ( !_this()->init() )
			return false;
		return create( PlatformThread::RunnableProcess< RannableThread > , this );
	}
	void stop()
	{
		PlatformThread::kill();
		_this()->exit();
	}
	unsigned run(){ return 0; }
	bool init(){ return true; }
	void exit(){}

	unsigned execRunPrivate()
	{
		T* impl = _this();
		unsigned reault = impl->run();
		impl->finish( reault );
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
	bool waitUntil( Mutex::Locker& locker , Fun fun )
	{
		return PlatformCondition::doWaitUntil( locker.mMutex , fun );
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
