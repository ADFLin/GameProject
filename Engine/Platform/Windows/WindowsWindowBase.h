#pragma once
#ifndef WindowsWindowBase_H_BB99A3AE_FA4A_482D_9346_827243A79EF3
#define WindowsWindowBase_H_BB99A3AE_FA4A_482D_9346_827243A79EF3

#include "WindowsHeader.h"

#include "Math/Vector2.h"

class WindowsWindowBase
{
public:
	HDC       getHDC() const { return mhDC; }
	HWND      getHWnd() const { return mhWnd; }

	int       getWidth() { return mWidth; }
	int       getHeight() { return mHeight; }

	bool      isFullscreen() const{ return mbFullscreen; }
	bool      setFullScreen(unsigned bits);
	void      destroyInternal();


	static bool RegisterWindowClass(LPTSTR className, WNDPROC wndProc, DWORD wIcon, WORD wSIcon);

	TVector2<int> getPosition();
	void          setPosition(TVector2<int> const& InPos);

	bool      isShow() const;
	void      show(bool bShow);

	bool     createWindow(TCHAR const* szTitle, LPTSTR className, DWORD style, DWORD exStyle);

	POINT     mPositionBeforeFullscreen = { 0,0 };
	int       mWidth;
	int       mHeight;
	HWND      mhWnd;
	HDC       mhDC;
	unsigned  mColorBits;
	bool      mbFullscreen;


	LRESULT CALLBACK DefaultProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};




#endif // WindowsWindowBase_H_BB99A3AE_FA4A_482D_9346_827243A79EF3
