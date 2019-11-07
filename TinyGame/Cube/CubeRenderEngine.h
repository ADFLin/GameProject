#ifndef CubeRenderEngine_h__
#define CubeRenderEngine_h__

#include "CubeBase.h"
#include "CubeMesh.h"
#include "IWorldEventListener.h"

#include <unordered_map>

namespace Cube
{
	class BlockRenderer;
	class EntityRenderer;


	class ICamera
	{
	public:
		virtual Vec3f getPos() = 0;
		virtual Vec3f getViewDir() = 0;
		virtual Vec3f getUpDir() = 0;
	};


	class Texture2D
	{

		void release();
	};

	class RenderDevice
	{
		Texture2D* createEmptyTexture();
		Texture2D* createTexture( int w , int h , void* data );
	};

	class RenderEngine : public IWorldEventListener
	{
	public:
		RenderEngine( int w , int h );

		void beginRender();
		void renderWorld( ICamera& camera );

		void drawCroodAxis( float len );

		void endRender();

		BlockRenderer& getBlockRenderer(){ return *mBlockRenderer; }

		void onModifyBlock( int bx , int by , int bz );

		void setupWorld( World& world );
	private:
		BlockRenderer* mBlockRenderer;


		struct WorldData
		{
			static int const NumPass = 1;
			bool     needUpdate;
			uint32   drawList[ NumPass ];
		};
		void cleanupWorldData()
		{





		}

		typedef std::unordered_map< uint64 , WorldData* > WDMap;

		typedef std::vector< WorldData* > WorldDataVec;

		WorldDataVec mWorldDataVec;
		WorldDataVec mSortedWorldDataVec;
		World* mClientWorld;
		int    mRenderDepth;
		int    mRenderHeight;
		int    mRenderWidth;
		WDMap  mWDMap;
		float  mAspect;
		Mesh   mMesh;
	};


}//namespace Cube

#endif // CubeRenderEngine_h__
