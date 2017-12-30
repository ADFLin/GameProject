#pragma once
#ifndef WindowsProcess_H_3E773242_ECA6_4021_A191_E5E7F911B4CA
#define WindowsProcess_H_3E773242_ECA6_4021_A191_E5E7F911B4CA

#include "COre/IntegerType.h"
#include "WindowsHeader.h"

class FPlatformProcess
{
public:
	static BOOL SuspendProcess(HANDLE ProcessHandle);
	static BOOL ResumeProcess(HANDLE ProcessHandle);

	static DWORD FindPIDByName(TCHAR const* name);

	static DWORD FindPIDByParentPID(DWORD parentID);

	static DWORD FindPIDByNameAndParentPID(TCHAR const* name, DWORD parentID);

	static bool IsProcessRunning(DWORD pid)
	{
		bool result = true;
		HANDLE handle = ::OpenProcess(SYNCHRONIZE, false, pid);
		if( handle == NULL )
		{
			result = false;
		}
		else
		{
			uint32 WaitResult = WaitForSingleObject(handle, 0);
			if( WaitResult != WAIT_TIMEOUT )
			{
				result = false;
			}
			::CloseHandle(handle);
		}
		return result;
	}
};

class ChildProcess
{
public:
	~ChildProcess()
	{
		cleanup();
	}

	HANDLE getHandle() { return mProcess; }

	HANDLE mProcess = NULL;

	HANDLE mExecuteOutRead = NULL;
	HANDLE mExecuteOutWrite = NULL;
	HANDLE mExecuteInRead = NULL;
	HANDLE mExecuteInWrite = NULL;

	void cleanup();

	void terminate()
	{
		if( mProcess )
		{
			TerminateProcess(mProcess, -1);
		}
		cleanup();
	}
	bool resume()
	{
		return !!FPlatformProcess::ResumeProcess(mProcess);
	}

	bool suspend()
	{
		return !!FPlatformProcess::SuspendProcess(mProcess);
	}

	bool create(char const* path, char const* command = nullptr);

	bool writeInputStream(void const* buffer, int maxSize, int& outWriteSize)
	{
		DWORD numWrite = 0;
		if( ::WriteFile(mExecuteInWrite, buffer, maxSize, &numWrite, nullptr) )
		{
			outWriteSize = numWrite;
			return true;
		}
		return false;
	}

	bool readOutputStream(void* buffer, int maxSize, int& outReadSize)
	{
		DWORD numRead = 0;
		if( ::ReadFile(mExecuteOutRead, buffer, maxSize, &numRead, nullptr) )
		{
			outReadSize = numRead;
			return true;
		}
		return false;
	}

};

#endif // WindowsProcess_H_3E773242_ECA6_4021_A191_E5E7F911B4CA
