#ifndef WinGLPlatform_h__
#define WinGLPlatform_h__

#include "Win32Platform.h"

#include <gl\gl.h>		// Header File For The OpenGL32 Library
#include <gl\glu.h>		// Header File For The GLu32 Library

#pragma comment (lib,"OpenGL32.lib")
#pragma comment (lib,"GLu32.lib")


template< class T >
class OpenGLSupportT
{
	inline    T* _this(){ return static_cast< T* >( this ); }
public:
	typedef OpenGLSupportT< T > OpenGLSupport;

	OpenGLSupportT(){ mhRC = NULL; }

	bool initilize( HDC hDC , BYTE colorDepth , bool beWindow = true )
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

			pfd.cColorBits = colorDepth;
			_this()->getDepthStencilBits( pfd.cDepthBits , pfd.cStencilBits );

			int pixelFmt = ::ChoosePixelFormat( hDC , &pfd );

			if (  pixelFmt == -1 || !::SetPixelFormat( hDC , pixelFmt , &pfd )  )
				return false;
		}
		else
		{
			if ( !choosePixelFormat( hDC , colorDepth ) )
				return false;
		}

		mhRC = ::wglCreateContext( hDC );

		if ( mhRC == NULL )
			return false;

		if ( !::wglMakeCurrent( hDC , mhRC )  )
			return false;

		return true;
	}


	bool choosePixelFormat( HDC hDC , BYTE colorDepth )
	{
		//int nMaxWeight = 0;
		//int nBestFormat = 0;
		//int nPixelFormat = 1;
		//PIXELFORMATDESCRIPTOR pfdDesc;
		//DWORD dwRequiredFlags = PFD_DRAW_TO_BITMAP | PFD_GENERIC_FORMAT | PFD_SUPPORT_OPENGL | PFD_SUPPORT_GDI;
		//DWORD dwRejectedFlags = PFD_GENERIC_ACCELERATED;
		//while(DescribePixelFormat( hDC , nPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfdDesc))
		//{
		//	if(  pfdDesc.cColorBits == colorDepth && 
		//		 pfdDesc.iLayerType == PFD_TYPE_RGBA &&
		//		(pfdDesc.dwFlags & dwRequiredFlags) == dwRequiredFlags &&
		//		(pfdDesc.dwFlags & dwRejectedFlags) == 0)
		//	{
		//		int nWeight = pfdDesc.cColorBits * 8 + pfdDesc.cDepthBits * 4 + pfdDesc.cStencilBits + pfdDesc.cAccumBits;
		//		if(nWeight > nMaxWeight)
		//		{
		//			nMaxWeight = nWeight;
		//			nBestFormat = nPixelFormat;
		//		}
		//	}
		//	nPixelFormat++;
		//}
		//if(nBestFormat && ::SetPixelFormat( hDC , nBestFormat, &pfdDesc))
		//	return true;
		//return false;

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

		_this()->getDepthStencilBits( pfd.cDepthBits , pfd.cStencilBits );

		int SelectedPixelFormat = ChoosePixelFormat(hDC, &pfd);
		if (SelectedPixelFormat == 0) {
			(void) MessageBox(WindowFromDC(hDC),
				"Failed to find acceptable pixel format.",
				"OpenGL application error",
				MB_ICONERROR | MB_OK);
			exit(1);
		}

		BOOL retVal = SetPixelFormat(hDC, SelectedPixelFormat, &pfd);
		if (retVal != TRUE) {
			(void) MessageBox(WindowFromDC(hDC),
				"Failed to set pixel format.",
				"OpenGL application error",
				MB_ICONERROR | MB_OK);
			exit(1);
		}

		return true;
	}

	void cleanup()
	{
		if ( mhRC )
			::wglDeleteContext( mhRC );
	}

	void getDepthStencilBits( BYTE& depth , BYTE& stencil )
	{
		depth   = 32;
		stencil = 0;
	}

	bool  makeCurrent( HDC hDC )
	{
		return wglMakeCurrent( hDC , mhRC ) != 0;
	}

	HGLRC getHRC(){ return mhRC; }

private:
	HGLRC mhRC;
};

template < class T >
class WinGLFrameT : public WinFrameT< T >
	              , public OpenGLSupportT< T >
{
	inline    T* _this(){ return static_cast< T* >( this ); }
public:
	typedef WinGLFrameT< T > WinGLFrame;
	WinGLFrameT(){}

public:
	void setupGLSetting()
	{
		glEnable(GL_TEXTURE_2D);    // Enable Texture Mapping ( NEW )
		glShadeModel(GL_SMOOTH);	// Enables Smooth Shading
		glClearColor(0.0f, 0.0f, 0.0f, 0.5f);					// Black Background

		glClearDepth(1.0f);			// Depth Buffer Setup
		glEnable(GL_DEPTH_TEST);	// Enables Depth Testing
		glDepthFunc(GL_LEQUAL);		// The Type Of Depth Test To Do
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	}

	void resizeGLScene( int width, int height )
	{
		if (height==0)
		{
			height=1;
		}

		glViewport(0, 0, width, height);			// Reset The Current Viewport

		glMatrixMode(GL_PROJECTION);				// Select The Projection Matrix
		glLoadIdentity();							// Reset The Projection Matrix

		// Calculate The Aspect Ratio Of The Window
		gluPerspective(45.0f,(GLdouble)width/(GLdouble)height,0.1f,1000.0f);

		glMatrixMode(GL_MODELVIEW);					// Select The Modelview Matrix
		glLoadIdentity();
	}

	bool  setCurrent()
	{
		return ::wglMakeCurrent( this->getHDC() , this->getHRC() ) != 0;
	}

	void swapBuffers()
	{
		glFlush();
		::SwapBuffers( this->getHDC() );
	}

	bool createGLContext( BYTE colorDepth );

protected:
	void   destoryWindow()
	{
		OpenGLSupport::cleanup();
		Win32Platform<T>::destoryWindow();
	}
	bool   setupWindow( bool fullscreen , unsigned colorBit )
	{
		if ( !createGLContext( colorBit ) )
			return false;

		_this()->setupGLSetting();

		return true;
	}
private:
	friend class WinFrameT< T >;
};



template < class T >
bool WinGLFrameT<T>::createGLContext( BYTE colorDepth )
{
	OpenGLSupport::initilize( this->getHDC() , colorDepth );

	RECT rect;
	GetClientRect( this->getHWnd() , &rect );
	resizeGLScene( rect.right - rect.left  , rect.bottom - rect.top );
	return true;
}

#endif // WinGLPlatform_h__
