#ifndef CubeRenderEngine_h__
#define CubeRenderEngine_h__

#include "CubeBase.h"
#include "CubeMesh.h"
#include "CubeWorld.h"

#include "IWorldEventListener.h"
#include "RHI/PrimitiveBatch.h"
#include "Async/AsyncWork.h"
#include "Math/GeometryPrimitive.h"

#include <unordered_map>
#include "Memory/BuddyAllocator.h"


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

	struct MeshRenderData
	{
		Render::RHIBufferRef vertexBuffer;
		Render::RHIBufferRef indexBuffer;
	};

	struct DrawCmdArgs
	{
		uint indexCountPerInstance;
		uint instanceCount;
		uint startIndexLocation;
		int  baseVertexLocation;
		uint startInstanceLocation;
	};

	struct MeshRenderPoolData : MeshRenderData
	{
		BuddyAllocatorBase vertexAllocator;
		BuddyAllocatorBase indexAllocator;

		bool initialize();

		int drawFrame = 0;
		TArray< DrawCmdArgs > drawCmdList;
		int cmdOffset;
	};

	struct ChunkRenderData
	{
		enum EState
		{
			eNone,
			eMeshGenerating,
			eMesh,
		};


		EState   state;
		bool     bNeedUpdate = false;
		Chunk*   chunk;
		Math::TAABBox< Vec3f > bound;
		Vec3f    posOffset;

		static int constexpr MaxLayerCount = 32;

		struct Layer
		{
			int index;
			Math::TAABBox< Vec3f > bound;

			MeshRenderPoolData* meshPool;
			DrawCmdArgs args;

			BuddyAllocatorBase::Allocation vertexAllocation;
			BuddyAllocatorBase::Allocation indexAlloction;
		};
		Layer layers[MaxLayerCount];
		int numLayer;
	};

	class RenderEngine : public IWorldEventListener , public IChunkEventListener
	{
	public:
		RenderEngine( int w , int h );
		~RenderEngine();

		bool initializeRHI();
		void releaseRHI();

		void beginRender();
		void renderWorld( ICamera& camera );

		void endRender();


		void onModifyBlock( int bx , int by , int bz );

		void setupWorld( World& world );

		void tick(float deltaTime);
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

		virtual void onChunkAdded(Chunk* chunk);
		virtual void onPrevRemovChunk(Chunk* chunk){}

		void updateRenderData(ChunkRenderData* data);

		typedef std::unordered_map< uint64 , ChunkRenderData* > ChunkDataMap;

		typedef std::vector< ChunkRenderData* > WorldDataVec;

		struct UpdatedRenderData
		{
			Mesh mesh;
			ChunkRenderData* chunkData;
			Math::TAABBox<Vec3f> bound;

			struct Layer
			{
				int index;
				Math::TAABBox< Vec3f > bound;

				uint32 vertexOffset;
				uint32 vertexCount;
				uint32 indexOffset;
				uint32 indexCount;
			};

			Layer layers[ChunkRenderData::MaxLayerCount];
			int numLayer;
		};
	public:
		ICamera* mDebugCamera = nullptr;

		Mutex mMutexPedingAdd;
		TArray< UpdatedRenderData > mPendingAddList;

		QueueThreadPool* mGereatePool;
		World* mClientWorld;
		int    mRenderDepth;
		int    mRenderHeight;
		int    mRenderWidth;
		ChunkDataMap  mChunkMap;
		Render::PrimitivesCollection mDebugPrimitives;
		float  mAspect;
		Vec2i  mViewSize;

		class BlockRenderShaderProgram* mProgBlockRender;
		class BlockRenderShaderProgram* mProgBlockRenderDepth;
		bool bWireframeMode = false;
		double mMergeTimeAcc = 0;

		Render::RHIInputLayoutRef mBlockInputLayout;
		Render::RHITexture2DRef  mTexBlockAtlas;

		TArray< MeshRenderPoolData* > mMeshPool;

		Render::RHITexture2DRef mHZBTexture;
		Render::RHITexture2DRef mSceneTexture;
		Render::RHITexture2DRef mSceneDepthTexture;
		Render::RHIFrameBufferRef mSceneFrameBuffer;


		void resizeRenderTarget();

		void notifyViewSizeChanged(Vec2i const& newSize);

		MeshRenderPoolData* acquireMeshRenderData(Mesh const& mesh);
		MeshRenderPoolData* acquireMeshRenderData(uint32 vertexSize, uint32 indexSize);
		Render::RHIBufferRef mCmdBuffer;
		int mRenderFrame = 0;


	};


}//namespace Cube

#endif // CubeRenderEngine_h__
