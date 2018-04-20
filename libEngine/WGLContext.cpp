#include "WGLContext.h"



#if 1
#define WGL_NUMBER_PIXEL_FORMATS_ARB 0x2000
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_DRAW_TO_BITMAP_ARB 0x2002
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_NEED_PALETTE_ARB 0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB 0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB 0x2006
#define WGL_SWAP_METHOD_ARB 0x2007
#define WGL_NUMBER_OVERLAYS_ARB 0x2008
#define WGL_NUMBER_UNDERLAYS_ARB 0x2009
#define WGL_TRANSPARENT_ARB 0x200A
#define WGL_SHARE_DEPTH_ARB 0x200C
#define WGL_SHARE_STENCIL_ARB 0x200D
#define WGL_SHARE_ACCUM_ARB 0x200E
#define WGL_SUPPORT_GDI_ARB 0x200F
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_STEREO_ARB 0x2012
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_RED_BITS_ARB 0x2015
#define WGL_RED_SHIFT_ARB 0x2016
#define WGL_GREEN_BITS_ARB 0x2017
#define WGL_GREEN_SHIFT_ARB 0x2018
#define WGL_BLUE_BITS_ARB 0x2019
#define WGL_BLUE_SHIFT_ARB 0x201A
#define WGL_ALPHA_BITS_ARB 0x201B
#define WGL_ALPHA_SHIFT_ARB 0x201C
#define WGL_ACCUM_BITS_ARB 0x201D
#define WGL_ACCUM_RED_BITS_ARB 0x201E
#define WGL_ACCUM_GREEN_BITS_ARB 0x201F
#define WGL_ACCUM_BLUE_BITS_ARB 0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB 0x2021
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_AUX_BUFFERS_ARB 0x2024
#define WGL_NO_ACCELERATION_ARB 0x2025
#define WGL_GENERIC_ACCELERATION_ARB 0x2026
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_SWAP_EXCHANGE_ARB 0x2028
#define WGL_SWAP_COPY_ARB 0x2029
#define WGL_SWAP_UNDEFINED_ARB 0x202A
#define WGL_TYPE_RGBA_ARB 0x202B
#define WGL_TYPE_COLORINDEX_ARB 0x202C
#define WGL_TRANSPARENT_RED_VALUE_ARB 0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB 0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB 0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB 0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB 0x203B

#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042

typedef BOOL(WINAPI * PFNWGLCHOOSEPIXELFORMATARBPROC) (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats,
													   int *piFormats, UINT *nNumFormats);

static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;

template< class Fun >
bool LoadGLFunction(char const* name, Fun& fun)
{
	fun = (Fun)wglGetProcAddress(name);
	return fun != nullptr;
}

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch( message )
	{
	case WM_KEYDOWN:
		if( wParam == VK_ESCAPE )
		{
			PostQuitMessage(0);
		}
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;       // message handled
}

bool CreateFakeGLContext()
{
	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc = WindowProcedure;
	wcex.hInstance = GetModuleHandle(NULL);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.lpszClassName = "FakeWindow";
	RegisterClassEx(&wcex);

	HWND hWNDFake = CreateWindow(
		"FakeWindow", "Fake Window",      // window class, title
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN, // style
		0, 0,                       // position x, y
		1, 1,                       // width, height
		NULL, NULL,                 // parent window, menu
		GetModuleHandle(NULL), NULL);           // instance, param

	HDC hDCFake = GetDC(hWNDFake);        // Device Context

	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cAlphaBits = 8;
	pfd.cDepthBits = 24;
	int fakePFDID = ChoosePixelFormat(hDCFake, &pfd);
	if( fakePFDID == 0 )
	{
		return false;
	}
	if( SetPixelFormat(hDCFake, fakePFDID, &pfd) == false )
	{
		return false;
	}
	HGLRC fakeRC = wglCreateContext(hDCFake);    // Rendering Contex
	if( fakeRC == 0 )
	{
		return false;
	}
	if( wglMakeCurrent(hDCFake, fakeRC) == false )
	{
		return false;
	}

	return true;
}

int ChoiceMultiSamplePixeFormat(HDC hDC, WGLSetupSetting const& setting, bool beWindow)
{
	if( wglChoosePixelFormatARB == nullptr )
	{
		if( !CreateFakeGLContext() )
			return 0;
		if( !LoadGLFunction("wglChoosePixelFormatARB", wglChoosePixelFormatARB) )
			return 0;
	}

	float fAttributes[] = { 0,0 };
	int iAttributes[] =
	{
		WGL_DRAW_TO_WINDOW_ARB, beWindow ? GL_TRUE : GL_FALSE ,
		WGL_SUPPORT_OPENGL_ARB,GL_TRUE,
		//WGL_SUPPORT_GDI_ARB , GL_TRUE,
		WGL_ACCELERATION_ARB , WGL_FULL_ACCELERATION_ARB,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB,setting.colorBits,
		WGL_ALPHA_BITS_ARB,setting.alpahaBits,
		WGL_DEPTH_BITS_ARB,setting.depthBits,
		WGL_STENCIL_BITS_ARB,setting.stencilBits,
		WGL_DOUBLE_BUFFER_ARB,GL_TRUE,
		WGL_SAMPLE_BUFFERS_ARB,GL_TRUE,
		WGL_SAMPLES_ARB, setting.numSamples ,
		0,0
	};

	// First We Check To See If We Can Get A Pixel Format For 4 Samples
	int pixelFormat;
	UINT numFormats;
	bool valid = wglChoosePixelFormatARB(hDC, iAttributes, fAttributes, 1, &pixelFormat, &numFormats);
	if( !valid || numFormats == 0 )
		return 0;

	return pixelFormat;
}
#endif

WindowsGLContext::WindowsGLContext()
{
	mhRC = NULL;
}

WindowsGLContext::~WindowsGLContext()
{
	cleanup();
}



int ChooseFullscreenPixelFormat(HDC hDC, WGLSetupSetting const& setting, PIXELFORMATDESCRIPTOR& pfd)
{
#if 0
	int nMaxWeight = 0;
	int nBestFormat = 0;
	int nPixelFormat = 1;
	PIXELFORMATDESCRIPTOR pfdDesc;
	DWORD dwRequiredFlags = PFD_GENERIC_FORMAT | PFD_SUPPORT_OPENGL;
	DWORD dwRejectedFlags = PFD_GENERIC_ACCELERATED;
	while( DescribePixelFormat(hDC, nPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfdDesc) )
	{
		if( pfdDesc.cColorBits == colorDepth &&
		   pfdDesc.iLayerType == PFD_TYPE_RGBA &&
		   (pfdDesc.dwFlags & dwRequiredFlags) == dwRequiredFlags &&
		   (pfdDesc.dwFlags & dwRejectedFlags) == 0 )
		{
			int nWeight = pfdDesc.cColorBits * 8 + pfdDesc.cDepthBits * 4 + pfdDesc.cStencilBits + pfdDesc.cAccumBits;
			if( nWeight > nMaxWeight )
			{
				nMaxWeight = nWeight;
				nBestFormat = nPixelFormat;
			}
		}
		nPixelFormat++;
	}
	if( nBestFormat && ::SetPixelFormat(hDC, nBestFormat, &pfdDesc) )
		return true;
	return false;
#endif

	pfd.dwFlags    = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.cColorBits = GetDeviceCaps(hDC, BITSPIXEL);
	pfd.cDepthBits = setting.depthBits;
	pfd.cStencilBits = setting.stencilBits;

	int SelectedPixelFormat = ChoosePixelFormat(hDC, &pfd);

	return SelectedPixelFormat;
}

bool WindowsGLContext::init(HDC hDC , WGLSetupSetting const& setting , bool beWindow /*= true */)
{
	PIXELFORMATDESCRIPTOR pfd;
	::ZeroMemory( &pfd , sizeof( pfd ) );

	int pixelFormat = 0;
#if 1
	if( setting.numSamples > 1 )
	{
		pixelFormat = ChoiceMultiSamplePixeFormat( hDC , setting , beWindow );

	}
#endif
	if ( pixelFormat == 0 )
	{
		if( beWindow )
		{
			pfd.nSize = sizeof(pfd);
			pfd.nVersion = 1;
			pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
			pfd.iPixelType = PFD_TYPE_RGBA;
			pfd.iLayerType = PFD_MAIN_PLANE;

			pfd.cColorBits = setting.colorBits;
			pfd.cDepthBits = setting.depthBits;
			pfd.cStencilBits = setting.stencilBits;

			pixelFormat = ::ChoosePixelFormat(hDC, &pfd);
		}
		else
		{
			pixelFormat = ChooseFullscreenPixelFormat(hDC, setting , pfd );
		}
	}

	if( pixelFormat == 0 )
		return false;

	BOOL retVal = SetPixelFormat(hDC, pixelFormat, &pfd);
	if( retVal != TRUE )
	{
		(void)MessageBox(WindowFromDC(hDC),
						 "Failed to set pixel format.",
						 "OpenGL application error",
						 MB_ICONERROR | MB_OK);
		return false;
	}

	mhRC = ::wglCreateContext( hDC );

	if ( mhRC == NULL )
		return false;

	if ( !::wglMakeCurrent( hDC , mhRC )  )
		return false;

	return true;
}



void WindowsGLContext::cleanup()
{
	if ( mhRC )
	{
		::wglDeleteContext( mhRC );
		mhRC = 0;
	}
}

bool WindowsGLContext::makeCurrent(HDC hDC)
{
	return wglMakeCurrent( hDC , mhRC ) != 0;
}
