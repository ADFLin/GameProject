#ifndef WGLContext_h__
#define WGLContext_h__

#include "OpenGLHeader.h"
#include "WindowsHeader.h"
#include "Core/IntegerType.h"

struct WGLPixelFormat
{
	WGLPixelFormat()
	{
		colorBits  = 24;
		alpahaBits = 8;
		depthBits = 24;
		stencilBits = 8;
		numSamples = 1;
	}
	uint8 colorBits;
	uint8 alpahaBits;
	uint8 depthBits;
	uint8 stencilBits;
	uint8 numSamples;
};

class WindowsGLContext
{
public:
	WindowsGLContext();
	~WindowsGLContext();


	bool  init();
	bool  init( HDC hDC , WGLPixelFormat const& format , bool bWindowed = true );
	void  cleanup();
	bool  makeCurrent();
	void  swapBuffer();

	bool  isValid() { return mhRC != NULL; }
	HGLRC getHandle(){ return mhRC; }
	bool  setupPixelFormat(HDC hDC, WGLPixelFormat const& format , bool bWindowed);
private:
	HGLRC mhRC;
	HDC   mhDC;
	// dump window != NULL
	HWND  mhWnd;
};

#endif // WGLContext_h__
