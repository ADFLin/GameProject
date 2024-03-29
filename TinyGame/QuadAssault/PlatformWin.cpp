#include "PlatformWin.h"

#include "PlatformConfig.h"

int64 PlatformWin::GetTickCount()
{
	return  ::GetTickCount();
}

PlatformWindow* PlatformWin::CreateWindow( char const* title , Vec2i const& size , int colorBit , bool bFullScreen )
{
	PlatformWindow* win = new PlatformWindow;
	if ( !win->create( title , size , colorBit , bFullScreen ) )
	{
		delete win;
		return NULL;
	}
	return win;
}

PlatformGLContext* PlatformWin::CreateGLContext( PlatformWindow& window , GLConfig& config )
{
	PlatformGLContext* context = new PlatformGLContext;
	if ( !context->init( window , config ) )
	{
		delete context;
		return NULL;
	}
	return context;
}

bool PlatformWin::IsKeyPressed( unsigned key )
{
	return ( GetAsyncKeyState(key) & 0x8000) != 0;
}

bool PlatformWin::IsButtonPressed( unsigned button )
{
	int key = 0;
	switch (button)
	{
	case EMouseKey::Left:  key = GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON; break;
	case EMouseKey::Right:  key = GetSystemMetrics(SM_SWAPBUTTON) ? VK_LBUTTON : VK_RBUTTON; break;
	case EMouseKey::Middle:  key = VK_MBUTTON;  break;
	case EMouseKey::X1: key = VK_XBUTTON1; break;
	case EMouseKey::X2: key = VK_XBUTTON2; break;
	}
	return (GetAsyncKeyState(key) & 0x8000) != 0;
}

GameWindowWin::GameWindowWin()
{
	mListener = NULL;
	mMouseState = 0;

	mSize = Vec2i::Zero();
	mhWnd = NULL;
}

LRESULT CALLBACK GameWindowWin::DefaultProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch (message)                  /* handle the messages */
	{
	case WM_CREATE:
		{
			CREATESTRUCT* ps = (CREATESTRUCT*)lParam;
			SetWindowLong( hWnd ,
				#if TARGET_PLATFORM_64BITS
						  GWLP_USERDATA, (LONG)ps->lpCreateParams
				#else
						  GWL_USERDATA, (LONG)ps->lpCreateParams
				#endif
			);
		}
		break;
	case WM_DESTROY:
		::PostQuitMessage (0);       /* send a WM_QUIT to the message queue */
		break;

	}

	GameWindowWin* window = ( GameWindowWin* )GetWindowLong( hWnd , 
#if TARGET_PLATFORM_64BITS
	GWLP_USERDATA
#else
	GWL_USERDATA
#endif
	);

	if ( !window->procMsg( message , wParam , lParam ) )
		return 0;

	return ::DefWindowProc( hWnd , message , wParam , lParam );
}

bool GameWindowWin::procMsg( UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message )
	{
	case WM_PAINT:
		::BeginPaint( mhWnd , NULL );
		::EndPaint( mhWnd , NULL );
		return true;
	case WM_CLOSE:
		return false;
	case WM_MBUTTONDOWN : case WM_MBUTTONUP : case WM_MBUTTONDBLCLK :
	case WM_LBUTTONDOWN : case WM_LBUTTONUP : case WM_LBUTTONDBLCLK :
	case WM_RBUTTONDOWN : case WM_RBUTTONUP : case WM_RBUTTONDBLCLK :
	case WM_MOUSEMOVE:    case WM_MOUSEWHEEL:
		return precMouseMsg( message , wParam , lParam );

	case WM_KEYDOWN: case WM_KEYUP: 
		return mListener->onKey( KeyMsg( EKeyCode::Type( wParam ) , message == WM_KEYDOWN ) ).isHandled() == false;
	case WM_CHAR:
		return mListener->onChar( wParam ).isHandled() == false;
	}
	return true;

}

bool GameWindowWin::precMouseMsg( UINT message, WPARAM wParam, LPARAM lParam )
{
	unsigned button = 0;

	switch ( message )
	{
#define CASE_MOUSE_MSG( BUTTON , WDOWN , WUP , WDCLICK )\
case WDOWN:\
	mMouseState |= BUTTON;\
	button = BUTTON | MBS_DOWN;\
	::SetCapture( mhWnd );\
	break;\
case WUP:\
	mMouseState &= ~BUTTON ;\
	button = BUTTON ;\
	::ReleaseCapture();\
	break;\
case WDCLICK:\
	button = BUTTON | MBS_DOUBLE_CLICK ;\
	break;

		CASE_MOUSE_MSG( MBS_MIDDLE , WM_MBUTTONDOWN , WM_MBUTTONUP , WM_MBUTTONDBLCLK )
		CASE_MOUSE_MSG( MBS_LEFT   , WM_LBUTTONDOWN , WM_LBUTTONUP , WM_LBUTTONDBLCLK )
		CASE_MOUSE_MSG( MBS_RIGHT  , WM_RBUTTONDOWN , WM_RBUTTONUP , WM_RBUTTONDBLCLK )
#undef CASE_MOUSE_MSG

case WM_MOUSEMOVE:
	button = MBS_MOVING;
	break;
case WM_MOUSEWHEEL:
	{
		int zDelta = HIWORD(wParam);
		if ( zDelta == WHEEL_DELTA )
			button = MBS_WHEEL;
		else
			button = MBS_WHEEL | MBS_DOWN;
	}
	break;
	}

	int x = (int) LOWORD(lParam);
	int y = (int) HIWORD(lParam);

	if (x >= 32767) x -= 65536;
	if (y >= 32767) y -= 65536;

	return mListener->onMouse( MouseMsg( x , y , button , mMouseState ) ).isHandled() == false;
}

bool GameWindowWin::create( char const* title , Vec2i const& size , int colorBit , bool bFullScreen )
{
	TCHAR const* FRAME_CLASS_NAME = TEXT("QAWindow");
	WNDCLASSEX  wc;

	HINSTANCE hInstance = ::GetModuleHandle( NULL );

	// Create the window class for the main window
	wc.cbSize         = sizeof(WNDCLASSEX);
	wc.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS /*| CS_OWNDC*/ | CS_CLASSDC;
	wc.lpfnWndProc    = DefaultProc;
	wc.cbClsExtra     = 0;
	wc.cbWndExtra     = 0;
	wc.hInstance      = hInstance;
	wc.hIcon          = LoadIcon(hInstance,MAKEINTRESOURCE(0));
	wc.hIconSm        = LoadIcon(hInstance,MAKEINTRESOURCE(0));
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName   = NULL;
	wc.lpszClassName  = FRAME_CLASS_NAME;

	// Register the window class
	if (!RegisterClassEx(&wc))
		return false;

	DWORD winStyle ,exStyle;

	if ( bFullScreen )
	{
		DEVMODE dmScreenSettings;

		memset( &dmScreenSettings , 0 , sizeof( dmScreenSettings) );

		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth	= size.x;
		dmScreenSettings.dmPelsHeight	= size.y;
		dmScreenSettings.dmBitsPerPel	= colorBit;
		dmScreenSettings.dmFields       = DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		if ( ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL )
		{
			return false;
		}

		winStyle = WS_POPUP;
		exStyle  = WS_EX_APPWINDOW;
	}
	else
	{
		winStyle  = WS_MINIMIZEBOX | WS_VISIBLE | WS_CAPTION | WS_SYSMENU ;
		exStyle   = WS_EX_OVERLAPPEDWINDOW;
	}

	RECT rect;
	SetRect (&rect , 0, 0, size.x , size.y );
	AdjustWindowRectEx(&rect,winStyle, FALSE ,exStyle);

	mhWnd = CreateWindowEx(
		exStyle,
		FRAME_CLASS_NAME ,
		title ,
		winStyle ,
		CW_USEDEFAULT, CW_USEDEFAULT,
		rect.right -rect.left ,
		rect.bottom - rect.top ,
		NULL,
		NULL,
		hInstance ,
		LPVOID( this ) );


	if ( !mhWnd )
		return false;

	mSize = size;

	ShowWindow( mhWnd , SW_SHOWDEFAULT );
	UpdateWindow( mhWnd );

	return true;
}

void GameWindowWin::close()
{
	if ( mhWnd )
		::DestroyWindow( mhWnd );
	mhWnd = NULL;
}



void GameWindowWin::showCursor( bool bShow )
{
	::ShowCursor( bShow );
}


GameWindowWin::~GameWindowWin()
{
	close();
}

void GameWindowWin::procSystemMessage()
{
	MSG msg;
	while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE ) )
	{
		// Process the message
		if ( msg.message == WM_QUIT )
			return;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

GLContextWindows::GLContextWindows()
{
	mhDC  = NULL;
	mhRC  = NULL;
}

GLContextWindows::~GLContextWindows()
{
	if ( mhRC )
		::wglDeleteContext( mhRC );

	mhDC  = NULL;
	mhRC  = NULL;
}

bool GLContextWindows::init( GameWindowWin& window , GLConfig& config )
{


	mhDC = ::GetDC( window.getSystemHandle() );

	PIXELFORMATDESCRIPTOR pfd;
	::ZeroMemory( &pfd , sizeof( pfd ) );


	pfd.nSize    = sizeof( pfd );
	pfd.nVersion = 1;
	pfd.dwFlags    = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.cColorBits   = config.colorBits;
	pfd.cDepthBits   = 24;
	pfd.cStencilBits = 8;
	//pfd.cAlphaBits   = ( colorBit == 32 ) ? 8 : 0;
	pfd.cAlphaBits   = 0;

	int pixelFmt = ::ChoosePixelFormat( mhDC , &pfd );

	if (  pixelFmt == -1 || !::SetPixelFormat( mhDC , pixelFmt , &pfd )  )
		return false;

	mhRC = ::wglCreateContext( mhDC );

	if ( mhDC == NULL )
		return false;

	if ( !::wglMakeCurrent( mhDC , mhRC )  )
		return false;

	return true;
}

void GLContextWindows::release()
{
	delete this;
}

void GLContextWindows::swapBuffers()
{
	::SwapBuffers( mhDC );
}

bool GLContextWindows::setCurrent()
{
	if ( !::wglMakeCurrent( mhDC , mhRC ) )
		return false;
	return true;
}
