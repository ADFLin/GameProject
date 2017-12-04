#include "TestStageHeader.h"

#include "FileSystem.h"

#include "WindowsHeader.h"
#include  <TlHelp32.h>

#include "Thread.h"
#include "StringParse.h"
#include "RandomUtility.h"
#include "WidgetUtility.h"

#include "Go/GoCore.h"
#include "Go/GoRenderer.h"

#include "GLGraphics2D.h"
#include "RenderGL/GLCommon.h"
#include "RenderGL/GLDrawUtility.h"
#include "RenderGL/GpuProfiler.h"

#include <winternl.h>


#define DETECT_LEELA_PROCESS 1

using namespace RenderGL;
using namespace Go;



BOOL SuspendProcess(HANDLE ProcessHandle)
{
	typedef NTSTATUS (NTAPI *FnNtSuspendProcess)(HANDLE ProcessHandle);
	// we need to get the address of the two ntapi functions
	FARPROC fpNtSuspendProcess = GetProcAddress(GetModuleHandle("ntdll"), "NtSuspendProcess");
	FnNtSuspendProcess NtSuspendProcess = (FnNtSuspendProcess)fpNtSuspendProcess; // add a check if the address is 0 or not before doing this..
	if( !NT_SUCCESS(NtSuspendProcess(ProcessHandle)) ) // call ntsuspendprocess
	{
		return FALSE; // failed
	}
	return TRUE; // success
}

BOOL ResumeProcess(HANDLE ProcessHandle)
{
	typedef NTSTATUS (NTAPI *FnNtResumeProcess)(HANDLE ProcessHandle);
	FARPROC fpNtResumeProcess = GetProcAddress(GetModuleHandle("ntdll"), "NtResumeProcess");
	FnNtResumeProcess NtResumeProcess = (FnNtResumeProcess)fpNtResumeProcess;
	if( !NT_SUCCESS(NtResumeProcess(ProcessHandle)) )
	{
		return FALSE;
	}
	return TRUE;
}



DWORD FindPIDByName(TCHAR const* name)
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

DWORD FindPIDByParentPID(DWORD parentID )
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

DWORD FindPIDByNameAndParentPID(TCHAR const* name , DWORD parentID )
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

bool IsProcessRunning(DWORD pid)
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

class MyGame : public Go::Game
{
public:
	struct Com
	{
		int pos[2];
	};
	void addCom( Com const& com )
	{
		Mutex::Locker lock(mComMutex);
		mComs.push_back(com);
	}
	std::vector< Com > mComs;
	Mutex mComMutex;
};

class IGameProcessThread : public RunnableThreadT< IGameProcessThread >
{
public:
	virtual ~IGameProcessThread(){}
	virtual unsigned run() = 0;
};

class GameLearningThread : public IGameProcessThread 
{
public:

	unsigned run()
	{
		for( ;;)
		{
			DWORD numRead;
			CHAR buffer[1024 + 1];
			BOOL bSuccess = ::ReadFile(mExecuteOutRead, buffer, ARRAY_SIZE(buffer) - 1, &numRead, NULL);
			if( !bSuccess || numRead == 0 )
				break;
			buffer[numRead] = 0;
			parseOutput(buffer, numRead);
		}
		return 0;
	}

	void parseOutput(char* buffer, int num)
	{
		char const* cur = buffer;
		while( *cur != 0  )
		{
			int step;
			char coord[32];
			int numRead;
			cur = ParseUtility::SkipSpace(cur);
			if( sscanf(cur, "%d%s%n", &step, coord, &numRead) == 2 && coord[0] == '(')
			{
				if( step == 1 )
				{
					MyGame::Com com;
					com.pos[0] = -2;
					com.pos[1] = -1;
					mGame->addCom(com);
					curStep = 1;
				}

				if( curStep != step )
				{
					::Msg("Warning:Error Step");
				}
				if( strcmp("(pass)", coord) == 0 )
				{
					MyGame::Com com;
					com.pos[0] = -1;
					com.pos[1] = -1;
					mGame->addCom(com);

				}
				else
				{
					int pos[2];
					Go::ReadCoord(coord + 1, pos);
					MyGame::Com com;
					com.pos[0] = pos[0];
					com.pos[1] = pos[1];
					mGame->addCom(com);

				}
				++curStep;
				cur += numRead;
			}
			else
			{
				char const* next = ParseUtility::SkipToNextLine(cur);
				int num = next - cur;
				if( num )
				{
					FixString< 512 > str{ cur , num };
					::Msg("%s", str.c_str());
					cur = next;
				}
			}
		}
	}

	int curStep = 0;

	HANDLE mExecuteOutRead;
	MyGame* mGame;

};


class GamePlayingThread : public IGameProcessThread
{
public:

	unsigned run()
	{
		for( ;;)
		{
			DWORD numRead;
			CHAR buffer[2048 + 1];
			BOOL bSuccess = ::ReadFile(mExecuteOutRead, buffer, ARRAY_SIZE(buffer) - 1, &numRead, NULL);
			if( !bSuccess || numRead == 0 )
				break;
			buffer[numRead] = 0;
			parseOutput(buffer, numRead);
		}
		return 0;
	}

	void parseOutput(char* buffer, int num)
	{
		char const* cur = buffer;
		while( *cur != 0 )
		{
			int step;
			char coord[32];
			int numRead;
			cur = ParseUtility::SkipSpace(cur);
			//if( sscanf(cur, "%d%s%n", &step, coord, &numRead) == 2 && coord[0] == '(' )
			//{
			//	if( step == 1 )
			//	{
			//		MyGame::Com com;
			//		com.pos[0] = -2;
			//		com.pos[1] = -1;
			//		mGame->addCom(com);
			//		curStep = 1;
			//	}

			//	if( curStep != step )
			//	{
			//		::Msg("Warning:Error Step");
			//	}
			//	if( strcmp("(pass)", coord) == 0 )
			//	{
			//		MyGame::Com com;
			//		com.pos[0] = -1;
			//		com.pos[1] = -1;
			//		mGame->addCom(com);

			//	}
			//	else
			//	{
			//		int pos[2];
			//		Go::ReadCoord(coord + 1, pos);
			//		MyGame::Com com;
			//		com.pos[0] = pos[0];
			//		com.pos[1] = pos[1];
			//		mGame->addCom(com);

			//	}
			//	++curStep;
			//	cur += numRead;
			//}
			//else
			{
				char const* next = ParseUtility::SkipToNextLine(cur);
				int num = next - cur;
				if( num )
				{
					FixString< 512 > str{ cur , num };
					::Msg("%s", str.c_str());
					cur = next;
				}
			}
		}
	}

	int curStep = 0;

	HANDLE  mExecuteOutRead;
	MyGame* mGame;

};



class LeelaZeroLearningStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	LeelaZeroLearningStage() {}

	enum
	{
		UI_TOGGLE_PAUSE_GAME = BaseClass::NEXT_UI_ID,
		NEXT_UI_ID ,
	};
	HANDLE mProcess = NULL;
	
	HANDLE mExecuteOutRead = NULL;
	HANDLE mExecuteOutWrite = NULL;
	IGameProcessThread* processThread = nullptr;

	bool bPauseProcess = false;

	MyGame mGame;

#if DETECT_LEELA_PROCESS
	DWORD  mPIDLeela = -1;
	
	static long const RestartTime = 20000;
	long   mRestartTimer = RestartTime;
#endif

	GameRenderer mGameRenderer;


	virtual bool onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		if( !::Global::getDrawEngine()->startOpenGL( true ) )
			return false;

		if( !mGameRenderer.initializeRHI() )
			return false;

		::Global::GUI().cleanupWidget();

		mGame.setup(19);
		mGameRenderer.generateNoiseOffset( mGame.getBoard().getSize() );

#if 1
		if( !createLearningProcess() )
			return false;
#else
		if( !createPlayProcess() )
			return false;
#endif

		auto devFrame = WidgetUtility::CreateDevFrame();
		devFrame->addButton(UI_TOGGLE_PAUSE_GAME, "Pause Process");
		return true;
	}

	char const* mLeelaZeroDir = "E:/Desktop/LeelaZero";

	bool createLearningProcess()
	{
		FixString<256> path;
		path.format("%s/%s", mLeelaZeroDir, "/autogtp.exe");

		if( !createChildPorcessWithIO(path) )
			return false;

		auto myThread = new GameLearningThread;
		myThread->mExecuteOutRead = mExecuteOutRead;
		myThread->mGame = &mGame;
		myThread->start();
		myThread->setDisplayName("Output Thread");
		processThread = myThread;

#if DETECT_LEELA_PROCESS
		mPIDLeela = -1;
		mRestartTimer = RestartTime;
#endif
		return true;
	}

	bool createPlayProcess()
	{
		FixString<256> path;
		path.format("%s/%s", mLeelaZeroDir, "/leelaz.exe");
		FixString<512> command;
		char const* weightName = "223737476718d58a4a5b0f317a1eeeb4b38f0c06af5ab65cb9d76d68d9abadb6";
		command.format(" --w %s", weightName );
		if( !createChildPorcessWithIO(path , command) )
			return false;

		auto myThread = new GamePlayingThread;
		myThread->mExecuteOutRead = mExecuteOutRead;
		myThread->mGame = &mGame;
		myThread->start();
		myThread->setDisplayName("Output Thread");
		processThread = myThread;
		return true;
	}

	bool createChildPorcessWithIO( char const* path , char const* command = nullptr)
	{
		SECURITY_ATTRIBUTES saAttr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;
		if( !::CreatePipe(&mExecuteOutRead, &mExecuteOutWrite, &saAttr, 0) )
			return false;

		if( !SetHandleInformation(mExecuteOutRead, HANDLE_FLAG_INHERIT, 0) )
			return false;


		mProcess = CreateChildProcess(path, command);
		if( mProcess == NULL )
			return false;

		return true;
	}

	virtual void onEnd()
	{
		::Global::getDrawEngine()->stopOpenGL(true);
		if( processThread )
		{
			processThread->kill();
		}
		cleanupProcessData();
		BaseClass::onEnd();
	}

	void restart() {}

	
	void tick() 
	{
		

	}
	void updateFrame(int frame) {}

	virtual void onUpdate(long time)
	{
		BaseClass::onUpdate(time);


		{
			Mutex::Locker locker(mGame.mComMutex);
			for( MyGame::Com com : mGame.mComs )
			{
				if( com.pos[0] == -2 )
				{
					mGame.restart();
					mGameRenderer.generateNoiseOffset(mGame.getBoard().getSize());
#if DETECT_LEELA_PROCESS
					mPIDLeela = -1;
					mRestartTimer = RestartTime;
#endif
				}
				else if( com.pos[0] == -1 )
				{
					mGame.pass();
					::Msg("Pass");
				}
				else
				{
					if( !mGame.play(com.pos[0], com.pos[1]) )
					{
						::Msg("Warning:Can't Play step : [%d,%d]", com.pos[0], com.pos[1]);
					}
				}
			}
			mGame.mComs.clear();
		}
#if DETECT_LEELA_PROCESS
		if( mPIDLeela == -1 )
		{
			mPIDLeela = FindPIDByNameAndParentPID(TEXT("leelaz.exe"), GetProcessId(mProcess));
			if( mPIDLeela == -1 )
			{
				mRestartTimer -= time;
			}
		}
		else
		{

			if( !IsProcessRunning(mPIDLeela) )
			{
				mRestartTimer -= time;
			}
			else
			{
				mRestartTimer = RestartTime;
			}
		}

		if( mRestartTimer < 0 )
		{
			static int tickCount = 0;
			++tickCount;
			processThread->kill();
			cleanupProcessData();
			createLearningProcess();
		}
#endif

		int frame = time / gDefaultTickTime;
		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}



	void onRender(float dFrame)
	{
		using namespace RenderGL;

		Vec2i const BoardPos = Vec2i(50, 50);

		using namespace Go;

		GLGraphics2D& g = ::Global::getGLGraphics2D();
		g.beginRender();

		GpuProfiler::getInstance().beginFrame();

		glClear(GL_COLOR_BUFFER_BIT);

		mGameRenderer.draw( BoardPos , mGame);

		GpuProfiler::getInstance().endFrame();

		g.endRender();

		g.beginRender();

		g.setTextColor(255, 0, 0);
		RenderUtility::SetFont(g, FONT_S10);
		int const offset = 15;
		int textX = 300;
		int y = 10;
		FixString< 512 > str;
		str.format("bUseBatchedRender = %s" , mGameRenderer.bUseBatchedRender ? "true" : "false" );
		g.drawText(textX , y += offset, str);
		for( int i = 0; i < GpuProfiler::getInstance().getSampleNum(); ++i )
		{
			GpuProfileSample* sample = GpuProfiler::getInstance().getSample(i);
			str.format("%.8lf => %s", sample->time, sample->name.c_str());
			g.drawText(textX + 10 * sample->level, y += offset, str);

		}
		g.endRender();
	}


	bool onMouse(MouseMsg const& msg)
	{
		if( !BaseClass::onMouse(msg) )
			return false;
		return true;
	}

	bool onKey(unsigned key, bool isDown)
	{
		if( !isDown )
			return false;
		switch( key )
		{
		case Keyboard::eR: restart(); break;
		case Keyboard::eZ: mGameRenderer.bUseBatchedRender = !mGameRenderer.bUseBatchedRender; break;
		}
		return false;
	}

	virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
	{
		switch( id )
		{
		case UI_TOGGLE_PAUSE_GAME:
			if( bPauseProcess )
			{
				if( ResumeProcess(mProcess) )
				{
					GUI::CastFast<GButton>(ui)->setTitle("Pause Process");
					bPauseProcess = false;
				}
			}
			else
			{
				if( SuspendProcess(mProcess) )
				{
					GUI::CastFast<GButton>(ui)->setTitle("Resume Process");
					bPauseProcess = true;
				}
			}
			break;
		default:
			break;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}


	void cleanupProcessData()
	{
		bPauseProcess = false;

		if( mExecuteOutRead )
		{
			CloseHandle(mExecuteOutRead);
			mExecuteOutRead = NULL;
		}
		if( mExecuteOutWrite )
		{
			CloseHandle(mExecuteOutWrite);
			mExecuteOutWrite = NULL;
		}
		if( processThread )
		{
			delete processThread;
			processThread = nullptr;
		}
		if( mProcess )
		{
			TerminateProcess(mProcess, -1);
			mProcess = NULL;
		}
	}

	HANDLE CreateChildProcess(TCHAR const* path, TCHAR const* commandLine)
	{
		PROCESS_INFORMATION piProcInfo;
		STARTUPINFO siStartInfo;
		BOOL bSuccess = FALSE;


		TCHAR workDir[MAX_PATH];

		TCString<TCHAR>::CopyN(workDir, path, FileUtility::GetDirPathPos(path) - path);
		// Set up members of the PROCESS_INFORMATION structure. 

		TCHAR command[MAX_PATH];
		if( commandLine )
		{
			TCString<TCHAR>::Copy(command, commandLine);
		}
		else
		{
			command[0] = 0;
		}


		ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

		// Set up members of the STARTUPINFO structure. 
		// This structure specifies the STDIN and STDOUT handles for redirection.

		ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
		siStartInfo.cb = sizeof(STARTUPINFO);
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

		siStartInfo.hStdOutput = mExecuteOutWrite;
		siStartInfo.hStdError = mExecuteOutWrite;
		siStartInfo.hStdInput = NULL;
		siStartInfo.wShowWindow = SW_HIDE;

		// Create the child process. 

		bSuccess = CreateProcess(
			path,
			command,  // command line 
			NULL,          // process security attributes 
			NULL,          // primary thread security attributes 
			TRUE,          // handles are inherited 
			0,             // creation flags 
			NULL,          // use parent's environment 
			workDir,
			&siStartInfo,  // STARTUPINFO pointer 
			&piProcInfo);  // receives PROCESS_INFORMATION 

						   // If an error occurs, exit the application. 
		if( !bSuccess )
			return NULL;

		CloseHandle(piProcInfo.hThread);
		return piProcInfo.hProcess;
	}
};

REGISTER_STAGE("LeelaZero Learning", LeelaZeroLearningStage, EStageGroup::Test);