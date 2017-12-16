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

bool ChildProcess::createWithIO(char const* path, char const* command /*= nullptr*/)
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
	workDir.assign(path, FileUtility::GetDirPathPos(path) - path);

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

DWORD FPlatformProcess::FindPIDByName(TCHAR const* name)
{
	PROCESSENTRY32 entry;

	entry.dwFlags = sizeof(PROCESSENTRY32);

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	DWORD result = -1;
	if( Process32First(hSnapshot, &entry) == TRUE )
	{
		while( Process32Next(hSnapshot, &entry) == TRUE )
		{
			if( TCString<TCHAR>::Compare(entry.szExeFile, name) == 0 )
			{
				result = entry.th32ProcessID;
				break;
			}
		}
	}
	CloseHandle(hSnapshot);
	return result;
}

DWORD FPlatformProcess::FindPIDByParentPID(DWORD parentID)
{
	PROCESSENTRY32 entry;

	entry.dwFlags = sizeof(PROCESSENTRY32);

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	DWORD result = -1;
	if( Process32First(hSnapshot, &entry) == TRUE )
	{
		while( Process32Next(hSnapshot, &entry) == TRUE )
		{
			if( entry.th32ParentProcessID == parentID )
			{
				result = entry.th32ProcessID;
				break;
			}
		}
	}
	CloseHandle(hSnapshot);
	return result;
}

DWORD FPlatformProcess::FindPIDByNameAndParentPID(TCHAR const* name, DWORD parentID)
{
	PROCESSENTRY32 entry;

	entry.dwFlags = sizeof(PROCESSENTRY32);

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	DWORD result = -1;
	if( Process32First(hSnapshot, &entry) == TRUE )
	{
		while( Process32Next(hSnapshot, &entry) == TRUE )
		{
			if( TCString<TCHAR>::Compare(entry.szExeFile, name) == 0 &&
			   entry.th32ParentProcessID == parentID )
			{
				result = entry.th32ProcessID;
				break;
			}
		}
	}
	CloseHandle(hSnapshot);
	return result;
}
