#include "PlatformThread.h"
#include "CString.h"

#if SYS_PLATFORM_WIN

WindowsThread::WindowsThread() 
	:mPriorityLevel(THREAD_PRIORITY_NORMAL)
	,mhThread(0)
	,mbRunning(false)
{

}

WindowsThread::~WindowsThread()
{
	detach();
}

uint32 WindowsThread::GetCurrentThreadId()
{
	return ::GetCurrentThreadId();
}

bool WindowsThread::create( ThreadFunc func , void* ptr , uint32 stackSize )
{
	if ( !mbRunning )
	{
		if ( mhThread )
			CloseHandle(mhThread);

		mhThread = (HANDLE) _beginthreadex(
			NULL, stackSize , func , ptr ,
			0,&mThreadID );
		if ( mhThread == NULL )
			return false;
	}

	mbRunning = true;
	return true;
}

void WindowsThread::detach()
{
	if( mhThread )
	{
		CloseHandle(mhThread);
		mhThread = NULL;
		mbRunning = false;
	}
}

bool WindowsThread::suspend()
{
	DWORD reault = SuspendThread(mhThread);
	if ( reault != -1 ) 
	{
		++mSupendTimes;
		return true;
	}
	return false;
}

bool WindowsThread::resume()
{
	DWORD reault = ResumeThread(mhThread);
	if ( reault != -1 ) 
	{
		--mSupendTimes;
		return true;
	}
	return false;
}

void WindowsThread::join()
{
	::WaitForSingleObject(mhThread, INFINITE);
}

bool WindowsThread::setPriorityLevel( DWORD level )
{
	if ( isRunning() && !SetThreadPriority(mhThread,level) )
		return false;

	mPriorityLevel = level;
	return true;
}

void WindowsThread::setDisplayName(char const* name)
{
	SetThreadName(mThreadID, name);
	SetThreadDescription(mhThread, FCString::CharToWChar(name).c_str());
}

void WindowsThread::SetThreadName(uint32 ThreadID, LPCSTR ThreadName)
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

bool WindowsThread::kill()
{
	if ( mhThread )
	{
		if ( TerminateThread(mhThread,0) )
		{
			CloseHandle(mhThread);
			mhThread = NULL;
			mbRunning = false;
			return true;
		}
	}
	return false;
}

#else

uint32 PosixThread::GetCurrentThreadId()
{
	static_assert(sizeof(uint32) == sizeof(pthread_t), "pthread_t cannot be converted to uint32 one to one");
	return static_cast<uint32>(pthread_self());
}

bool PosixThread::create(ThreadFunc func, void* ptr, uint32 stackSize)
{
	if( pthread_create(&mHandle, NULL, func, ptr) == 0 )
		return true;

	return false;
}

bool PosixThread::kill()
{
	pthread_cancel(mHandle);
}

void PosixThread::join()
{
	pthread_join(mHandle, 0);
}

#endif


