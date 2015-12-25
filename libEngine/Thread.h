#ifndef Thread_h__
#define Thread_h__

#include "Win32Header.h"
#include <process.h>

class WinThread
{
public:
	typedef unsigned (_stdcall *ThreadFunc)(void*);

	WinThread()
		:m_PriorityLevel( THREAD_PRIORITY_NORMAL )
		,m_hThread(0),m_beRunning(false){}
	~WinThread(){  detach(); }


	bool create( ThreadFunc fun , void* ptr );
	void detach()
	{
		if ( m_hThread )
		{
			m_hThread = NULL;
			m_beRunning = false;
		}
	}

	bool     kill();
	bool     suspend();
	bool     resume();
	void     join(){ ::WaitForSingleObject( m_hThread , INFINITE ); }

	DWORD    getPriorityLevel() { return m_PriorityLevel; }
	bool     setPriorityLevel( DWORD level);
	DWORD    getSuspendTimes(){ return m_SupendTimes; }
	unsigned getID(){ m_uThreadID; }
	bool     isRunning(){ return m_beRunning; }

	void      endRunning( unsigned retValue )
	{ 
		m_beRunning = false;
		::_endthreadex( retValue );  
	}
protected:

	DWORD    m_SupendTimes;
	DWORD    m_PriorityLevel;

	HANDLE   m_hThread;
	unsigned m_uThreadID;
	bool     m_beRunning;
};

class PlatformThread
{
public:
	class ThreadFunc;
	bool  create( ThreadFunc fun , void* ptr );
	void  endRunning( unsigned retValue );

};

class WinThread;

template< class ThreadImpl , class PlatformThread = WinThread >
class ThreadT : public PlatformThread
{
public:
	typedef typename PlatformThread::ThreadFunc ThreadFunc;

	bool     start()
	{ 
		return create( processThread , this );
	}
	unsigned run()
	{
		assert(0);
	}

private:
	ThreadImpl* _this(){ return static_cast< ThreadImpl* >( this ); }

	static unsigned _stdcall processThread( void* t )
	{
		ThreadT* ptrThread = static_cast< ThreadT* >( t );
		unsigned reault = ptrThread->_this()->run();
		ptrThread->endRunning( reault );
		return reault;
	}
};


class SimpleThread : public ThreadT< SimpleThread >
{
public:
	typedef ThreadT< SimpleThread >::ThreadFunc ThreadFunc;

	unsigned run()
	{
		return (*m_ThreadInfo.Func)( m_ThreadInfo.Data );
	}

private:
	struct ThreadInfo
	{
		ThreadInfo():Func(0),Data(0){}
		ThreadInfo(ThreadFunc fun,void* data)
			:Func(fun),Data(data){}
		ThreadFunc Func;
		void* Data;
	} m_ThreadInfo;
};

template< class T >
class MemberFunThread : public ThreadT< MemberFunThread<T> >
{
public:
	typedef unsigned (T::*MemFun)();
	MemberFunThread( T* ptr , MemFun fun ){ init( ptr , fun ); }
	MemberFunThread(){ mPtr = 0 ; mFun = 0; }
	void init( T* ptr , MemFun fun )
	{
		mPtr = ptr; mFun = fun;
	}
	unsigned run(){  return (mPtr->*mFun)();  }

private:
	T*     mPtr;
	MemFun mFun;
};


inline bool WinThread::create( ThreadFunc fun , void* ptr )
{
	if ( !m_beRunning )
	{
		if ( m_hThread )
			CloseHandle(m_hThread);

		m_hThread = (HANDLE) _beginthreadex(
			NULL,0 , fun , ptr ,
			0,&m_uThreadID );
		if ( m_hThread == NULL )
			return false;
	}

	m_beRunning = true;
	return true;
}


inline bool WinThread::suspend()
{
	DWORD reault = SuspendThread(m_hThread);
	if ( reault != -1 ) 
	{
		++m_SupendTimes;
		return true;
	}
	return false;
}

inline bool WinThread::resume()
{
	DWORD reault = ResumeThread(m_hThread);
	if ( reault != -1 ) 
	{
		--m_SupendTimes;
		return true;
	}
	return false;
}

inline bool WinThread::setPriorityLevel( DWORD level )
{
	if ( isRunning() && !SetThreadPriority(m_hThread,level) )
		return false;

	m_PriorityLevel = level;
	return true;
}

inline bool WinThread::kill()
{
	if ( isRunning() )
	{
		if ( TerminateThread(m_hThread,0) )
		{
			m_beRunning = false;
			return true;
		}
	}
	return false;
}



class Mutex
{
public:
	Mutex()      { ::InitializeCriticalSection( &mCS ); }
	~Mutex()     { ::DeleteCriticalSection( &mCS ); }
	void lock()  { ::EnterCriticalSection( &mCS ); }
	void unlock(){ ::LeaveCriticalSection( &mCS ); }
	class Locker
	{
	public:
		Locker( Mutex& mutex ):mMutex( mutex ){ mMutex.lock(); }
		~Locker(){ mMutex.unlock(); }
	private:
		Mutex& mMutex;
	};
private: 
	friend class Condition;
	CRITICAL_SECTION mCS;
};

class Condition
{
public:
	Condition()
	{
		InitializeConditionVariable( &m_cd );
	}

	~Condition()
	{

		
	}
	template< class Fun >
	bool waitUntil( Mutex& matex , Fun fun )
	{
		Mutex::Locker locker(mutex);
		bool result;
		while( fun() )
		{
			result = SleepConditionVariableCS( &m_cd , &mutex.mCS , INFINITE ) !=0;
		}
		return result;
	}
	bool waitTime( Mutex& mutex , DWORD time = INFINITE )
	{ 
		return SleepConditionVariableCS( &m_cd , &mutex.mCS , time ) !=0;
	}
	void notify()
	{
		WakeConditionVariable( &m_cd );
	}

	void notifyAll()
	{
		WakeAllConditionVariable( &m_cd );
	}

private:
	CONDITION_VARIABLE m_cd;
};

#ifndef SINGLE_THREAD
	#define DEFINE_MUTEX( MUTEX ) Mutex MUTEX;
	#define MUTEX_LOCK( MUTEX ) Mutex::Locker locker_##__LINE__( MUTEX );
#else
	#define DEFINE_MUTEX( MUTEX )
	#define MUTEX_LOCK( MUTEX ) 
#endif

template< class T >
struct LockObject
{
	struct Param
	{
		Param( T& obj , Mutex* m ):object(&obj),mutex(m){}
	private:
		Mutex* mutex;
		T*     object;
		friend struct LockObject;
	};
	LockObject( Param const& info )
		:mInfo(info){ if ( mInfo.mutex ) mInfo.mutex->lock(); }
	~LockObject(){ if ( mInfo.mutex ) mInfo.mutex->unlock(); }
	T* operator->(){ return mInfo.object; }
	operator T* () { return mInfo.object; }
	T& operator *(){ return *mInfo.object; }
private:
	LockObject( LockObject& other ){}
	Param   mInfo;
};

#endif // Thread_h__
