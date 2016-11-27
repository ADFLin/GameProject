#include "Thread.h"

#if SYS_PLATFORM_WIN

bool WinThread::create( ThreadFunc fun , void* ptr )
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

bool WinThread::suspend()
{
	DWORD reault = SuspendThread(m_hThread);
	if ( reault != -1 ) 
	{
		++m_SupendTimes;
		return true;
	}
	return false;
}

bool WinThread::resume()
{
	DWORD reault = ResumeThread(m_hThread);
	if ( reault != -1 ) 
	{
		--m_SupendTimes;
		return true;
	}
	return false;
}

bool WinThread::setPriorityLevel( DWORD level )
{
	if ( isRunning() && !SetThreadPriority(m_hThread,level) )
		return false;

	m_PriorityLevel = level;
	return true;
}

bool WinThread::kill()
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

#else


#endif