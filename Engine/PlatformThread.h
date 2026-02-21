#ifndef Thread_h__
#define Thread_h__

#include "PlatformConfig.h"
#include "CompilerConfig.h"
#include "SystemPlatform.h"

#include "Core/IntegerType.h"
#include "TemplateMisc.h"
#include "Meta/Concept.h"
#include "Meta/FunctionCall.h"

#include <utility>

uint32 const WAIT_TIME_INFINITE = 0xffffffff;

#if SYS_PLATFORM_LINUX || SYS_PLATFORM_HTML5
#define SYS_SUPPORT_POSIX_THREAD 1
#endif

#ifndef SYS_SUPPORT_POSIX_THREAD
#define SYS_SUPPORT_POSIX_THREAD 0
#endif

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#include <process.h>
#include <string>




#define ERROR_THREAD_ID (-1)
class WindowsThread : public Noncopyable
{
public:
	typedef unsigned (_stdcall *ThreadFunc)(void*);

	WindowsThread();
	~WindowsThread();

	static uint32 GetCurrentThreadId();

	bool create( ThreadFunc func , void* ptr , uint32 stackSize = 0 );
	void detach();

	bool     kill();
	bool     hadSuspended() const;
	bool     suspend();
	bool     resume();
	void     join();

	DWORD    getPriorityLevel() { return mPriorityLevel; }
	bool     setPriorityLevel( DWORD level);
	DWORD    getSuspendTimes(){ return mSupendTimes; }
	unsigned getID(){ return mThreadID; }
	bool     isRunning(){ return mbRunning; }
	void     setDisplayName(char const* name);

	static void SetThreadName(uint32 threadID, char const* threadName);
	static std::string GetThreadName(uint32 threadId);

protected:
	template< class T >
	static unsigned _stdcall RunnableProcess( void* t )
	{
		unsigned result = static_cast< T* >( t )->execRunPrivate();
		static_cast< T* >(t)->mbRunning = false;
		::_endthreadex(result);
		return result;
	}

	volatile int32 mSupendTimes = 0;
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
	mutable CRITICAL_SECTION mCS;
};


class WindowsConditionVariable : public Noncopyable
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

class WindowsRWLock : public Noncopyable
{
public:
	WindowsRWLock()
	{
		InitializeSRWLock(&mImpl);
	}

	void readLock()
	{
		AcquireSRWLockShared(&mImpl);
	}
	void writeLock()
	{
		AcquireSRWLockExclusive(&mImpl);
	}

	bool tryReadLock()
	{
		return TryAcquireSRWLockShared(&mImpl);
	}
	bool tryWirteLock()
	{
		return TryAcquireSRWLockExclusive(&mImpl);
	}

	void readUnlock()
	{
		ReleaseSRWLockShared(&mImpl);
	}
	void writeUnlock()
	{
		ReleaseSRWLockExclusive(&mImpl);
	}

	SRWLOCK mImpl;
};

class WindowsSpinLock : public Noncopyable
{
public:
	WindowsSpinLock() { InitializeSRWLock(&mImpl); }
	void lock() { AcquireSRWLockExclusive(&mImpl); }
	void unlock() { ReleaseSRWLockExclusive(&mImpl); }
	bool tryLock() { return !!TryAcquireSRWLockExclusive(&mImpl); }
private:
	SRWLOCK mImpl;
};

class WindowsThreadEvent : public Noncopyable
{

public:
	WindowsThreadEvent()
	{
		mHandle = ::CreateEventA(NULL, TRUE, FALSE, "Event");
	}

	~WindowsThreadEvent()
	{
		if (mHandle)
		{
			::CloseHandle(mHandle);
		}
	}

	void reset()
	{
		::ResetEvent(mHandle);
	}


	void fire()
	{
		::SetEvent(mHandle);
	}

	void wait(double time)
	{
		long millisecond = int(time * 1000);
		DWORD result = ::WaitForSingleObject(mHandle, millisecond);
	}
	void wait()
	{
		DWORD result = ::WaitForSingleObject(mHandle, INFINITE);
	}
	HANDLE mHandle;
};


using PlatformThread = WindowsThread;
using PlatformMutex = WindowsMutex;
using PlatformConditionVariable = WindowsConditionVariable;
using PlatformRWLock = WindowsRWLock;
using PlatformSpinLock = WindowsSpinLock;
using PlatformThreadEvent = WindowsThreadEvent;

#elif SYS_SUPPORT_POSIX_THREAD

#include <pthread.h>

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
		return false;
	}
	bool     resume()
	{
		return false;
	}
	void     join();


	template< class T >
	static void* RunnableProcess( void* t )
	{
		return nullptr;
	}

	pthread_t mHandle;
};

class PosixMutex
{
public:
	PosixMutex()  { ::pthread_mutex_init(&mMutex,NULL); }
	~PosixMutex() { :: pthread_mutex_destroy(&mMutex); }
	void lock()  { ::pthread_mutex_lock(&mMutex); }
	void unlock(){ ::pthread_mutex_unlock(&mMutex); }
private: 
	friend class ConditionVariable;
	pthread_mutex_t mMutex;
};

class PosixConditionVariable
{
public:
	void notifyOne()
	{

	}

	void notifyAll()
	{

	}
protected:
	bool doWait(PosixMutex& mutex)
	{
		return false;
	}

	template< class TFunc >
	bool doWait(PosixMutex& mutex, TFunc func)
	{
		return false;
	}
	bool doWaitTime(PosixMutex& mutex, uint32 time = -1)
	{
		return false;
	}
};

#include <atomic>
class PosixSpinLock : public Noncopyable
{
public:
	PosixSpinLock() { mLocked.clear(); }
	void lock()
	{
		while (mLocked.test_and_set(std::memory_order_acquire))
		{
#if defined(__i386__) || defined(__x86_64__)
			__builtin_ia32_pause();
#elif defined(__arm__) || defined(__aarch64__)
			asm volatile("yield" ::: "memory");
#endif
		}
	}
	void unlock() { mLocked.clear(std::memory_order_release); }
	bool tryLock() { return !mLocked.test_and_set(std::memory_order_acquire); }
private:
	std::atomic_flag mLocked = ATOMIC_FLAG_INIT;
};

using PlatformThread = PosixThread;
using PlatformMutex = PosixMutex;
using PlatformConditionVariable = PosixConditionVariable;
using PlatformSpinLock = PosixSpinLock;
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

template< typename TFunc >
class TAsyncFuncThread : public RunnableThreadT<TAsyncFuncThread<TFunc>>
{
public:
	TAsyncFuncThread(TFunc&& func)
		:mFunc(std::forward<TFunc>(func))
	{
	}

	unsigned run()
	{
		if constexpr (TCheckConcept< CFunctionCallable, TFunc, Thread* >::Value)
		{
			mFunc(this);
		}
		else
		{
			mFunc();
		}
		return 0;
	}
	void exit()
	{
		delete this;
	}
	TFunc mFunc;
};

template< typename TFunc >
void Async(TFunc&& func)
{
	TAsyncFuncThread< TFunc >* thread = new TAsyncFuncThread< TFunc >(std::forward<TFunc>(func));
	thread->start();
}

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

class SpinLock : public PlatformSpinLock
{
public:
#if _DEBUG
	SpinLock() :mOwnerThreadId(0) {}
	void lock()
	{
		uint32 threadId = PlatformThread::GetCurrentThreadId();
		if (mOwnerThreadId == threadId)
		{
			// Deadlock detected: Reentrant lock attempt on non-reentrant SpinLock!
			SystemPlatform::DebugBreak();
		}
		PlatformSpinLock::lock();
		mOwnerThreadId = threadId;
	}

	void unlock()
	{
		mOwnerThreadId = 0;
		PlatformSpinLock::unlock();
	}

	bool tryLock()
	{
		if (PlatformSpinLock::tryLock())
		{
			mOwnerThreadId = PlatformThread::GetCurrentThreadId();
			return true;
		}
		return false;
	}
private:
	uint32 mOwnerThreadId;
#endif

public:
	class Locker
	{
	public:
		Locker(SpinLock& lock) :mLock(lock) { mLock.lock(); }
		~Locker() { mLock.unlock(); }
	private:
		SpinLock& mLock;
	};
};

class RWLock : public PlatformRWLock
{
public:
	class ReadLocker
	{
	public:
		ReadLocker(RWLock& lock) :mLock(lock) { mLock.readLock(); }
		~ReadLocker() { mLock.readUnlock(); }
	private:
		friend class ConditionVariable;
		RWLock& mLock;
	};

	class WriteLocker
	{
	public:
		WriteLocker(RWLock& lock) :mLock(lock) { mLock.writeLock(); }
		~WriteLocker() { mLock.writeUnlock(); }
	private:
		friend class ConditionVariable;
		RWLock& mLock;
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

class ThreadEvent : public PlatformThreadEvent
{

};

template< class T >
struct TLockedObjectHandle
{
public:
	TLockedObjectHandle(T& obj, Mutex* m) :object(&obj), mutex(m) {}
private:
	Mutex* mutex;
	T*     object;
	template< class Q >
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
