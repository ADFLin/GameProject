#include "WindowsProcess.h"

#include "CString.h"
#include "FixString.h"
#include "FileSystem.h"

#include  <TlHelp32.h>
#include <winternl.h>

void ChildProcess::cleanup()
{
#define SAFE_RELEASE_HANDLE( HandleName )\
	if( HandleName )\
	{\
		CloseHandle(HandleName);\
		HandleName = NULL;\
	}

	SAFE_RELEASE_HANDLE(mExecuteOutRead);
	SAFE_RELEASE_HANDLE(mExecuteOutWrite);
	SAFE_RELEASE_HANDLE(mExecuteInRead);
	SAFE_RELEASE_HANDLE(mExecuteInWrite);
	SAFE_RELEASE_HANDLE(mProcess);

#undef SAFE_RELEASE_HANDLE
}

void ChildProcess::terminate()
{
	if( mProcess )
	{
		TerminateProcess(mProcess, -1);
	}
	cleanup();
}

bool ChildProcess::resume()
{
	return !!FPlatformProcess::ResumeProcess(mProcess);
}

bool ChildProcess::suspend()
{
	return !!FPlatformProcess::SuspendProcess(mProcess);
}

bool ChildProcess::create(char const* path, char const* command /*= nullptr*/)
{
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;
	if( !::CreatePipe(&mExecuteOutRead, &mExecuteOutWrite, &saAttr, 0) )
		return false;
	if( !::SetHandleInformation(mExecuteOutRead, HANDLE_FLAG_INHERIT, 0) )
		return false;

	if( !::CreatePipe(&mExecuteInRead, &mExecuteInWrite, &saAttr, 0) )
		return false;
	if( !::SetHandleInformation(mExecuteInWrite, HANDLE_FLAG_INHERIT, 0) )
		return false;


	PROCESS_INFORMATION procInfo;
	STARTUPINFO startInfo;
	BOOL bSuccess = FALSE;
	FixString<  MAX_PATH, TCHAR > workDir;
	workDir.assign(path, FileUtility::GetFileName(path) - path);

	FixString< MAX_PATH, TCHAR > commandLine;
	if( command )
	{
		commandLine = command;
	}

	// Set up members of the PROCESS_INFORMATION structure. 
	ZeroMemory(&procInfo, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.

	ZeroMemory(&startInfo, sizeof(STARTUPINFO));
	startInfo.cb = sizeof(STARTUPINFO);
	startInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

	startInfo.hStdOutput = mExecuteOutWrite;
	startInfo.hStdError = mExecuteOutWrite;
	startInfo.hStdInput = mExecuteInRead;
	startInfo.wShowWindow = SW_HIDE;

	// Create the child process. 

	bSuccess = ::CreateProcess(
		path,
		commandLine,  // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		workDir,
		&startInfo,  // STARTUPINFO pointer 
		&procInfo);  // receives PROCESS_INFORMATION 

					 // If an error occurs, exit the application. 
	if( !bSuccess )
		return false;

	CloseHandle(procInfo.hThread);
	mProcess = procInfo.hProcess;

	return true;
}

bool ChildProcess::writeInputStream(void const* buffer, int maxSize, int& outWriteSize)
{
	DWORD numWrite = 0;
	if( ::WriteFile(mExecuteInWrite, buffer, maxSize, &numWrite, nullptr) )
	{
		outWriteSize = numWrite;
		return true;
	}
	return false;
}

bool ChildProcess::readOutputStream(void* buffer, int maxSize, int& outReadSize)
{
	DWORD numRead = 0;
	if( ::ReadFile(mExecuteOutRead, buffer, maxSize, &numRead, nullptr) )
	{
		outReadSize = numRead;
		return true;
	}
	return false;
}

void ChildProcess::waitCompletion()
{
	::WaitForSingleObject(mProcess, INFINITE);
}

bool ChildProcess::getExitCode(int32& outCode)
{
	DWORD exitCode = 0;
	if( !GetExitCodeProcess(mProcess, &exitCode) )
		return false;

	outCode = exitCode;
	return true;
}

BOOL FPlatformProcess::SuspendProcess(HANDLE ProcessHandle)
{
	typedef NTSTATUS(NTAPI *FnNtSuspendProcess)(HANDLE ProcessHandle);
	// we need to get the address of the two ntapi functions
	FARPROC fpNtSuspendProcess = GetProcAddress(GetModuleHandle("ntdll"), "NtSuspendProcess");
	FnNtSuspendProcess NtSuspendProcess = (FnNtSuspendProcess)fpNtSuspendProcess; // add a check if the address is 0 or not before doing this..
	if( !NT_SUCCESS(NtSuspendProcess(ProcessHandle)) ) // call ntsuspendprocess
	{
		return FALSE; // failed
	}
	return TRUE; // success
}

BOOL FPlatformProcess::ResumeProcess(HANDLE ProcessHandle)
{
	typedef NTSTATUS(NTAPI *FnNtResumeProcess)(HANDLE ProcessHandle);
	FARPROC fpNtResumeProcess = GetProcAddress(GetModuleHandle("ntdll"), "NtResumeProcess");
	FnNtResumeProcess NtResumeProcess = (FnNtResumeProcess)fpNtResumeProcess;
	if( !NT_SUCCESS(NtResumeProcess(ProcessHandle)) )
	{
		return FALSE;
	}
	return TRUE;
}

template< class TFunc >
void VisitProcess( TFunc&& func )
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if( hSnapshot != INVALID_HANDLE_VALUE )
	{
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);
		if( Process32First(hSnapshot, &entry) == TRUE )
		{
			do
			{
				if ( !func( entry ) )
					break;
			}
			while( Process32Next(hSnapshot, &entry) == TRUE );
		}
		CloseHandle(hSnapshot);
	}
}

DWORD FPlatformProcess::FindPIDByName(TCHAR const* name)
{
	DWORD result = -1;
	VisitProcess([&result , name](PROCESSENTRY32 const& entry)
	{
		if( FCString::CompareIgnoreCase(entry.szExeFile, name) == 0 )
		{
			result = entry.th32ProcessID;
			return false;
		}
		return true;
	});
	return result;
}

DWORD FPlatformProcess::FindPIDByParentPID(DWORD parentID)
{
	DWORD result = -1;
	VisitProcess([&result, parentID](PROCESSENTRY32 const& entry)
	{
		if( entry.th32ParentProcessID == parentID )
		{
			result = entry.th32ProcessID;
			return false;
		}
		return true;
	});
	return result;
}

DWORD FPlatformProcess::FindPIDByNameAndParentPID(TCHAR const* name, DWORD parentID)
{
	DWORD result = -1;
	VisitProcess([&result, name, parentID](PROCESSENTRY32 const& entry)
	{
		if( FCString::Compare(entry.szExeFile, name) == 0 &&
		   entry.th32ParentProcessID == parentID )
		{
			result = entry.th32ProcessID;
			return false;
		}
		return true;
	});
	return result;
}
