#include "GControlerWin7.h"

#include <cstdio>

namespace Mine
{
	DWORD const InputSimulator::sButtonDownFlag[] =
	{
		MOUSEEVENTF_LEFTDOWN , MOUSEEVENTF_MIDDLEDOWN ,MOUSEEVENTF_RIGHTDOWN ,
	};

	DWORD const InputSimulator::sButtonUpFlag[] =
	{
		MOUSEEVENTF_LEFTUP ,  MOUSEEVENTF_MIDDLEUP , MOUSEEVENTF_RIGHTUP ,
	};


	bool InputSimulator::init()
	{
		// Get the screen resolution
		HDC hdc = ::GetDC(NULL);
		mScreenWidth = ::GetDeviceCaps(hdc, HORZRES);
		mScreenHeight = ::GetDeviceCaps(hdc, VERTRES);
		::ReleaseDC(NULL, hdc);

		return true;
	}

	void InputSimulator::actionMouseClick(Button button, HWND hWnd, int x, int y)
	{
		DWORD downFlag = sButtonDownFlag[button];
		DWORD upFlag = sButtonUpFlag[button];
		actionMouseClickInternal(hWnd, x, y, downFlag, upFlag);
	}

	void InputSimulator::actionMouseLeftRightClick(HWND hWnd, int x, int y)
	{
		DWORD downFlag = sButtonDownFlag[eLeftButton] | sButtonDownFlag[eRightButton];
		DWORD upFlag = sButtonUpFlag[eLeftButton] | sButtonUpFlag[eRightButton];

		actionMouseClickInternal(hWnd, x, y, downFlag, upFlag);
	}

	void InputSimulator::actionMouseClickInternal(HWND hWnd, int x, int y, DWORD downFlag, DWORD upFlag)
	{
		// Coordinates of our click spot (relative to poker table client area)
		int dX, dY;
		POINT ptCoords;
		ptCoords.x = x;
		ptCoords.y = y;
		// Convert client coords to screen
		::ClientToScreen(hWnd, &ptCoords);
		covertScreenToWorld(ptCoords, dX, dY);


		::BringWindowToTop(hWnd);
		// Remember which window has the focus, so we can restore it to that window.
		GUITHREADINFO info;
		::ZeroMemory(&info, sizeof(info));
		info.cbSize = sizeof(info);
		::GetGUIThreadInfo(NULL, &info);

		// Now lets create our mouse inputs..

#if  1
#define MOUSE_INPUT( TIME ,DX , DY , FLAG ) \
	mouse_event( FLAG , DX , DY , NULL , 0 );\
	::Sleep(TIME);
#else	
		int index = 0;
		int numberOfInputs = 3;
		INPUT input[4];
		MOUSEINPUT* mouseInput;

#define MOUSE_INPUT( TIME , DX , DY , FLAG )\
	input[index].type=INPUT_MOUSE;
		mouseInput = &input[index++].mi;
		mouseInput->dx = DX; \
			mouseInput->dy = DY; \
			mouseInput->mouseData = NULL; \
			mouseInput->dwFlags = FLAG; \
			mouseInput->time = TIME; \
			mouseInput->dwExtraInfo = 0;
#endif

		// Move the mouse to the button...
		MOUSE_INPUT(40, dX, dY, MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE);
		MOUSE_INPUT(5, dX, dY, downFlag | MOUSEEVENTF_ABSOLUTE);
		MOUSE_INPUT(40, dX, dY, upFlag | MOUSEEVENTF_ABSOLUTE);
		//MOUSE_INPUT( 0 , dX , dY , MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE );

#undef MOUSE_INPUT



	// Also get the current mouse position, so we can restore that.
		POINT ptCursor;
		if( ::GetCursorPos(&ptCursor) )
		{
			//++numberOfInputs;
			covertScreenToWorld(ptCursor, dX, dY);
			//mouseInput->dx = dX;
			//mouseInput->dy = dY;
		}

		// Make sure this window isn't obscured behind some other window

		// Now actually send the mouse input
		//::SendInput(numberOfInputs, input, sizeof(INPUT));
		// Reset the focus to the previous window
		//if (info.hwndFocus)
		//	::SetFocus(info.hwndFocus);
	}

	void InputSimulator::covertScreenToWorld(POINT const& ptCoords, int &dX, int &dY)
	{
		// Convert our screen coordinates to world coordinates
		double temp1 = 65535 * ptCoords.x;
		dX = int(temp1 / mScreenWidth);
		temp1 = 65535 * ptCoords.y;
		dY = int(temp1 / mScreenHeight);
	}

	bool InputSimulator::actionKey(HWND hWnd, BYTE vKey)
	{
		if( !::BringWindowToTop(hWnd) )
			return false;
		::keybd_event(vKey, 0, 0, 0);
		::Sleep(20);
		::keybd_event(vKey, 0, KEYEVENTF_KEYUP, 0);
		return true;
	}

	GControlerWin7::GControlerWin7()
	{
		mInput.init();
	}

	bool GControlerWin7::getCellSize(int& sx, int& sy)
	{
		RECT saveRect;
		::GetWindowRect(hWndFrame, &saveRect);


		RECT rect;
		::GetClientRect(hWndStatic, &rect);

		sx = (int)((float(rect.right - rect.left) - 2 * MinCellStartPosX) / MinCellLength + 0.5f);
		sy = (int)((float(rect.bottom - rect.top) - 2 * MinCellStartPosX) / MinCellLength + 0.5f);

		//restore winodw
		//::SetWindowPos( hWndFrame , NULL ,
		//	saveRect.left , saveRect.top,
		//	saveRect.right - saveRect.left ,saveRect.bottom - saveRect.top ,
		//	SWP_NOACTIVATE | SWP_FRAMECHANGED );

		return true;
	}

	bool GControlerWin7::hookGame()
	{
		hWndFrame = ::FindWindow(TEXT("Minesweeper"), NULL);
		if( hWndFrame == NULL )
			return false;

		//::SetForegroundWindow( hWndFrame );
		hWndStatic = ::FindWindowEx(hWndFrame, NULL, TEXT("Static"), NULL);
		if( hWndStatic == NULL )
			return false;

		hDC = GetDC(hWndStatic);
		return true;
	}

	void GControlerWin7::openNeighberCell(int cx, int cy)
	{
		int x, y;
		CellToWindowPos(calcCellScale(), cx, cy, x, y);
		mInput.actionMouseLeftRightClick(hWndFrame, x, y);
	}

	int  GControlerWin7::openCell(int cx, int cy)
	{
		int x, y;
		CellToWindowPos(calcCellScale(), cx, cy, x, y);

		int const MaxTryCount = 2;
		for( int i = 0; i < MaxTryCount; ++i )
		{
			mInput.actionMouseClick(InputSimulator::eLeftButton, hWndFrame, x, y);
			int num = readCellNumber(x, y);

			if( num != CV_UNPROBLED )
				return num;

			::Sleep(50);

			num = readCellNumber(x, y);
			if( num != CV_UNPROBLED )
				return num;
			//if ( num == CN_FLAG )
			//{
			//	mInput.actionMouseClick( InputSimulator::eRightButton , hWndFrame , x , y );
			//	num = getCellNumber( cx , cy );

			//	if ( num != CN_UNKNOWN )
			//		throw HookException()
			//}

		}
		throw ControlException("Mouse Input Error");
	}

	int GControlerWin7::lookCell(int cx, int cy, bool bWaitResult)
	{
		int x, y;
		CellToWindowPos(calcCellScale(), cx, cy, x, y);
		int result = readCellNumber(x, y);
		while ( result == CV_UNPROBLED && bWaitResult )
		{
			::Sleep(50);
			result = readCellNumber(x, y);
		}
		return result;
	}

	bool GControlerWin7::markCell(int cx, int cy)
	{
		int x, y;
		CellToWindowPos(calcCellScale(), cx, cy, x, y);

		int num = readCellNumber(x, y);

		if( num >= 0 )
			return false;

		if( num == CV_FLAG )
			return true;

		mInput.actionMouseClick(InputSimulator::eRightButton, hWndFrame, x, y);
		return readCellNumber(x, y) == CV_FLAG;
	}


	bool GControlerWin7::unmarkCell(int cx, int cy)
	{
		int x, y;
		CellToWindowPos(calcCellScale(), cx, cy, x, y);
		int num = readCellNumber(x, y);

		if( num >= 0 )
			return false;
		if( num != CV_FLAG )
			return true;

		mInput.actionMouseClick(InputSimulator::eRightButton, hWndFrame, x, y);
		return readCellNumber(x, y) == CV_UNPROBLED;
	}

	enum
	{
		LT_LOSE_THE_GAME,
		LT_WIN_THE_GAME,
		LT_SETTING,
	};

	TCHAR* gTextZhTW[] =
	{
		TEXT("輸掉遊戲"),
		TEXT("贏得遊戲"),
		TEXT("選項") ,
	};

	EGameState GControlerWin7::checkState()
	{
		hWndDlg = ::FindWindowEx(NULL, NULL, TEXT("#32770"), gTextZhTW[LT_LOSE_THE_GAME]);
		if( hWndDlg )
			return EGameState::Fail;

		hWndDlg = ::FindWindowEx(NULL, NULL, TEXT("#32770"), gTextZhTW[LT_WIN_THE_GAME]);
		if( hWndDlg )
			return EGameState::Complete;

		return EGameState::Run;
	}

	void GControlerWin7::restart()
	{
		if( hWndDlg )
		{
			mInput.actionKey(hWndDlg, 'P');
		}
		else
		{
			mInput.actionKey(hWndFrame, 'F2');
			HWND hWnd = ::FindWindowEx(NULL, NULL, TEXT("#32770"), gTextZhTW[LT_WIN_THE_GAME]);
		}
	}

	int GControlerWin7::readCellNumber(int x, int y)
	{
		static int const numberOffset[][2] =
		{
			{5,0},
			{1,-4},{3,-3},{0,0},{2,2},
			{-2,-2},{-2,1},{0,4},{-2,3},
		};

		static COLORREF const sNumberColor[] =
		{
			RGB(214,228,227),//empty
			RGB(62, 80,190),//1
			RGB(30 ,103,  2),//2
			RGB(172,  6,  8),//3
			RGB(1 , 2 , 128),//4
			RGB(123 , 0 , 0),//5
			RGB(1, 124, 126) ,//6
			RGB(170,  9,  5) ,//7
			RGB(171, 6 , 11) ,//8
		};

		int num = CV_UNPROBLED;
		for( int i = 1; i <= 8; ++i )
		{
			int nx = x + numberOffset[i][0];
			int ny = y + numberOffset[i][1];
			if( isSameColor(::GetPixel(hDC, nx, ny), sNumberColor[i]) ||
			    isSameColor(::GetPixel(hDC, nx + 1, ny), sNumberColor[i]) ||
			    isSameColor(::GetPixel(hDC, nx, ny + 1), sNumberColor[i]) )
			{
				num = i;
				break;
			}
		}
		switch( num )
		{
		case 1:
			if( isSameColor(::GetPixel(hDC, x + numberOffset[0][0], y + numberOffset[0][1]), sNumberColor[0]) ||
			   isSameColor(::GetPixel(hDC, x + numberOffset[0][0], y + numberOffset[0][1]), RGB(175, 184, 215)) ||
			   isSameColor(::GetPixel(hDC, x - numberOffset[0][0], y - numberOffset[0][1]), sNumberColor[0]) ||
			   isSameColor(::GetPixel(hDC, x - numberOffset[0][0], y - numberOffset[0][1]), RGB(175, 184, 215)) )
				return 1;
			break;
		case 3:
			if( isSameColor(::GetPixel(hDC, x + numberOffset[8][0], y + numberOffset[8][1]), sNumberColor[8]) )
				return 8;
			return 3;
		case CV_UNPROBLED:
			if( isSameColor(::GetPixel(hDC, x + numberOffset[0][0], y + numberOffset[0][1]), sNumberColor[0]) ||
			   isSameColor(::GetPixel(hDC, x + numberOffset[0][0], y + numberOffset[0][1]), RGB(175, 184, 215)) ||
			   isSameColor(::GetPixel(hDC, x - numberOffset[0][0], y - numberOffset[0][1]), sNumberColor[0]) ||
			   isSameColor(::GetPixel(hDC, x - numberOffset[0][0], y - numberOffset[0][1]), RGB(175, 184, 215)) )
				return 0;
			break;
		default:
			return num;
		}
		return CV_UNPROBLED;
	}

	bool GControlerWin7::setupMode(GameMode mode)
	{
		HWND hWnd = ::FindWindowEx(NULL, NULL, TEXT("#32770"), gTextZhTW[LT_SETTING]);
		if( !hWnd )
		{
			mInput.actionKey(hWndFrame, VK_F5);
			hWnd = ::FindWindowEx(NULL, NULL, TEXT("#32770"), gTextZhTW[LT_SETTING]);
			if( !hWnd )
				return false;
		}

		::Sleep(100);

		switch( mode )
		{
		case GM_BEGINNER:
			mInput.actionKey(hWnd, 'B');
			break;
		case GM_INTERMEDITE:
			mInput.actionKey(hWnd, 'J');
			break;
		case GM_EXPERT:
			mInput.actionKey(hWnd, 'V');
			break;
		}
		mInput.actionKey(hWnd, VK_RETURN);

		return true;
	}

	bool GControlerWin7::setupCustomMode(int sx, int sy, int numbomb)
	{
		HWND hWnd = ::FindWindowEx(NULL, NULL, TEXT("#32770"), gTextZhTW[LT_SETTING]);
		if( !hWnd )
		{
			mInput.actionKey(hWndFrame, VK_F5);

			hWnd = ::FindWindowEx(NULL, NULL, TEXT("#32770"), gTextZhTW[LT_SETTING]);
			if( !hWnd )
				return false;
		}

		::Sleep(10);
		if( !mInput.actionKey(hWnd, 'U') )
			return false;

		DWORD const idWidthEdit = 0x3f3;
		DWORD const idHeightEdit = 0x3fb;
		DWORD const idBombEdit = 0x3f4;

		TCHAR str[256] = "120";
		sprintf_s(str, "%d", sx);
		::SendMessage(::GetDlgItem(hWnd, idWidthEdit), WM_SETTEXT, 0, (LPARAM)str);
		sprintf_s(str, "%d", sy);
		::SendMessage(::GetDlgItem(hWnd, idHeightEdit), WM_SETTEXT, 0, (LPARAM)str);
		sprintf_s(str, "%d", numbomb);
		::SendMessage(::GetDlgItem(hWnd, idBombEdit), WM_SETTEXT, 0, (LPARAM)str);
		//mInput.actionKey( hWnd , VK_RETURN );

		return true;
	}

}//namespace Mine