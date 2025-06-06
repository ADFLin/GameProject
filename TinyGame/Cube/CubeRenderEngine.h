#ifndef CubeRenderEngine_h__
#define CubeRenderEngine_h__

#include "CubeBase.h"
#include "CubeMesh.h"
#include "CubeWorld.h"

#include "IWorldEventListener.h"

#include "Async/AsyncWork.h"

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

	struct ChunkRenderData
	{
		EDataState state;
		bool     needUpdate;
		Mesh     mesh;
	};

	class RenderEngine : public IWorldEventListener , public IChunkEventListener
	{
	public:
		RenderEngine( int w , int h );
		~RenderEngine();
		void beginRender();
		void renderWorld( ICamera& camera );

		void endRender();


		void onModifyBlock( int bx , int by , int bz );

		void setupWorld( World& world );
	private:
		void cleanupRenderData()
		{
			mGereatePool->cencelAllWorks();
			mGereatePool->waitAllWorkComplete();
			for (auto const& pair : mChunkMap)
			{
				delete pair.second;
			}
			mChunkMap.clear();
		}

		virtual void onChunkAdded(Chunk* chunk){}
		virtual void onPrevRemovChunk(Chunk* chunk){}

		typedef std::unordered_map< uint64 , ChunkRenderData* > ChunkDataMap;

		typedef std::vector< ChunkRenderData* > WorldDataVec;


		QueueThreadPool* mGereatePool;
		World* mClientWorld;
		int    mRenderDepth;
		int    mRenderHeight;
		int    mRenderWidth;
		ChunkDataMap  mChunkMap;
		float  mAspect;
	};


}//namespace Cube

#endif // CubeRenderEngine_h__
