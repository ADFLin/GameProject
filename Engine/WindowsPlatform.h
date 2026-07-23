#ifndef Win32Platform_h__
#define Win32Platform_h__

#include "WindowsHeader.h"

#include "Math/Vector2.h"
#include "Platform/Windows/WindowsWindowBase.h"

class WindowsPlatform
{
public:
	WindowsPlatform(){ m_hInstance = ::GetModuleHandle( NULL );  }
	HINSTANCE getAppInstance(){ return m_hInstance; }
	bool      updateSystem()
	{
		MSG msg;
		while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
		{
			// Process the message
			if ( msg.message == WM_QUIT)
				return false;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return true;
	}

	HINSTANCE m_hInstance;
};


template< class T >
class WinFrameT : public WindowsWindowBase
{
	T* _this(){ return static_cast< T* >( this ); }
public:
	typedef WinFrameT< T > WinFrame;

	 WinFrameT();
	~WinFrameT();

	bool      create( TCHAR const* szTitle, int iWidth , int iHeight,
		               WNDPROC wndProc = DefaultProc ,
				       bool fullscreen = false , 
					   unsigned colorBits = 32 );
	void      destroy();

	void      resize( int w , int h )
	{
		mWidth  = w;
		mHeight = h;
		RECT rect = calcAdjustWindowRect();

		// move the window back to its old position
		BOOL result = SetWindowPos( mhWnd, 0, 0 , 0 , 		
			rect.right -rect.left ,rect.bottom - rect.top , SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

		_this()->onWindowResize();
	}


	unsigned  getColorBits() const { return mColorBits; }
	bool      toggleFullscreen()
	{
#if 0
		//http://blogs.msdn.com/b/oldnewthing/archive/2010/04/12/9994016.aspx
		static WINDOWPLACEMENT g_wpPrev = { sizeof(g_wpPrev) };

		DWORD dwStyle = GetWindowLong(m_hWnd, GWL_STYLE);
		if (dwStyle & WS_OVERLAPPEDWINDOW)
		{
			MONITORINFO mi = { sizeof(mi) };
			if (GetWindowPlacement(m_hWnd, &g_wpPrev) &&
				GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY), &mi))
			{
				SetWindowLong(m_hWnd, GWL_STYLE,
					dwStyle & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(m_hWnd, HWND_TOP,
					mi.rcMonitor.left, mi.rcMonitor.top,
					mi.rcMonitor.right - mi.rcMonitor.left,
					mi.rcMonitor.bottom - mi.rcMonitor.top,
					SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
			}
		}
		else
		{
			SetWindowLong(m_hWnd, GWL_STYLE,
				dwStyle | WS_OVERLAPPEDWINDOW);
			SetWindowPlacement(m_hWnd, &g_wpPrev);
			SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
#else
		if (!mbFullscreen)
		{
			if (!setFullScreen(mColorBits))
				return false;

			SetWindowRgn(mhWnd, 0, false);
			// switch off the title bar
			RECT rect;
			::GetWindowRect(mhWnd, &rect);

			mPositionBeforeFullscreen.x = rect.left;
			mPositionBeforeFullscreen.y = rect.top;
			SetWindowLong(mhWnd, GWL_STYLE, _this()->getWinStyle() & ~WS_CAPTION);

			// move the window to (0,0)
			SetWindowPos(mhWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			InvalidateRect(mhWnd, 0, true);
		}

		else
		{
			::ChangeDisplaySettings(NULL, 0);

			// replace the title bar
			SetWindowLong(mhWnd, GWL_STYLE, _this()->getWinStyle());

			RECT rect;
			SetRect(&rect, 0, 0, mWidth, mHeight);
			AdjustWindowRectEx(&rect, _this()->getWinStyle(), FALSE, _this()->getWinExtStyle());

			// move the window back to its old position
			SetWindowPos(mhWnd, 0, mPositionBeforeFullscreen.x, mPositionBeforeFullscreen.y,
				rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);
			InvalidateRect(mhWnd, 0, true);
		}
#endif

		mbFullscreen = !mbFullscreen;
		return true;
	}
	//////
public:
	DWORD  getWinStyle(){ return WS_MINIMIZEBOX | WS_VISIBLE | WS_CAPTION | WS_SYSMENU ; }
	DWORD  getWinExtStyle(){ return WS_EX_OVERLAPPEDWINDOW; }
	WORD   getIcon(){ return 0; }
	WORD   getSmallIcon(){ return 0; }
	LPTSTR getWinClassName(){ return TEXT("GameWindow"); }
protected:
	bool   setupWindow( bool fullscreen , unsigned colorBits ){ return true; }
	void   onWinodwPrevDestory(){}
	void   onWindowResize(){}
	/////

	static bool  mbHasRegisterClass;
private:
	RECT   calcAdjustWindowRect()
	{
		RECT rect;
		SetRect(&rect, 0, 0, mWidth, mHeight);
		AdjustWindowRectEx(&rect, _this()->getWinStyle(), FALSE, _this()->getWinExtStyle());
		return rect;
	}

	bool      createWindow( TCHAR const* szTitle , bool fullscreen );
};

template< class T >
bool WinFrameT<T>::mbHasRegisterClass = false;

template< class T >
WinFrameT<T>::WinFrameT()
{
	mWidth     = 0;
	mHeight    = 0;
	mhWnd       = NULL;
	mhDC        = NULL;
	mbFullscreen = false;
}

template< class T >
WinFrameT<T>::~WinFrameT()
{
	UnregisterClass( _this()->getWinClassName() , ::GetModuleHandle( NULL ) );
}

template< class T >
bool WinFrameT<T>::create( TCHAR const* szTitle, int iWidth , int iHeight, WNDPROC wndProc,
							       bool fullscreen , unsigned colorBits )
{
	mWidth    = iWidth;
	mHeight   = iHeight;
	mColorBits = colorBits;

	if (!mbHasRegisterClass )
	{
		if( !RegisterWindowClass(
				_this()->getWinClassName(),
				wndProc,
				_this()->getIcon(),
				_this()->getSmallIcon()))
		{
			MessageBox(NULL, TEXT("RegisterClassEx failed!"),
					   _this()->getWinClassName(), MB_ICONERROR
			);
			return false;
		}
		mbHasRegisterClass = true;
	}

	if ( !createWindow( szTitle , fullscreen ) )
		return false;

	if ( !_this()->setupWindow( fullscreen , colorBits ) )
		return false;

	ShowWindow ( getHWnd() , SW_SHOWDEFAULT );
	UpdateWindow ( getHWnd() );

	return true;

}

template< class T >
void WinFrameT<T>::destroy()
{
	_this()->onWinodwPrevDestory();
	destroyInternal();

}


template< class T >
bool WinFrameT<T>::createWindow( TCHAR const* szTitle , bool fullscreen )
{
	mbFullscreen = fullscreen;

	DWORD style ,exStyle;
	if ( fullscreen )
	{
		if ( !setFullScreen( mColorBits ) )
			return false;

		style   = WS_POPUP;
		exStyle = WS_EX_APPWINDOW;
	}
	else
	{
		style   = _this()->getWinStyle();
		exStyle = _this()->getWinExtStyle();
	}

	return WindowsWindowBase::createWindow(szTitle, _this()->getWinClassName(), style, exStyle);
}

#endif // Win32Platform_h__
