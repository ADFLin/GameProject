#ifndef Win32Platform_h__
#define Win32Platform_h__

#include "WindowsHeader.h"

class WindowsPlatform
{
public:
	WindowsPlatform(){ m_hInstance = ::GetModuleHandle( NULL );  }
	HINSTANCE getAppInstance(){ return m_hInstance; }
	long      getMillionSecond(){ return GetTickCount(); }
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
class WinFrameT
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
	HDC       getHDC() const { return m_hDC; }
	HWND      getHWnd() const { return m_hWnd; }

	int       getWidth(){ return m_iWidth; }
	int       getHeight(){ return m_iHeight; }

	void      resize( int w , int h )
	{
		m_iWidth  = w;
		m_iHeight = h;
		RECT rect;
		SetRect (&rect , 0, 0, m_iWidth , m_iHeight );
		AdjustWindowRectEx( &rect, _this()->getWinStyle() , FALSE , _this()->getWinExtStyle() );

		// move the window back to its old position
		BOOL result = SetWindowPos( m_hWnd, 0, 0 , 0 , 		
			rect.right -rect.left ,rect.bottom - rect.top , SWP_NOMOVE | SWP_NOZORDER );

		if( result == FALSE )
		{
			int i = 1;

		}

	}

	unsigned  getColorBits() const { return m_colorBits; }

	bool      toggleFullscreen();

	//////
public:
	DWORD  getWinStyle(){ return WS_MINIMIZEBOX | WS_VISIBLE | WS_CAPTION | WS_SYSMENU ; }
	DWORD  getWinExtStyle(){ return WS_EX_OVERLAPPEDWINDOW; }
	WORD   getIcon(){ return 0; }
	WORD   getSmallIcon(){ return 0; }
	LPTSTR getWinClassName(){ return TEXT("GameWindow"); }
protected:
	bool   setupWindow( bool fullscreen , unsigned colorBits ){ return true; }
	void   destoryWindow();
	/////

protected:
	static LRESULT CALLBACK DefaultProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
	bool      setFullScreen( unsigned bits );

private:
	bool      createWindow( TCHAR const* szTitle , bool fullscreen );
	bool      registerWindow( WNDPROC wndProc , DWORD wIcon ,WORD wSIcon );

	int       m_iWidth;
	int       m_iHeight;
	HWND      m_hWnd;
	HDC       m_hDC;
	unsigned  m_colorBits;
	bool      m_fullscreen;

	bool      mbHasRegisterClass = false;
};


template< class T >
WinFrameT<T>::WinFrameT()
{
	m_iWidth     = 0;
	m_iHeight    = 0;
	m_hWnd       = NULL;
	m_hDC        = NULL;
	m_fullscreen = false;
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
	m_iWidth    = iWidth;
	m_iHeight   = iHeight;
	m_colorBits = colorBits;

	if (!mbHasRegisterClass )
	{
		if( !registerWindow(
			wndProc,
			_this()->getIcon(),
			_this()->getSmallIcon()
		) )
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
	UpdateWindow ( getHWnd());

	return true;

}

template< class T >
void WinFrameT<T>::destroy()
{

	_this()->destoryWindow();

	if( m_fullscreen )
	{
		::ChangeDisplaySettings(NULL, 0);
		m_fullscreen = false;
	}

	if( m_hDC )
	{
		::ReleaseDC(m_hWnd, m_hDC);
		m_hDC = NULL;
	}

	if( m_hWnd )
	{
		HWND hWndToDestroy = m_hWnd;
		m_hWnd = NULL;
		::DestroyWindow(hWndToDestroy);
	}

}

template< class T >
bool WinFrameT<T>::registerWindow( WNDPROC wndProc, DWORD wIcon ,WORD wSIcon )
{
	WNDCLASSEX  wc;

	HINSTANCE hInstance = ::GetModuleHandle( NULL );

	// Create the window class for the main window
	wc.cbSize         = sizeof(WNDCLASSEX);
	wc.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS /*| CS_OWNDC*/ | CS_CLASSDC;
	wc.lpfnWndProc    = wndProc;
	wc.cbClsExtra     = 0;
	wc.cbWndExtra     = 0;
	wc.hInstance      = hInstance;
	wc.hIcon          = LoadIcon(hInstance,MAKEINTRESOURCE(wIcon));
	wc.hIconSm        = LoadIcon(hInstance,MAKEINTRESOURCE(wSIcon));
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName   = NULL;
	wc.lpszClassName  = _this()->getWinClassName();

	// Register the window class
	if (!RegisterClassEx(&wc))
		return false;
	return true;
}


template< class T >
bool WinFrameT<T>::setFullScreen( unsigned bits )
{
	DEVMODE dmScreenSettings;

	memset( &dmScreenSettings , 0 , sizeof( dmScreenSettings) );

	dmScreenSettings.dmSize = sizeof(dmScreenSettings);
	dmScreenSettings.dmPelsWidth	= m_iWidth;
	dmScreenSettings.dmPelsHeight	= m_iHeight;
	dmScreenSettings.dmBitsPerPel	= bits;
	dmScreenSettings.dmFields       = DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

	if ( ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL )
	{
		return false;
	}

	return true;
}

template< class T >
LRESULT CALLBACK WinFrameT<T>::DefaultProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch (message)                  /* handle the messages */
	{
	case WM_DESTROY:
		::PostQuitMessage (0);       /* send a WM_QUIT to the message queue */
		break;
	}
	return ::DefWindowProc (hWnd, message, wParam, lParam);
}

template< class T >
void WinFrameT<T>::destoryWindow()
{

}


template< class T >
bool WinFrameT<T>::createWindow( TCHAR const* szTitle , bool fullscreen )
{
	m_fullscreen = fullscreen;

	DWORD style ,exStyle;
	if ( fullscreen )
	{
		if ( !setFullScreen( m_colorBits ) )
			return false;

		style   = WS_POPUP;
		exStyle = WS_EX_APPWINDOW;
	}
	else
	{
		style   = _this()->getWinStyle();
		exStyle = _this()->getWinExtStyle();
	}


	RECT rect;
	SetRect (&rect , 0, 0, m_iWidth , m_iHeight );
	AdjustWindowRectEx(&rect,style, FALSE ,exStyle);

	m_hWnd = CreateWindowEx(
		exStyle,
		_this()->getWinClassName() ,
		szTitle,
		style ,
		CW_USEDEFAULT, CW_USEDEFAULT,
		rect.right -rect.left ,
		rect.bottom - rect.top ,
		NULL,
		NULL,
		::GetModuleHandle( NULL ) ,
		NULL
	);

	if ( !m_hWnd )
		return false;

	m_hDC = GetDC( m_hWnd );

	return true;
}

template< class T >
bool WinFrameT<T>::toggleFullscreen()
{
#if 0
	//http://blogs.msdn.com/b/oldnewthing/archive/2010/04/12/9994016.aspx
	static WINDOWPLACEMENT g_wpPrev = { sizeof(g_wpPrev) };

	DWORD dwStyle = GetWindowLong( m_hWnd , GWL_STYLE);
	if (dwStyle & WS_OVERLAPPEDWINDOW) 
	{
		MONITORINFO mi = { sizeof(mi) };
		if ( GetWindowPlacement(m_hWnd , &g_wpPrev) &&
			 GetMonitorInfo(MonitorFromWindow(m_hWnd ,MONITOR_DEFAULTTOPRIMARY), &mi) ) 
		{
				SetWindowLong(m_hWnd , GWL_STYLE,
					dwStyle & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(m_hWnd , HWND_TOP,
					mi.rcMonitor.left, mi.rcMonitor.top,
					mi.rcMonitor.right - mi.rcMonitor.left,
					mi.rcMonitor.bottom - mi.rcMonitor.top,
					SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
	} 
	else 
	{
		SetWindowLong(m_hWnd , GWL_STYLE,
			dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(m_hWnd , &g_wpPrev);
		SetWindowPos(m_hWnd , NULL, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
#else
	static POINT pos;
	if ( !m_fullscreen )
	{
		if ( !setFullScreen( m_colorBits ) )
			return false;

		SetWindowRgn(m_hWnd, 0, false);
		// switch off the title bar
		RECT rect;
		::GetWindowRect( m_hWnd , &rect );

		pos.x = rect.left;
		pos.y = rect.top;
		SetWindowLong( m_hWnd , GWL_STYLE , _this()->getWinStyle() & ~WS_CAPTION );

		// move the window to (0,0)
		SetWindowPos(m_hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		InvalidateRect(m_hWnd , 0, true);	
	}

	else
	{
		::ChangeDisplaySettings( NULL , 0);

		// replace the title bar
		SetWindowLong( m_hWnd , GWL_STYLE , _this()->getWinStyle());

		RECT rect;
		SetRect (&rect , 0, 0, m_iWidth , m_iHeight );
		AdjustWindowRectEx( &rect, _this()->getWinStyle() , FALSE , _this()->getWinExtStyle() );

		// move the window back to its old position
		SetWindowPos( m_hWnd, 0, pos.x , pos.y , 		
			rect.right -rect.left ,rect.bottom - rect.top ,  SWP_NOZORDER );
		InvalidateRect( m_hWnd, 0, true );
	}
#endif

	m_fullscreen = !m_fullscreen;
	return true;
}
#endif // Win32Platform_h__
