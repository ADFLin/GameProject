#ifndef GControlerWin7_h__
#define GControlerWin7_h__

#include "GameInterface.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cmath>

#define USE_HOOK
#ifdef USE_HOOK
#include "InputHook.h"
#endif

namespace Mine
{
	class InputSimulator
	{
	public:
		enum Button
		{
			eLeftButton = 0,
			eMiddleButton = 1,
			eRightButton = 2,
		};

		bool init();

		static DWORD const sButtonDownFlag[];
		static DWORD const sButtonUpFlag[];

		bool actionKey(HWND hWnd, BYTE vKey);
		void actionMouseClick(Button button, HWND hWnd, int x, int y);
		void actionMouseLeftRightClick(HWND hWnd, int x, int y);
		void actionMouseClickInternal(HWND hWnd, int x, int y, DWORD downFlag, DWORD upFlag);

		void covertScreenToWorld(POINT const& ptCoords, int &dX, int &dY);

	private:
		int mScreenWidth;
		int mScreenHeight;
	};

	class GameController_Windows7 : public IMineControlClient
	{
	public:
		static LRESULT __stdcall CALLBACK MouseHookProc(int code, WPARAM wParam, LPARAM lParam);

		GameController_Windows7();

		virtual bool getCellSize(int& cx, int& cy);
		virtual bool hookGame();

		virtual void openNeighberCell(int cx, int cy);
		virtual int  openCell(int cx, int cy);
		virtual bool markCell(int cx, int cy);
		virtual bool unmarkCell(int cx, int cy);
		virtual void restart();
		virtual bool setupMode(EGameMode mode);
		virtual bool setupCustomMode(int sx, int sy, int numbombs);

		virtual EGameState checkState();


		static int const MinCellStartPosX = 30;
		static int const MinCellStartPosY = 30;
		static int const MinCellLength = 18;
		static int const MinStaticSizeX = 601;

		void OnMouseHook(WPARAM wParam, MOUSEHOOKSTRUCT* mhs);

		bool isIncludedColor(int x, int y, int len, COLORREF color)
		{
			for( int i = 0; i < len; ++i )
				for( int j = 0; j < len; ++j )
				{
					COLORREF cmpColor = ::GetPixel(hDC, x + i, y + j);
					if( IsSameColor(cmpColor, color) )
						return true;
				}
			return false;
		}

		int lookCell(int cx, int cy, bool bWaitResult);

		int readCellNumber(int x, int y);

		float calcCellScale()
		{
			RECT rect;
			::GetClientRect(hWndStatic, &rect);
			int sizeX = rect.right - rect.left;

			return 1.0;
			return float(sizeX) / MinStaticSizeX;
		}
		static void CellToWindowPos(float scale, int cx, int cy, int& x, int& y)
		{
			x = int(scale * (cx * MinCellLength + MinCellStartPosX + MinCellLength / 2));
			y = int(scale * (cy * MinCellLength + MinCellStartPosY + MinCellLength / 2));
		}
		static void WindowPosToCell(float scale, int x, int y, int& cx, int& cy)
		{
			cx = int((x) / scale - MinCellStartPosX) / MinCellLength;
			cy = int((y) / scale - MinCellStartPosY) / MinCellLength;
		}

		static COLORREF sNumberColor[];

		static bool IsSameColor(COLORREF color, COLORREF cmpColor)
		{
			int const error = 20;
			int dR = Math::Abs(GetRValue(color) - GetRValue(cmpColor));
			if( dR > error ) return false;
			int dG = Math::Abs(GetGValue(color) - GetGValue(cmpColor));
			if( dG > error ) return false;
			int dB = Math::Abs(GetBValue(color) - GetBValue(cmpColor));
			if( dB > error ) return false;
			//if ( dR + dB + dG > 30 )
			//	return false;
			return true;
		}
		TCHAR const** mWinowTextMap;
		InputSimulator mInput;
		HWND hWndDlg;
		HDC  hDC;
		HWND hWndStatic;
		HWND hWndFrame;
	};

}


#endif // GControlerWin7_h__