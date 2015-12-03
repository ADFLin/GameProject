#include "WGLContext.h"

WGLContext::WGLContext()
{
	mhRC = NULL;
}

WGLContext::~WGLContext()
{
	cleanup();
}

bool WGLContext::init(HDC hDC , InitSetting const& setting , bool beWindow /*= true */)
{
	PIXELFORMATDESCRIPTOR pfd;
	::ZeroMemory( &pfd , sizeof( pfd ) );

	if ( beWindow )
	{
		pfd.nSize    = sizeof( pfd );
		pfd.nVersion = 1;
		pfd.dwFlags  = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.iLayerType = PFD_MAIN_PLANE;

		pfd.cColorBits = setting.colorBits;
		pfd.cDepthBits = setting.depthBits;
		pfd.cStencilBits = setting.stencilBits;

		int pixelFmt = ::ChoosePixelFormat( hDC , &pfd );

		if (  pixelFmt == -1 || !::SetPixelFormat( hDC , pixelFmt , &pfd )  )
			return false;
	}
	else
	{
		if ( !choosePixelFormat( hDC , setting ) )
			return false;
	}

	mhRC = ::wglCreateContext( hDC );

	if ( mhRC == NULL )
		return false;

	if ( !::wglMakeCurrent( hDC , mhRC )  )
		return false;

	return true;
}

bool WGLContext::choosePixelFormat(HDC hDC , InitSetting const& setting)
{
#if 0
	int nMaxWeight = 0;
	int nBestFormat = 0;
	int nPixelFormat = 1;
	PIXELFORMATDESCRIPTOR pfdDesc;
	DWORD dwRequiredFlags = PFD_DRAW_TO_BITMAP | PFD_GENERIC_FORMAT | PFD_SUPPORT_OPENGL | PFD_SUPPORT_GDI;
	DWORD dwRejectedFlags = PFD_GENERIC_ACCELERATED;
	while(DescribePixelFormat( hDC , nPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfdDesc))
	{
		if(  pfdDesc.cColorBits == colorDepth && 
			pfdDesc.iLayerType == PFD_TYPE_RGBA &&
			(pfdDesc.dwFlags & dwRequiredFlags) == dwRequiredFlags &&
			(pfdDesc.dwFlags & dwRejectedFlags) == 0)
		{
			int nWeight = pfdDesc.cColorBits * 8 + pfdDesc.cDepthBits * 4 + pfdDesc.cStencilBits + pfdDesc.cAccumBits;
			if(nWeight > nMaxWeight)
			{
				nMaxWeight = nWeight;
				nBestFormat = nPixelFormat;
			}
		}
		nPixelFormat++;
	}
	if(nBestFormat && ::SetPixelFormat( hDC , nBestFormat, &pfdDesc))
		return true;
	return false;
#endif

	PIXELFORMATDESCRIPTOR pfd = 
	{
		sizeof(PIXELFORMATDESCRIPTOR),	/* size of this pfd */
		1,				/* version num */
		PFD_SUPPORT_OPENGL,		/* support OpenGL */
		0,				/* pixel type */
		0,				/* 8-bit color depth */
		0, 0, 0, 0, 0, 0,		/* color bits (ignored) */
		0,				/* no alpha buffer */
		0,				/* alpha bits (ignored) */
		0,				/* no accumulation buffer */
		0, 0, 0, 0,			/* accum bits (ignored) */
		32,				/* depth buffer */
		0,				/* no stencil buffer */
		0,				/* no auxiliary buffers */
		PFD_MAIN_PLANE,			/* main layer */
		0,				/* reserved */
		0, 0, 0,			/* no layer, visible, damage masks */
	};

	pfd.cColorBits = GetDeviceCaps(hDC, BITSPIXEL);
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.dwFlags |= PFD_DRAW_TO_BITMAP;
	pfd.cColorBits = setting.colorBits;
	pfd.cDepthBits = setting.depthBits;
	pfd.cStencilBits = setting.stencilBits;

	int SelectedPixelFormat = ChoosePixelFormat(hDC, &pfd);
	if (SelectedPixelFormat == 0) 
	{
		(void) MessageBox(WindowFromDC(hDC),
			"Failed to find acceptable pixel format.",
			"OpenGL application error",
			MB_ICONERROR | MB_OK);
		return false;
	}

	BOOL retVal = SetPixelFormat(hDC, SelectedPixelFormat, &pfd);
	if (retVal != TRUE) 
	{
		(void) MessageBox(WindowFromDC(hDC),
			"Failed to set pixel format.",
			"OpenGL application error",
			MB_ICONERROR | MB_OK);
		return false;
	}

	return true;
}

void WGLContext::cleanup()
{
	if ( mhRC )
	{
		::wglDeleteContext( mhRC );
		mhRC = 0;
	}
}

bool WGLContext::makeCurrent(HDC hDC)
{
	return wglMakeCurrent( hDC , mhRC ) != 0;
}
