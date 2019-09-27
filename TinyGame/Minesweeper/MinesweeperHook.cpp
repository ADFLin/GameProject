
//#define WINVER      0x0400
////#define _WIN32_WINNT  0x0400
//#define _WIN32_IE   0x0400
//#define _RICHEDIT_VER   0x0100
//#define _WTL_USE_CSTRING

#if 0

#include <atlbase.h>
#include <atlapp.h>

#include "CSolverFrame.h"
#include "resource.h"
#endif

#include <deque>

#ifdef max
#undef max
#undef min
#endif
#include <algorithm>

#include "GameInterface.h"

class ActionExecute
{
public:
	enum Action
	{
		Mark ,
		Open ,
		FullClean ,
	};
	struct ActionInfo
	{
		int    x,y;
		Action action;
	};
	typedef std::deque< ActionInfo > ActionQueue;
	void addAction( Action action , int x , int y );

	virtual bool doAction( ActionInfo const& action );

	enum
	{

	};
	void update( int maxRepeat )
	{

	}

	ActionQueue mActQueue;
};

#if 0

CAppModule _Module;

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CSolverFrame wndMain;
	if ( !wndMain.init() )
		return 0;

	if(wndMain.CreateEx() == NULL)
	{
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}

	wndMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);
	// If you are running on NT 4.0 or higher you can use the following call instead to 
	// make the EXE free threaded. This means that calls come in on a random RPC thread.
	//  HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES); // add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}

#include <iostream>

int main()
{
	GControlerWin7 hook;
	if ( !hook.hookGame() )
	{
		std::cout << "can't find window" << std::endl;
		return 0;
	}

	//MineSweeperSolver solver( hook );

	MessageBox(NULL, TEXT("Press OK to close."), TEXT("") , MB_OK);

	//solver.refreshInformation();
	//solver.print();


	//while( solver.setepLogicSolve() ){ 	::Sleep( 100 ); }

	//solver.print();
	

	for( int i = 0 ; i < 10 ; ++i )
	{
		//Win7MSHook::MouseLeftClick( hook.hWndStatic , 200 , 200 );
		//hook.openCell(i,i);
		//::Sleep( 500 );
	}


	return 0;
}

#endif
