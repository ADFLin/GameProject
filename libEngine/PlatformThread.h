#ifndef Thread_h__
#define Thread_h__

#include "PlatformConfig.h"
#include "CompilerConfig.h"

#include "Core/IntegerType.h"

uint32 const WAIT_TIME_INFINITE = 0xffffffff;

#if SYS_PLATFORM_LINUX
#define SYS_SUPPORT_POSIX_THREAD 1
#endif

#ifndef SYS_SUPPORT_POSIX_THREAD
#define SYS_SUPPORT_POSIX_THREAD 0
#endif

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#include <process.h>

#define ERROR_THREAD_ID (-1)
class WindowsThread
{
public:
	typedef unsigned (_stdcall *ThreadFunc)(void*);

	WindowsThread();
	~WindowsThread();

	static uint32 GetCurrentThreadId();

	bool create( ThreadFunc func , void* ptr , uint32 stackSize = 0 );
	void detach();

	bool     kill();
	bool     suspend();
	bool     resume();
	void     join();

	DWORD    getPriorityLevel() { return mPriorityLevel; }
	bool     setPriorityLevel( DWORD level);
	DWORD    getSuspendTimes(){ return mSupendTimes; }
	unsigned getID(){ return mThreadID; }
	bool     isRunning(){ return mbRunning; }
	void     setDisplayName(char const* name)
	{
		SetThreadName(mThreadID, name);
	}

	static void SetThreadName(uint32 ThreadID, LPCSTR ThreadName);
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


class WindowsMutex
{
public:
	WindowsMutex()     { ::InitializeCriticalSection( &mCS ); }
	~WindowsMutex()    { ::DeleteCriticalSection( &mCS ); }
	void lock()    { ::EnterCriticalSection( &mCS ); }
	void unlock()  { ::LeaveCriticalSection( &mCS ); }
	bool tryLock() { return !!::TryEnterCriticalSection(&mCS); }
private: 
	friend class WindowsConditionVariable;
	CRITICAL_SECTION mCS;
};


class WindowsConditionVariable
{
public:
	WindowsConditionVariable()
	{
		InitializeConditionVariable( &mCV );
	}

	~WindowsConditionVariable()
	{


	}
	void notifyOne()
	{
		WakeConditionVariable( &mCV );
	}

	void notifyAll()
	{
		WakeAllConditionVariable( &mCV );
	}
protected:

	bool doWait(WindowsMutex& mutex)
	{
		bool result = SleepConditionVariableCS(&mCV, &mutex.mCS, INFINITE) != 0;
		return result;
	}

	template< class TFunc >
	bool doWait( WindowsMutex& mutex , TFunc func )
	{
		bool result = true;
		while( !func() )
		{
			result = SleepConditionVariableCS( &mCV , &mutex.mCS , INFINITE ) !=0;
		}
		return result;
	}
	bool doWaitTime( WindowsMutex& mutex , uint32 time = INFINITE )
	{ 
		return SleepConditionVariableCS( &mCV , &mutex.mCS , time ) !=0;
	}
private:
	CONDITION_VARIABLE mCV;
};


typedef WindowsThread PlatformThread;
typedef WindowsMutex  PlatformMutex;
typedef WindowsConditionVariable PlatformConditionVariable;

#elif SYS_SUPPORT_POSIX_THREAD

class PosixThread
{
public:
	typedef void* (*ThreadFunc)(void*);

	static uint32 GetCurrentThreadId();

	bool create( ThreadFunc func , void* ptr , uint32 stackSize);
	void detach()
	{
		
	}

	bool     kill();
	bool     suspend()
	{

	}
	bool     resume()
	{

	}
	void     join();


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
	friend class ConditionVariable;
	pthread_mutex_t mMutex;
};

class PosixConditionVariable
{


};

typedef PosixThread PlatformThread;
typedef PosixMutex PlatformMutex;
typedef PosixConditionVariable PlatformConditionVariable;

#else
#error "Thread Not Support!"
#endif

class Thread : public PlatformThread
{

};

template< class T >
class RunnableThreadT : public Thread
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
		return create( Thread::RunnableProcess< T > , _this() , stackSize );
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


template< class TFunc >
class TFunctionThread : public RunnableThreadT< TFunctionThread<TFunc> >
{
public:
	TFunctionThread( TFunc&& func ):mFunction( std::forward<TFunc>(func) ){}
	unsigned run(){  (mFunction)(); return 0; }
private:
	TFunc mFunction;
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
		friend class ConditionVariable;
		Mutex& mMutex;
	};
private: 
	friend class ConditionVariable;
};

class ConditionVariable : public PlatformConditionVariable
{
public:
	ConditionVariable(){}
	~ConditionVariable(){}

	bool wait(Mutex::Locker& locker)
	{
		return PlatformConditionVariable::doWait(locker.mMutex);
	}

	template< class TFunc >
	bool wait(Mutex::Locker& locker, TFunc func )
	{
		return PlatformConditionVariable::doWait(locker.mMutex, func );
	}
	bool waitTime(Mutex::Locker& locker, uint32 time = WAIT_TIME_INFINITE )
	{ 
		return PlatformConditionVariable::doWaitTime(locker.mMutex, time );
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
