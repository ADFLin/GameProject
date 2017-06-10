#ifndef WGLContext_h__
#define WGLContext_h__

#include "GLConfig.h"
#include "IntegerType.h"

class WGLContext
{
public:
	WGLContext();
	~WGLContext();

	struct InitSetting
	{
		InitSetting()
		{
			colorBits   = 32;
			depthBits   = 24;
			stencilBits = 8;
		}
		uint8 colorBits;
		uint8 depthBits;
		uint8 stencilBits;
	};

	bool  init( HDC hDC , InitSetting const& setting , bool beWindow = true );
	void  cleanup();
	bool  makeCurrent( HDC hDC );

	bool  isVaild() { return mhRC != NULL; }
	HGLRC getHandle(){ return mhRC; }

private:
	bool  choosePixelFormat( HDC hDC , InitSetting const& setting );
	HGLRC mhRC;
};

#endif // WGLContext_h__
