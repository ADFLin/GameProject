#ifndef RenderSystem_h__
#define RenderSystem_h__

#include "common.h"
#include "ISystem.h"


typedef CFly::Scene RenderScene;

class RenderSystem
{
public:
	RenderSystem();
	~RenderSystem();
	bool init( SSystemInitParams& params );

	Viewport*     getScreenViewport(){ return mScreenViewport; }
	CFWorld*      getCFWorld()       { return mCFWorld;  }
	RenderScene*  createRenderScene();
	Material*     createEmptyMaterial();

private:
	CFWorld*  mCFWorld;
	Viewport* mScreenViewport;
};

#endif // RenderSystem_h__