#ifndef WGLContext_h__
#define WGLContext_h__

#include "GLConfig.h"
#include "Core/IntegerType.h"

struct WGLSetupSetting
{
	WGLSetupSetting()
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


	bool  init( HDC hDC , WGLSetupSetting const& setting , bool beWindow = true );
	void  cleanup();
	bool  makeCurrent( HDC hDC );

	bool  isValid() { return mhRC != NULL; }
	HGLRC getHandle(){ return mhRC; }

private:
	HGLRC mhRC;
};

#endif // WGLContext_h__
