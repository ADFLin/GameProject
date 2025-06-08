#ifndef CubeScene_h__
#define CubeScene_h__

#include "CubeBase.h"

#include "CubeRenderEngine.h"

namespace Cube
{
	class Scene
	{
	public:
		Scene( int w , int h )
		{
			mWorld = NULL;
			mRenderEngine = new RenderEngine( w , h );
		}

		void changeWorld( World& world )
		{
			if ( mWorld == &world )
				return;
			mWorld = &world;
			mRenderEngine->setupWorld( world );
		}
		void render( ICamera& camera )
		{
			mRenderEngine->beginRender();
			mRenderEngine->renderWorld( camera );
			mRenderEngine->endRender();
		}

		void tick(float deltaTime)
		{

			mRenderEngine->tick(deltaTime);
		}

		World*        mWorld;
		RenderEngine* mRenderEngine;
	};

}//namespace Cube


#endif // CubeScene_h__