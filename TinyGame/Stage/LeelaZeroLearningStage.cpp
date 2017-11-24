#include "TestStageHeader.h"

#include "FileSystem.h"

#include "WindowsHeader.h"
#include  <TlHelp32.h>

#include "Thread.h"
#include "StringParse.h"

#include "Go/GoCore.h"

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


class GameListenThread : public RunnableThreadT< GameListenThread >
{
public:

	unsigned run()
	{
		mGame->setup(19);
		for( ;;)
		{
			DWORD numRead;
			CHAR buffer[1024];
			BOOL bSuccess = ::ReadFile(mExecuteOutRead, buffer, ARRAY_SIZE(buffer), &numRead, NULL);
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
				FixString< 512 > str{ cur , next - cur };
				::Msg("%s", str.c_str() );
				cur = next;
			}
		}
	}

	int curStep = 0;

	HANDLE mExecuteOutRead;
	MyGame* mGame;

};

#define DETECT_LEELA_PROCESS 0

class LeelaZeroLearningStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	LeelaZeroLearningStage() {}

	HANDLE mGTPProcess = NULL;
	
	HANDLE mExecuteOutRead = NULL;
	HANDLE mExecuteOutWrite = NULL;
	GameListenThread* outputThread = nullptr;


	MyGame mGame;

#if DETECT_LEELA_PROCESS
	DWORD  mPIDLeela = -1;
	
	static long const RestartTime = 20000;
	long   mRestartTimer = RestartTime;
#endif

	void cleanupProcessData()
	{
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
		if( outputThread )
		{
			delete outputThread;
			outputThread = nullptr;
		}
		if( mGTPProcess )
		{
			TerminateProcess(mGTPProcess, -1);
			mGTPProcess = NULL;
		}
	}

	HANDLE CreateChildProcess(TCHAR const* path , TCHAR* commandLine )
	{
		PROCESS_INFORMATION piProcInfo;
		STARTUPINFO siStartInfo;
		BOOL bSuccess = FALSE;

		
		TCHAR workDir[MAX_PATH];

		TCString<TCHAR>::CopyN(workDir, path, FileUtility::GetDirPathPos(path) - path);
		// Set up members of the PROCESS_INFORMATION structure. 

		ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

		// Set up members of the STARTUPINFO structure. 
		// This structure specifies the STDIN and STDOUT handles for redirection.

		ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
		siStartInfo.cb = sizeof(STARTUPINFO);
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		
		siStartInfo.hStdOutput = mExecuteOutWrite;
		siStartInfo.hStdError  = mExecuteOutWrite;
		siStartInfo.hStdInput = NULL;
		siStartInfo.wShowWindow = SW_HIDE;

		// Create the child process. 

		bSuccess = CreateProcess(
			path,
			commandLine ,  // command line 
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

	virtual bool onInit()
	{
		if( !BaseClass::onInit() )
			return false;
		::Global::GUI().cleanupWidget();

		if( !createProcess() )
			return false;

		return true;
	}

	bool createProcess()
	{
		SECURITY_ATTRIBUTES saAttr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;
		if( !::CreatePipe(&mExecuteOutRead, &mExecuteOutWrite, &saAttr, 0) )
			return false;

		if( !SetHandleInformation(mExecuteOutRead, HANDLE_FLAG_INHERIT, 0) )
			return false;

		TCHAR const* path = "E:/Desktop/LeelaZero/autogtp.exe";
		mGTPProcess = CreateChildProcess(path , nullptr);
		if( mGTPProcess == NULL )
			return false;

		outputThread = new GameListenThread;
		outputThread->mExecuteOutRead = mExecuteOutRead;
		outputThread->mGame = &mGame;
		outputThread->start();
		outputThread->setDisplayName("Output Thread");
#if DETECT_LEELA_PROCESS
		mPIDLeela = -1;
		mRestartTimer = RestartTime;
#endif
		return true;
	}

	virtual void onEnd()
	{
		cleanupProcessData();
		BaseClass::onEnd();
	}

	void restart() {}

	
	void tick() 
	{
		
		{
			Mutex::Locker locker(mGame.mComMutex);
			for( MyGame::Com com : mGame.mComs )
			{
				if( com.pos[0] == -2 )
				{
					mGame.restart();
					mGame.bNextGame = true;
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
			mPIDLeela = FindPIDByNameAndParentPID(TEXT("leelaz.exe") , GetProcessId(mGTPProcess) );
			if( mPIDLeela == -1 )
			{
				mRestartTimer -= gDefaultTickTime;
			}
		}
		else
		{

			if(  !IsProcessRunning(mPIDLeela) )
			{
				
				if( mGame.bNextGame == true )
				{
					mGame.bNextGame = false;
					mPIDLeela = -1;
					mRestartTimer = 20000;
				}
				else
				{
					mRestartTimer -= gDefaultTickTime;
				}
			}
			else
			{
				mRestartTimer = 20000;
			}
		}

		if( mRestartTimer < 0 )
		{
			static int tickCount = 0;
			++tickCount;
			outputThread->kill();
			cleanupProcessData();
			createProcess();
		}
#endif
	}
	void updateFrame(int frame) {}

	virtual void onUpdate(long time)
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}

	int const CellSize = 28;
	int const StoneSize = (CellSize / 2) * 9 / 10;

	void onRender(float dFrame)
	{

		Vec2i const BoardPos = Vec2i(50, 50);
		char* CoordStr = "ABCDEFGHJKLMNOPQRSTQV";
		int const StarMarkPos[3] = { 3 , 9 , 15 };

		using namespace Go;

		Graphics2D& g = ::Global::getGraphics2D();

		Go::Board const& board = mGame.getBoard();

		int size = board.getSize();
		int length = (size - 1) * CellSize;

		int border = 40;
		int boardSize = length + 2 * border;
		RenderUtility::SetPen(g, Color::eBlack);
		RenderUtility::SetBrush(g, Color::eOrange);
		g.drawRect(BoardPos - Vec2i(border, border), Vec2i(boardSize, boardSize));

		Vec2i posV = BoardPos;
		Vec2i posH = BoardPos;

		RenderUtility::SetFont(g, FONT_S12);
		g.setTextColor(0, 0, 0);
		for( int i = 0; i < size; ++i )
		{
			g.drawLine(posV, posV + Vec2i(0, length));
			g.drawLine(posH, posH + Vec2i(length, 0));

			FixString< 64 > str;
			str.format("%2d", i + 1);
			g.drawText(posH - Vec2i(30, 8), str);
			g.drawText(posH + Vec2i(12 + length, -8), str);

			str.format("%c", CoordStr[i]);
			g.drawText(posV - Vec2i(5, 30), str);
			g.drawText(posV + Vec2i(-5, 15 + length), str);

			posV.x += CellSize;
			posH.y += CellSize;
		}

		RenderUtility::SetPen(g, Color::eBlack);
		RenderUtility::SetBrush(g, Color::eBlack);

		int const starRadius = 5;
		switch( size )
		{
		case 19:
			{
				Vec2i pos;
				for( int i = 0; i < 3; ++i )
				{
					pos.x = BoardPos.x + StarMarkPos[i] * CellSize;
					for( int j = 0; j < 3; ++j )
					{
						pos.y = BoardPos.y + StarMarkPos[j] * CellSize;
						g.drawCircle(pos, starRadius);
					}
				}
			}
			break;
		case 13:
			{
				Vec2i pos;
				for( int i = 0; i < 2; ++i )
				{
					pos.x = BoardPos.x + StarMarkPos[i] * CellSize;
					for( int j = 0; j < 2; ++j )
					{
						pos.y = BoardPos.y + StarMarkPos[j] * CellSize;
						g.drawCircle(pos, starRadius);
					}
				}
				g.drawCircle(BoardPos + CellSize * Vec2i(6, 6), starRadius);
			}
			break;
		}

		int lastPlayPos[2] = { -1,-1 };
		mGame.getLastStepPos(lastPlayPos);
		for( int i = 0; i < size; ++i )
		{
			for( int j = 0; j < size; ++j )
			{
				int data = board.getData(i, j);
				Vec2i pos = BoardPos + CellSize * Vec2i(i, j);
				if( data )
				{
					drawStone(g, pos, data);
				}

				if( i == lastPlayPos[0] && j == lastPlayPos[1] )
				{
					RenderUtility::SetPen(g, Color::eRed);
					RenderUtility::SetBrush(g, Color::eRed);
					g.drawCircle(pos, StoneSize / 2);

				}
			}
		}
	}
	void drawStone(Graphics2D& g, Vec2i const& pos, int color)
	{
		using namespace Go;

		RenderUtility::SetPen(g, Color::eBlack);
		RenderUtility::SetBrush(g, (color == Board::eBlack) ? Color::eBlack : Color::eWhite);
		g.drawCircle(pos, StoneSize);
		if( color == Board::eBlack )
		{
			RenderUtility::SetBrush(g, Color::eWhite);
			g.drawCircle(pos + Vec2i(5, -5), 3);
		}
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
		}
		return false;
	}

	virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
	{
		switch( id )
		{
		default:
			break;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}
protected:
};

REGISTER_STAGE("LeelaZero Learning", LeelaZeroLearningStage, EStageGroup::Test);