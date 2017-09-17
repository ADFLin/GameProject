#include "Thread.h"

#if SYS_PLATFORM_WIN

bool WinThread::create( ThreadFunc fun , void* ptr , uint32 stackSize )
{
	if ( !mbRunning )
	{
		if ( mhThread )
			CloseHandle(mhThread);

		mhThread = (HANDLE) _beginthreadex(
			NULL, stackSize , fun , ptr ,
			0,&mThreadID );
		if ( mhThread == NULL )
			return false;
	}

	mbRunning = true;
	return true;
}

bool WinThread::suspend()
{
	DWORD reault = SuspendThread(mhThread);
	if ( reault != -1 ) 
	{
		++mSupendTimes;
		return true;
	}
	return false;
}

bool WinThread::resume()
{
	DWORD reault = ResumeThread(mhThread);
	if ( reault != -1 ) 
	{
		--mSupendTimes;
		return true;
	}
	return false;
}

bool WinThread::setPriorityLevel( DWORD level )
{
	if ( isRunning() && !SetThreadPriority(mhThread,level) )
		return false;

	mPriorityLevel = level;
	return true;
}

bool WinThread::kill()
{
	if ( isRunning() )
	{
		if ( TerminateThread(mhThread,0) )
		{
			mbRunning = false;
			return true;
		}
	}
	return false;
}

#else


#endif